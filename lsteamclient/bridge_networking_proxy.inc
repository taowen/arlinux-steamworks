/* Bionic-side legacy SteamNetworking006 P2P proxy. */
static bool bridge_network_rpc( lsteam_bridge_game_server_request *request,
                                lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_NETWORKING_CALL,
                             request, response );
}

static int8_t bridge_network_send( void *, CSteamID remote, const void *data,
                                   uint32_t size, uint32_t send_type,
                                   int32_t channel )
{
    if ((size && !data) || size > 16u * 1024u * 1024u) return 0;
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        uint32_t chunk = size - offset;
        if (chunk > LSTEAM_BRIDGE_MAX_TICKET)
            chunk = LSTEAM_BRIDGE_MAX_TICKET;
        request.method = 0;
        request.steam_id = bridge_iface_id_value( remote );
        request.arg0 = send_type;
        request.arg1 = channel;
        request.arg2 = size;
        request.arg3 = offset;
        request.data_size = chunk;
        if (chunk) memcpy( request.data, (const uint8_t *)data + offset, chunk );
        if (!bridge_network_rpc( &request, &response ) || !response.result)
            return 0;
        offset += chunk;
    } while (offset < size);
    return 1;
}

static int8_t bridge_network_available( void *, uint32_t *size, int32_t channel )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    request.arg0 = channel;
    if (!bridge_network_rpc( &request, &response )) return 0;
    if (size) *size = response.arg0;
    return response.result != 0;
}

static int8_t bridge_network_read( void *, void *data, uint32_t capacity,
                                   uint32_t *size, CSteamID *remote,
                                   int32_t channel )
{
    if (size) *size = 0;
    if (!data || !capacity || capacity > 16u * 1024u * 1024u) return 0;
    uint32_t offset = 0, total = 0;
    uint64_t sender = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 2;
        request.arg0 = capacity;
        request.arg1 = channel;
        request.arg2 = offset;
        if (!bridge_network_rpc( &request, &response )) return 0;
        if (!offset)
        {
            total = response.arg0;
            sender = response.steam_id;
            if (!response.result)
            {
                if (size) *size = total;
                return 0;
            }
        }
        if (total > capacity || response.data_size > total - offset) return 0;
        if (response.data_size)
            memcpy( (uint8_t *)data + offset, response.data,
                    response.data_size );
        offset += response.data_size;
        if (offset < total && !response.data_size) return 0;
    } while (offset < total);
    if (size) *size = total;
    if (remote) *remote = bridge_iface_make_id( sender );
    return 1;
}

static int8_t bridge_network_id_bool( uint32_t method, CSteamID remote,
                                      uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( remote );
    request.arg0 = argument;
    return bridge_network_rpc( &request, &response ) && response.result;
}
static int8_t bridge_network_accept( void *, CSteamID remote )
{ return bridge_network_id_bool( 3, remote ); }
static int8_t bridge_network_close( void *, CSteamID remote )
{ return bridge_network_id_bool( 4, remote ); }
static int8_t bridge_network_close_channel( void *, CSteamID remote,
                                            int32_t channel )
{ return bridge_network_id_bool( 5, remote, channel ); }

static int8_t bridge_network_session_state( void *, CSteamID remote,
                                            P2PSessionState_t *state )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!state) return 0;
    request.method = 6;
    request.steam_id = bridge_iface_id_value( remote );
    if (!bridge_network_rpc( &request, &response ) || !response.result ||
        response.data_size != sizeof(*state)) return 0;
    memcpy( state, response.data, sizeof(*state) );
    return 1;
}

static int8_t bridge_network_allow_relay( void *, int8_t allowed )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 7;
    request.arg0 = allowed != 0;
    return bridge_network_rpc( &request, &response ) && response.result;
}
