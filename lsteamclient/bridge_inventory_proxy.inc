static bool bridge_inventory_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_INVENTORY_CALL, request, response );
}

static uint64_t bridge_inventory_id_value( CSteamID id )
{
    uint64_t value = 0;
    memcpy( &value, &id, sizeof(value) );
    return value;
}

static uint32_t bridge_inventory_status( void *, int32_t result_handle )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = result_handle;
    return bridge_inventory_rpc( &request, &response )
        ? response.result : 2 /* k_EResultFail */;
}

static int8_t bridge_inventory_items( void *, int32_t result_handle,
                                      SteamItemDetails_t *items,
                                      uint32_t *item_count )
{
    if (!item_count) return 0;
    uint32_t capacity = *item_count;
    uint32_t expected = 0, offset = 0;
    int8_t result = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 1;
        request.arg0 = result_handle;
        request.arg1 = capacity;
        request.arg2 = offset;
        request.arg3 = items != NULL;
        if (!bridge_inventory_rpc( &request, &response )) return 0;
        *item_count = response.arg0;
        result = response.result != 0;
        if (!items || !result) return result;
        if (response.arg0 > capacity) return 0;
        expected = response.arg0 * sizeof(lsteam_bridge_inventory_item);
        if (response.arg1 != expected || offset > expected ||
            response.data_size > expected - offset ||
            response.data_size % sizeof(lsteam_bridge_inventory_item))
            return 0;
        uint32_t count = response.data_size /
                         sizeof(lsteam_bridge_inventory_item);
        const lsteam_bridge_inventory_item *wire =
            reinterpret_cast<const lsteam_bridge_inventory_item *>(response.data);
        uint32_t base = offset / sizeof(*wire);
        for (uint32_t i = 0; i < count; ++i)
        {
            items[base + i].m_itemId = wire[i].item_id;
            items[base + i].m_iDefinition = wire[i].definition;
            items[base + i].m_unQuantity = wire[i].quantity;
            items[base + i].m_unFlags = wire[i].flags;
        }
        offset += response.data_size;
        if (!response.data_size && offset != expected) return 0;
    } while (offset < expected);
    return result;
}

static int8_t bridge_inventory_string(
    uint32_t method, int32_t key, uint32_t index, const char *property,
    char *value, uint32_t *value_size )
{
    if (!value_size) return 0;
    uint32_t capacity = *value_size;
    uint32_t offset = 0, total = 0;
    int8_t result = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = method;
        request.arg0 = key;
        request.arg1 = index;
        request.arg2 = offset;
        request.arg3 = property != NULL;
        if (property && !bridge_http_copy_text(
                request.text, sizeof(request.text), property ))
            return 0;
        if (!bridge_inventory_rpc( &request, &response )) return 0;
        total = response.arg0;
        *value_size = total;
        result = response.result != 0;
        if (!value || !result) return result;
        if (total > capacity || offset > total ||
            response.data_size > total - offset) return 0;
        if (response.data_size)
            memcpy( value + offset, response.data, response.data_size );
        offset += response.data_size;
        if (!response.data_size && offset != total) return 0;
    } while (offset < total);
    return result;
}

static int8_t bridge_inventory_item_property(
    void *, int32_t result_handle, uint32_t item_index,
    const char *property, char *value, uint32_t *value_size )
{
    return bridge_inventory_string( 2, result_handle, item_index, property,
                                    value, value_size );
}

static uint32_t bridge_inventory_timestamp( void *, int32_t result_handle )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.arg0 = result_handle;
    return bridge_inventory_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_inventory_check_steamid( void *, int32_t result_handle,
                                              CSteamID steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.arg0 = result_handle;
    request.steam_id = bridge_inventory_id_value( steam_id );
    return bridge_inventory_rpc( &request, &response ) && response.result;
}

static void bridge_inventory_destroy( void *, int32_t result_handle )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 5;
    request.arg0 = result_handle;
    bridge_inventory_rpc( &request, &response );
}

