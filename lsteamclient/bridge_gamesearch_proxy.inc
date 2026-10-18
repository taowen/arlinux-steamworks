/* Bionic-side SteamMatchGameSearch001 proxy. */
static bool bridge_game_search_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_GAME_SEARCH_CALL,
                             request, response );
}

static uint32_t bridge_game_search_result(
    uint32_t method, uint64_t steam_id = 0, uint32_t arg0 = 0,
    uint32_t arg1 = 0, uint32_t arg2 = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = steam_id;
    request.arg0 = arg0;
    request.arg1 = arg1;
    request.arg2 = arg2;
    return bridge_game_search_rpc( &request, &response )
        ? response.result : 9 /* k_EGameSearchErrorCode_Failed_Unknown_Error */;
}

static uint32_t bridge_game_search_text_pair(
    uint32_t method, const char *key, const char *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!key || !value || strlen(key) >= sizeof(request.text) ||
        strlen(value) >= sizeof(request.value)) return 6;
    request.method = method;
    bridge_iface_copy_text( request.text, key );
    bridge_iface_copy_text( request.value, value );
    return bridge_game_search_rpc( &request, &response )
        ? response.result : 9;
}

static uint32_t bridge_game_search_add( void *, const char *key,
                                        const char *values )
{ return bridge_game_search_text_pair( 0, key, values ); }

static uint32_t bridge_game_search_lobby( void *, CSteamID lobby,
                                          int32_t minimum, int32_t maximum )
{
    return bridge_game_search_result( 1, bridge_iface_id_value( lobby ),
                                      minimum, maximum );
}

static uint32_t bridge_game_search_solo( void *, int32_t minimum,
                                         int32_t maximum )
{ return bridge_game_search_result( 2, 0, minimum, maximum ); }

static uint32_t bridge_game_search_accept( void * )
{ return bridge_game_search_result( 3 ); }
static uint32_t bridge_game_search_decline( void * )
{ return bridge_game_search_result( 4 ); }

static uint32_t bridge_game_search_connection(
    void *, CSteamID host, char *details, int32_t capacity )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!details || capacity <= 0 ||
        (uint32_t)capacity > sizeof(response.data)) return 6;
    request.method = 5;
    request.steam_id = bridge_iface_id_value( host );
    request.arg0 = capacity;
    if (!bridge_game_search_rpc( &request, &response )) return 9;
    if (response.data_size > (uint32_t)capacity) return 9;
    if (response.data_size)
        memcpy( details, response.data, response.data_size );
    if (!response.data_size || response.data_size == (uint32_t)capacity)
        details[capacity - 1] = 0;
    return response.result;
}

static uint32_t bridge_game_search_end_search( void * )
{ return bridge_game_search_result( 6 ); }

static uint32_t bridge_game_search_host_params( void *, const char *key,
                                                const char *value )
{ return bridge_game_search_text_pair( 7, key, value ); }

static uint32_t bridge_game_search_set_connection(
    void *, const char *details, int32_t size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (size < 0 || (size && !details) ||
        (uint32_t)size > sizeof(request.data)) return 6;
    request.method = 8;
    request.data_size = size;
    if (size) memcpy( request.data, details, size );
    return bridge_game_search_rpc( &request, &response )
        ? response.result : 9;
}

static uint32_t bridge_game_search_request_players(
    void *, int32_t minimum, int32_t maximum, int32_t team_maximum )
{
    return bridge_game_search_result( 9, 0, minimum, maximum, team_maximum );
}

static uint32_t bridge_game_search_confirm( void *, uint64_t game_id )
{ return bridge_game_search_result( 10, game_id ); }

static uint32_t bridge_game_search_cancel( void * )
{ return bridge_game_search_result( 11 ); }

static uint32_t bridge_game_search_submit(
    void *, uint64_t game_id, CSteamID player, uint32_t player_result )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    uint64_t player_id = bridge_iface_id_value( player );
    request.method = 12;
    request.steam_id = game_id;
    request.arg0 = player_result;
    request.data_size = sizeof(player_id);
    memcpy( request.data, &player_id, sizeof(player_id) );
    return bridge_game_search_rpc( &request, &response )
        ? response.result : 9;
}

static uint32_t bridge_game_search_end_game( void *, uint64_t game_id )
{ return bridge_game_search_result( 13, game_id ); }
