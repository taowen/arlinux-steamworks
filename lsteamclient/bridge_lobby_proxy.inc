/* Bionic-side SteamMatchMaking009 lobby and favorites proxy. */
static bool bridge_lobby_rpc( lsteam_bridge_game_server_request *request,
                              lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_LOBBY_CALL, request, response );
}

static void bridge_lobby_second_id( lsteam_bridge_game_server_request *request,
                                    CSteamID id )
{
    uint64_t value = bridge_iface_id_value( id );
    request->data_size = sizeof(value);
    memcpy( request->data, &value, sizeof(value) );
}

static __thread char bridge_lobby_string[LSTEAM_BRIDGE_MAX_TICKET];

static const char *bridge_lobby_string_result(
    lsteam_bridge_game_server_request *request )
{
    lsteam_bridge_game_server_response response = {};
    if (!bridge_lobby_rpc( request, &response ) ||
        response.data_size > sizeof(bridge_lobby_string)) return "";
    if (response.data_size)
        memcpy( bridge_lobby_string, response.data, response.data_size );
    if (!response.data_size) bridge_lobby_string[0] = 0;
    else bridge_lobby_string[response.data_size - 1] = 0;
    return bridge_lobby_string;
}

static int32_t bridge_lobby_favorite_count( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    return bridge_lobby_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_lobby_get_favorite(
    void *, int32_t index, uint32_t *appid, uint32_t *ip,
    uint16_t *connection_port, uint16_t *query_port,
    uint32_t *flags, uint32_t *last_played )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    request.arg0 = index;
    if (!bridge_lobby_rpc( &request, &response ) || !response.result ||
        response.data_size != 12) return 0;
    if (appid) *appid = response.arg0;
    if (ip) *ip = response.arg1;
    if (connection_port) memcpy( connection_port, response.data, 2 );
    if (query_port) memcpy( query_port, response.data + 2, 2 );
    if (flags) memcpy( flags, response.data + 4, 4 );
    if (last_played) memcpy( last_played, response.data + 8, 4 );
    return 1;
}

static int32_t bridge_lobby_add_favorite(
    void *, uint32_t appid, uint32_t ip, uint16_t connection_port,
    uint16_t query_port, uint32_t flags, uint32_t last_played )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    request.arg0 = appid;
    request.arg1 = ip;
    request.arg2 = connection_port | ((uint32_t)query_port << 16);
    request.arg3 = flags;
    request.steam_id = last_played;
    return bridge_lobby_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_lobby_remove_favorite(
    void *, uint32_t appid, uint32_t ip, uint16_t connection_port,
    uint16_t query_port, uint32_t flags )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.arg0 = appid;
    request.arg1 = ip;
    request.arg2 = connection_port | ((uint32_t)query_port << 16);
    request.arg3 = flags;
    return bridge_lobby_rpc( &request, &response ) && response.result;
}

static uint64_t bridge_lobby_async( uint32_t method, CSteamID lobby = CSteamID() )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( lobby );
    return bridge_lobby_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_lobby_request_list( void * )
{ return bridge_lobby_async( 4 ); }
static uint64_t bridge_lobby_join( void *, CSteamID lobby )
{ return bridge_lobby_async( 14, lobby ); }

static void bridge_lobby_string_filter( void *, const char *key,
                                        const char *value, uint32_t comparison )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 5;
    request.arg0 = comparison;
    bridge_iface_copy_text( request.text, key );
    bridge_iface_copy_text( request.value, value );
    bridge_lobby_rpc( &request, &response );
}

static void bridge_lobby_numerical_filter( void *, const char *key,
                                           int32_t value, uint32_t comparison )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    request.arg0 = value;
    request.arg1 = comparison;
    bridge_iface_copy_text( request.text, key );
    bridge_lobby_rpc( &request, &response );
}

static void bridge_lobby_near_filter( void *, const char *key, int32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 7;
    request.arg0 = value;
    bridge_iface_copy_text( request.text, key );
    bridge_lobby_rpc( &request, &response );
}