static int8_t bridge_inventory_result_call( uint32_t method,
                                            int32_t *result_handle )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    if (result_handle) *result_handle = -1;
    if (!result_handle || !bridge_inventory_rpc( &request, &response ) ||
        !response.result) return 0;
    *result_handle = response.arg0;
    return 1;
}

static int8_t bridge_inventory_all( void *, int32_t *result_handle )
{ return bridge_inventory_result_call( 6, result_handle ); }

static int8_t bridge_inventory_upload(
    uint32_t method, const void *data, uint32_t size, uint32_t count_or_size,
    int8_t flag, int32_t *result_handle )
{
    if (size && !data) return 0;
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = method;
        request.arg1 = offset;
        request.arg2 = count_or_size;
        request.arg3 = flag != 0;
        uint32_t count = size - offset;
        if (count > sizeof(request.data)) count = sizeof(request.data);
        request.data_size = count;
        if (count)
            memcpy( request.data, static_cast<const uint8_t *>(data) + offset,
                    count );
        if (!bridge_inventory_rpc( &request, &response )) return 0;
        offset += count;
        if (offset == size)
        {
            if (result_handle) *result_handle = response.arg0;
            return response.result != 0;
        }
    } while (offset < size);
    return 0;
}

static int8_t bridge_inventory_by_id( void *, int32_t *result_handle,
                                      const uint64_t *ids, uint32_t count )
{
    if (result_handle) *result_handle = -1;
    if (!result_handle || count > UINT32_MAX / sizeof(*ids)) return 0;
    return bridge_inventory_upload( 7, ids, count * sizeof(*ids), count, 0,
                                    result_handle );
}

static int8_t bridge_inventory_serialize( void *, int32_t result_handle,
                                          void *buffer, uint32_t *buffer_size )
{
    if (!buffer_size) return 0;
    uint32_t capacity = *buffer_size;
    uint32_t offset = 0, total = 0;
    int8_t result = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 8;
        request.arg0 = result_handle;
        request.arg1 = capacity;
        request.arg2 = offset;
        request.arg3 = buffer != NULL;
        if (!bridge_inventory_rpc( &request, &response )) return 0;
        total = response.arg0;
        *buffer_size = total;
        result = response.result != 0;
        if (!buffer || !result) return result;
        if (total > capacity || offset > total ||
            response.data_size > total - offset) return 0;
        if (response.data_size)
            memcpy( static_cast<uint8_t *>(buffer) + offset, response.data,
                    response.data_size );
        offset += response.data_size;
        if (!response.data_size && offset != total) return 0;
    } while (offset < total);
    return result;
}

static int8_t bridge_inventory_deserialize( void *, int32_t *result_handle,
                                            const void *buffer, uint32_t size,
                                            int8_t require_online )
{
    if (result_handle) *result_handle = -1;
    if (!result_handle) return 0;
    return bridge_inventory_upload( 9, buffer, size, size, require_online,
                                    result_handle );
}

static int8_t bridge_inventory_load_definitions( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    return bridge_inventory_rpc( &request, &response ) && response.result;
}

static int8_t bridge_inventory_id_array(
    uint32_t method, uint64_t steam_id, int32_t *ids, uint32_t *id_count )
{
    if (!id_count) return 0;
    uint32_t capacity = *id_count;
    uint32_t offset = 0, total = 0;
    int8_t result = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = method;
        request.steam_id = steam_id;
        request.arg1 = capacity;
        request.arg2 = offset;
        request.arg3 = ids != NULL;
        if (!bridge_inventory_rpc( &request, &response )) return 0;
        total = response.arg0;
        *id_count = total;
        result = response.result != 0;
        if (!ids || !result) return result;
        uint32_t expected = total * sizeof(*ids);
        if (total > capacity || response.arg1 != expected ||
            offset > expected || response.data_size > expected - offset ||
            response.data_size % sizeof(*ids)) return 0;
        if (response.data_size)
            memcpy( reinterpret_cast<uint8_t *>(ids) + offset, response.data,
                    response.data_size );
        offset += response.data_size;
        if (!response.data_size && offset != expected) return 0;
    } while (offset < total * sizeof(*ids));
    return result;
}

