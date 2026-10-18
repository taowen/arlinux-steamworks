/* Bionic-side SteamAppList001 proxy. */
static bool bridge_app_list_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_APP_LIST_CALL,
                             request, response );
}

static uint32_t bridge_app_list_count( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    return bridge_app_list_rpc( &request, &response )
        ? response.result : 0;
}

static uint32_t bridge_app_list_apps(
    void *, uint32_t *apps, uint32_t maximum )
{
    if (!maximum) return 0;
    if (!apps || maximum > 65536) return 0;
    uint32_t copied = 0;
    const uint32_t chunk_max = LSTEAM_BRIDGE_MAX_TICKET / sizeof(uint32_t);
    while (copied < maximum)
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 1;
        request.arg0 = copied;
        request.arg1 = maximum - copied < chunk_max
            ? maximum - copied : chunk_max;
        request.arg2 = maximum;
        if (!bridge_app_list_rpc( &request, &response ) ||
            response.data_size % sizeof(uint32_t) ||
            response.data_size > request.arg1 * sizeof(uint32_t))
            break;
        uint32_t count = response.data_size / sizeof(uint32_t);
        if (count) memcpy( apps + copied, response.data, response.data_size );
        copied += count;
        if (!count || count < request.arg1) break;
    }
    return copied;
}

static int32_t bridge_app_list_string(
    uint32_t method, uint32_t app_id, char *output, int32_t capacity )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!output || capacity <= 0) return 0;
    request.method = method;
    request.arg0 = app_id;
    request.arg1 = capacity;
    if (!bridge_app_list_rpc( &request, &response ) ||
        response.data_size > (uint32_t)capacity) {
        output[0] = 0;
        return 0;
    }
    uint32_t copy = response.data_size;
    if (copy) memcpy( output, response.data, copy );
    if (!copy) output[0] = 0;
    else output[copy - 1] = 0;
    return (int32_t)response.result;
}

static int32_t bridge_app_list_name(
    void *, uint32_t app_id, char *name, int32_t capacity )
{ return bridge_app_list_string( 2, app_id, name, capacity ); }

static int32_t bridge_app_list_install_dir(
    void *, uint32_t app_id, char *directory, int32_t capacity )
{ return bridge_app_list_string( 3, app_id, directory, capacity ); }

static int32_t bridge_app_list_build_id( void *, uint32_t app_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.arg0 = app_id;
    return bridge_app_list_rpc( &request, &response )
        ? (int32_t)response.result : 0;
}