static void bridge_lobby_u32_void( uint32_t method, uint32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = value;
    bridge_lobby_rpc( &request, &response );
}
static void bridge_lobby_slots_filter( void *, int32_t value )
{ bridge_lobby_u32_void( 8, value ); }
static void bridge_lobby_distance_filter( void *, uint32_t value )
{ bridge_lobby_u32_void( 9, value ); }
static void bridge_lobby_result_filter( void *, int32_t value )
{ bridge_lobby_u32_void( 10, value ); }

static void bridge_lobby_compatible_filter( void *, CSteamID id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    request.steam_id = bridge_iface_id_value( id );
    bridge_lobby_rpc( &request, &response );
}

static CSteamID bridge_lobby_id_result( uint32_t method, CSteamID lobby,
                                        uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( lobby );
    request.arg0 = argument;
    return bridge_iface_make_id(
        bridge_lobby_rpc( &request, &response ) ? response.steam_id : 0 );
}

static CSteamID bridge_lobby_by_index( void *, int32_t index )
{ return bridge_lobby_id_result( 12, CSteamID(), index ); }

static uint64_t bridge_lobby_create( void *, uint32_t type, int32_t maximum )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 13;
    request.arg0 = type;
    request.arg1 = maximum;
    return bridge_lobby_rpc( &request, &response ) ? response.result : 0;
}

static void bridge_lobby_leave( void *, CSteamID lobby )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 15;
    request.steam_id = bridge_iface_id_value( lobby );
    bridge_lobby_rpc( &request, &response );
}

static int8_t bridge_lobby_two_id_bool( uint32_t method, CSteamID first,
                                       CSteamID second )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( first );
    bridge_lobby_second_id( &request, second );
    return bridge_lobby_rpc( &request, &response ) && response.result;
}

static int8_t bridge_lobby_invite( void *, CSteamID lobby, CSteamID friend_id )
{ return bridge_lobby_two_id_bool( 16, lobby, friend_id ); }

static int32_t bridge_lobby_id_int( uint32_t method, CSteamID lobby )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( lobby );
    return bridge_lobby_rpc( &request, &response ) ? response.result : 0;
}
static int32_t bridge_lobby_member_count( void *, CSteamID lobby )
{ return bridge_lobby_id_int( 17, lobby ); }
static int32_t bridge_lobby_data_count( void *, CSteamID lobby )
{ return bridge_lobby_id_int( 21, lobby ); }
static int32_t bridge_lobby_member_limit( void *, CSteamID lobby )
{ return bridge_lobby_id_int( 32, lobby ); }

static CSteamID bridge_lobby_member_by_index( void *, CSteamID lobby,
                                              int32_t index )
{ return bridge_lobby_id_result( 18, lobby, index ); }
static CSteamID bridge_lobby_owner( void *, CSteamID lobby )
{ return bridge_lobby_id_result( 35, lobby ); }

static const char *bridge_lobby_get_data( void *, CSteamID lobby,
                                          const char *key )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 19;
    request.steam_id = bridge_iface_id_value( lobby );
    bridge_iface_copy_text( request.text, key );
    return bridge_lobby_string_result( &request );
}

static int8_t bridge_lobby_set_data( void *, CSteamID lobby, const char *key,
                                     const char *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    request.steam_id = bridge_iface_id_value( lobby );
    bridge_iface_copy_text( request.text, key );
    bridge_iface_copy_text( request.value, value );
    return bridge_lobby_rpc( &request, &response ) && response.result;
}

static int8_t bridge_lobby_data_by_index( void *, CSteamID lobby, int32_t index,
                                          char *key, int32_t key_capacity,
                                          char *value, int32_t value_capacity )
{
    constexpr size_t half = LSTEAM_BRIDGE_MAX_TICKET / 2;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 22;
    request.steam_id = bridge_iface_id_value( lobby );
    request.arg0 = index;
    if (!key || !value || key_capacity <= 0 || value_capacity <= 0 ||
        !bridge_lobby_rpc( &request, &response ) || !response.result ||
        response.data_size != LSTEAM_BRIDGE_MAX_TICKET) return 0;
    snprintf( key, key_capacity, "%.*s", (int)half - 1,
              (const char *)response.data );
    snprintf( value, value_capacity, "%.*s", (int)half - 1,
              (const char *)response.data + half );
    return 1;
}

