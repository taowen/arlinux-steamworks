/* Bionic-side SteamGameServerStats001 proxy. */
static bool bridge_game_server_stats_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_GAME_SERVER_STATS_CALL,
                             request, response );
}

static void bridge_game_server_stats_name(
    lsteam_bridge_game_server_request *request, const char *name )
{
    if (!name) return;
    snprintf( request->text, sizeof(request->text), "%s", name );
}

static uint64_t bridge_game_server_stats_request( void *, CSteamID user )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.steam_id = bridge_iface_id_value( user );
    return bridge_game_server_stats_rpc( &request, &response ) ?
           response.result : 0;
}

static int8_t bridge_game_server_stats_get_int(
    void *, CSteamID user, const char *name, int32_t *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    request.steam_id = bridge_iface_id_value( user );
    bridge_game_server_stats_name( &request, name );
    if (!bridge_game_server_stats_rpc( &request, &response ) ||
        !response.result) return 0;
    if (value) memcpy( value, &response.arg0, sizeof(*value) );
    return 1;
}

static int8_t bridge_game_server_stats_get_float(
    void *, CSteamID user, const char *name, float *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    request.steam_id = bridge_iface_id_value( user );
    bridge_game_server_stats_name( &request, name );
    if (!bridge_game_server_stats_rpc( &request, &response ) ||
        !response.result) return 0;
    if (value) memcpy( value, &response.arg0, sizeof(*value) );
    return 1;
}

static int8_t bridge_game_server_stats_get_achievement(
    void *, CSteamID user, const char *name, int8_t *achieved )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.steam_id = bridge_iface_id_value( user );
    bridge_game_server_stats_name( &request, name );
    if (!bridge_game_server_stats_rpc( &request, &response ) ||
        !response.result) return 0;
    if (achieved) *achieved = response.arg0 != 0;
    return 1;
}

static int8_t bridge_game_server_stats_set_int(
    void *, CSteamID user, const char *name, int32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.steam_id = bridge_iface_id_value( user );
    request.arg0 = (uint32_t)value;
    bridge_game_server_stats_name( &request, name );
    return bridge_game_server_stats_rpc( &request, &response ) &&
           response.result;
}

static int8_t bridge_game_server_stats_set_float(
    void *, CSteamID user, const char *name, float value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 5;
    request.steam_id = bridge_iface_id_value( user );
    memcpy( &request.arg0, &value, sizeof(value) );
    bridge_game_server_stats_name( &request, name );
    return bridge_game_server_stats_rpc( &request, &response ) &&
           response.result;
}

static int8_t bridge_game_server_stats_update_rate(
    void *, CSteamID user, const char *name, float count, double length )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    request.steam_id = bridge_iface_id_value( user );
    memcpy( &request.arg0, &count, sizeof(count) );
    request.data_size = sizeof(length);
    memcpy( request.data, &length, sizeof(length) );
    bridge_game_server_stats_name( &request, name );
    return bridge_game_server_stats_rpc( &request, &response ) &&
           response.result;
}

static int8_t bridge_game_server_stats_named_bool(
    uint32_t method, CSteamID user, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( user );
    bridge_game_server_stats_name( &request, name );
    return bridge_game_server_stats_rpc( &request, &response ) &&
           response.result;
}

static int8_t bridge_game_server_stats_set_achievement(
    void *, CSteamID user, const char *name )
{
    return bridge_game_server_stats_named_bool( 7, user, name );
}

static int8_t bridge_game_server_stats_clear_achievement(
    void *, CSteamID user, const char *name )
{
    return bridge_game_server_stats_named_bool( 8, user, name );
}

static uint64_t bridge_game_server_stats_store( void *, CSteamID user )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 9;
    request.steam_id = bridge_iface_id_value( user );
    return bridge_game_server_stats_rpc( &request, &response ) ?
           response.result : 0;
}