static int8_t bridge_inventory_definition_ids( void *, int32_t *ids,
                                                uint32_t *id_count )
{ return bridge_inventory_id_array( 21, 0, ids, id_count ); }

static int8_t bridge_inventory_definition_property(
    void *, int32_t definition, const char *property, char *value,
    uint32_t *value_size )
{
    return bridge_inventory_string( 22, definition, 0, property, value,
                                    value_size );
}

static uint64_t bridge_inventory_request_eligible( void *, CSteamID steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 23;
    request.steam_id = bridge_inventory_id_value( steam_id );
    return bridge_inventory_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_inventory_eligible_ids( void *, CSteamID steam_id,
                                             int32_t *ids,
                                             uint32_t *id_count )
{
    return bridge_inventory_id_array( 24, bridge_inventory_id_value( steam_id ), ids,
                                      id_count );
}

static uint64_t bridge_inventory_request_prices( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 26;
    return bridge_inventory_rpc( &request, &response ) ? response.result : 0;
}

static uint32_t bridge_inventory_price_count( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 27;
    return bridge_inventory_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_inventory_prices_common( int32_t *definitions,
                                              uint64_t *current,
                                              uint64_t *base,
                                              uint32_t capacity )
{
    if (capacity && (!definitions || !current)) return 0;
    uint32_t offset = 0, total = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 28;
        request.arg1 = capacity;
        request.arg2 = offset;
        request.arg3 = capacity != 0;
        if (!bridge_inventory_rpc( &request, &response ) || !response.result)
            return 0;
        total = response.arg0;
        uint32_t expected = total * sizeof(lsteam_bridge_inventory_price);
        if (total > capacity || response.arg1 != expected || offset > expected ||
            response.data_size > expected - offset ||
            response.data_size % sizeof(lsteam_bridge_inventory_price))
            return 0;
        uint32_t count = response.data_size /
                         sizeof(lsteam_bridge_inventory_price);
        const lsteam_bridge_inventory_price *wire =
            reinterpret_cast<const lsteam_bridge_inventory_price *>(response.data);
        uint32_t first = offset / sizeof(*wire);
        for (uint32_t i = 0; i < count; ++i)
        {
            definitions[first + i] = wire[i].definition;
            current[first + i] = wire[i].current_price;
            if (base) base[first + i] = wire[i].base_price;
        }
        offset += response.data_size;
        if (!response.data_size && offset != expected) return 0;
    } while (offset < total * sizeof(lsteam_bridge_inventory_price));
    return 1;
}

static int8_t bridge_inventory_prices( void *, int32_t *definitions,
                                       uint64_t *current, uint64_t *base,
                                       uint32_t capacity )
{
    if (capacity && !base) return 0;
    return bridge_inventory_prices_common( definitions, current, base,
                                           capacity );
}

static int8_t bridge_inventory_prices_v2( void *, int32_t *definitions,
                                          uint64_t *current,
                                          uint32_t capacity )
{
    return bridge_inventory_prices_common( definitions, current, NULL,
                                           capacity );
}

static int8_t bridge_inventory_price( void *, int32_t definition,
                                      uint64_t *current, uint64_t *base )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 29;
    request.arg0 = definition;
    if (current) *current = 0;
    if (base) *base = 0;
    if (!bridge_inventory_rpc( &request, &response ) || !response.result ||
        response.data_size != 16) return 0;
    if (current) memcpy( current, response.data, 8 );
    if (base) memcpy( base, response.data + 8, 8 );
    return 1;
}

static int8_t bridge_inventory_price_v2( void *self, int32_t definition,
                                         uint64_t *current )
{
    uint64_t base = 0;
    return bridge_inventory_price( self, definition, current, &base );
}

static int8_t bridge_inventory_inspect( void *, int32_t *result_handle,
                                        const char *token )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 37;
    if (result_handle) *result_handle = -1;
    if (!result_handle || !bridge_http_copy_text(
            request.text, sizeof(request.text), token ) ||
        !bridge_inventory_rpc( &request, &response ) || !response.result)
        return 0;
    *result_handle = response.arg0;
    return 1;
}