static int8_t bridge_lobby_delete_data( void *, CSteamID lobby, const char *key )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 23;
    request.steam_id = bridge_iface_id_value( lobby );
    bridge_iface_copy_text( request.text, key );
    return bridge_lobby_rpc( &request, &response ) && response.result;
}

static const char *bridge_lobby_member_data( void *, CSteamID lobby,
                                             CSteamID member, const char *key )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 24;
    request.steam_id = bridge_iface_id_value( lobby );
    bridge_lobby_second_id( &request, member );
    bridge_iface_copy_text( request.text, key );
    return bridge_lobby_string_result( &request );
}

static void bridge_lobby_set_member_data( void *, CSteamID lobby,
                                          const char *key, const char *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 25;
    request.steam_id = bridge_iface_id_value( lobby );
    bridge_iface_copy_text( request.text, key );
    bridge_iface_copy_text( request.value, value );
    bridge_lobby_rpc( &request, &response );
}

static int8_t bridge_lobby_send_chat( void *, CSteamID lobby, const void *data,
                                      int32_t size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (size < 0 || (size && !data) ||
        (uint32_t)size > LSTEAM_BRIDGE_MAX_TICKET) return 0;
    request.method = 26;
    request.steam_id = bridge_iface_id_value( lobby );
    request.data_size = size;
    if (size) memcpy( request.data, data, size );
    return bridge_lobby_rpc( &request, &response ) && response.result;
}

static int32_t bridge_lobby_get_chat( void *, CSteamID lobby, int32_t index,
                                      CSteamID *sender, void *data,
                                      int32_t capacity, uint32_t *entry_type )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!data || capacity <= 0) return 0;
    request.method = 27;
    request.steam_id = bridge_iface_id_value( lobby );
    request.arg0 = index;
    if (!bridge_lobby_rpc( &request, &response ) ||
        response.data_size > (uint32_t)capacity) return 0;
    if (response.data_size) memcpy( data, response.data, response.data_size );
    if (sender) *sender = bridge_iface_make_id( response.steam_id );
    if (entry_type) *entry_type = response.arg0;
    return response.result;
}

static int8_t bridge_lobby_request_data( void *, CSteamID lobby )
{ return bridge_lobby_id_int( 28, lobby ) != 0; }

static void bridge_lobby_set_game_server( void *, CSteamID lobby, uint32_t ip,
                                          uint16_t port, CSteamID server )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 29;
    request.steam_id = bridge_iface_id_value( lobby );
    request.arg0 = ip;
    request.arg1 = port;
    bridge_lobby_second_id( &request, server );
    bridge_lobby_rpc( &request, &response );
}

static int8_t bridge_lobby_get_game_server( void *, CSteamID lobby, uint32_t *ip,
                                            uint16_t *port, CSteamID *server )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 30;
    request.steam_id = bridge_iface_id_value( lobby );
    if (!bridge_lobby_rpc( &request, &response ) || !response.result) return 0;
    if (ip) *ip = response.arg0;
    if (port) *port = response.arg1;
    if (server) *server = bridge_iface_make_id( response.steam_id );
    return 1;
}

static int8_t bridge_lobby_set_u32( uint32_t method, CSteamID lobby,
                                    uint32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( lobby );
    request.arg0 = value;
    return bridge_lobby_rpc( &request, &response ) && response.result;
}
static int8_t bridge_lobby_set_limit( void *, CSteamID lobby, int32_t limit )
{ return bridge_lobby_set_u32( 31, lobby, limit ); }
static int8_t bridge_lobby_set_type( void *, CSteamID lobby, uint32_t type )
{ return bridge_lobby_set_u32( 33, lobby, type ); }
static int8_t bridge_lobby_set_joinable( void *, CSteamID lobby, int8_t joinable )
{ return bridge_lobby_set_u32( 34, lobby, joinable != 0 ); }
static int8_t bridge_lobby_set_owner( void *, CSteamID lobby, CSteamID owner )
{ return bridge_lobby_two_id_bool( 36, lobby, owner ); }
static int8_t bridge_lobby_set_linked( void *, CSteamID lobby, CSteamID linked )
{ return bridge_lobby_two_id_bool( 37, lobby, linked ); }
