/* Bionic-side SteamUser021 and SteamFriends017 calls backed by native Steam. */
static uint64_t bridge_iface_id_value( CSteamID id )
{
    uint64_t value = 0;
    memcpy( &value, &id, sizeof(value) );
    return value;
}

static CSteamID bridge_iface_make_id( uint64_t value )
{
    CSteamID id = {};
    memcpy( &id, &value, sizeof(value) );
    return id;
}

static void bridge_iface_copy_text( char *dst, const char *src )
{
    snprintf( dst, LSTEAM_BRIDGE_MAX_TEXT, "%s", src ? src : "" );
}

static bool bridge_iface_rpc( uint16_t opcode,
                              lsteam_bridge_game_server_request *interface_request,
                              lsteam_bridge_game_server_response *response )
{
    static uint32_t next_sequence = 20000;
    const char *path;
    int fd;
    if (!bridge_rpc_connect( &fd, &path )) return false;
    lsteam_bridge_request request = { LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        opcode, __atomic_add_fetch( &next_sequence, 1, __ATOMIC_RELAXED ) };
    memset( response, 0, sizeof(*response) );
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, interface_request, sizeof(*interface_request),
                    MSG_NOSIGNAL ) == sizeof(*interface_request) &&
              recv( fd, response, sizeof(*response), 0 ) == sizeof(*response);
    close( fd );
    ok = ok && response->magic == LSTEAM_BRIDGE_MAGIC &&
         response->version == LSTEAM_BRIDGE_VERSION &&
         response->opcode == opcode && response->sequence == request.sequence &&
         !response->status && response->data_size <= sizeof(response->data);
    if (!ok)
        bridge_trace( "interface opcode=%u method=%u failed status=%d",
                      opcode, interface_request->method, response->status );
    return ok;
}

static uint32_t bridge_user_begin_auth( void *, const void *ticket, int32_t size,
                                        CSteamID steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (size < 0 || (size && !ticket) ||
        (uint32_t)size > LSTEAM_BRIDGE_MAX_TICKET) return 1;
    request.method = 0;
    request.steam_id = bridge_iface_id_value( steam_id );
    request.data_size = size;
    if (size) memcpy( request.data, ticket, size );
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 1;
}

static void bridge_user_end_auth( void *, CSteamID steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    request.steam_id = bridge_iface_id_value( steam_id );
    bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response );
}

static void bridge_user_cancel_ticket( void *, uint32_t ticket )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    request.arg0 = ticket;
    bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response );
}

static uint32_t bridge_user_has_license( void *, CSteamID steam_id, uint32_t appid )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.steam_id = bridge_iface_id_value( steam_id );
    request.arg0 = appid;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 2;
}

static void bridge_user_terminate_connection( void *, uint32_t ip, uint16_t port )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.arg0 = ip;
    request.arg1 = port;
    bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response );
}

static void bridge_user_advertise_game( void *, CSteamID server, uint32_t ip,
                                        uint16_t port )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 5;
    request.steam_id = bridge_iface_id_value( server );
    request.arg0 = ip;
    request.arg1 = port;
    bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response );
}

static int8_t bridge_user_is_behind_nat( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) &&
           response.result;
}

static int32_t bridge_user_badge_level( void *, int32_t series, int8_t foil )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 7;
    request.arg0 = series;
    request.arg1 = foil != 0;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 0;
}

static int32_t bridge_user_player_level( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 8;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 0;
}

static uint64_t bridge_user_request_encrypted_ticket( void *, void *data,
                                                      int32_t size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (size < 0 || (size && !data) ||
        (uint32_t)size > LSTEAM_BRIDGE_MAX_TICKET) return 0;
    request.method = 9;
    request.data_size = size;
    if (size) memcpy( request.data, data, size );
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 0;
}

static int8_t bridge_user_get_encrypted_ticket( void *, void *ticket,
                                                int32_t capacity,
                                                uint32_t *ticket_size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (ticket_size) *ticket_size = 0;
    request.method = 10;
    if (!bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ))
        return 0;
    if (ticket_size) *ticket_size = response.data_size;
    if (!response.result || !ticket || capacity < 0 ||
        response.data_size > (uint32_t)capacity) return 0;
    if (response.data_size) memcpy( ticket, response.data, response.data_size );
    return 1;
}

static void bridge_user_track_usage( void *, CGameID &game_id, int32_t event,
                                     const char *extra )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    memcpy( &request.steam_id, &game_id, sizeof(request.steam_id) );
    request.arg0 = event;
    bridge_iface_copy_text( request.text, extra );
    bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response );
}

static int8_t bridge_user_data_folder( void *, char *folder, int32_t capacity )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!folder || capacity <= 0) return 0;
    folder[0] = 0;
    request.method = 12;
    if (!bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ||
        !response.result || !response.data_size ||
        response.data_size > sizeof(response.data) ||
        response.data_size > (uint32_t)capacity)
        return 0;
    memcpy( folder, response.data, response.data_size );
    folder[capacity - 1] = 0;
    return 1;
}

static uint64_t bridge_user_store_auth_url( void *, const char *redirect )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 13;
    bridge_iface_copy_text( request.text, redirect );
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 0;
}

static int8_t bridge_user_phone_verified( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 14;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) &&
           response.result;
}

static int8_t bridge_user_two_factor( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 15;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) &&
           response.result;
}

static int8_t bridge_user_phone_identifying( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 16;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) &&
           response.result;
}

static int8_t bridge_user_phone_requiring_verification( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 17;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) &&
           response.result;
}

static uint64_t bridge_user_market_eligibility( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 18;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 0;
}

static uint64_t bridge_user_duration_control( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 19;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) ?
           response.result : 0;
}

static int8_t bridge_user_set_duration_state( void *, uint32_t state )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    request.arg0 = state;
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_CALL, &request, &response ) &&
           response.result;
}

static bool bridge_friends_rpc( lsteam_bridge_game_server_request *request,
                                lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_FRIENDS_CALL, request, response );
}

static __thread char bridge_friends_string[LSTEAM_BRIDGE_MAX_TICKET];

static const char *bridge_friends_string_result(
    lsteam_bridge_game_server_request *request, bool allow_null )
{
    lsteam_bridge_game_server_response response = {};
    if (!bridge_friends_rpc( request, &response ) ||
        response.data_size > sizeof(bridge_friends_string) ||
        (allow_null && !response.arg0))
        return allow_null ? NULL : "";
    size_t size = response.data_size;
    if (size) memcpy( bridge_friends_string, response.data, size );
    if (!size) bridge_friends_string[0] = 0;
    else bridge_friends_string[size - 1] = 0;
    return bridge_friends_string;
}

static const char *bridge_friends_persona_name( void * )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 0;
    return bridge_friends_string_result( &request, false );
}

static uint32_t bridge_friends_persona_state( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    return bridge_friends_rpc( &request, &response ) ? response.result : 0;
}

static int32_t bridge_friends_count( void *, int32_t flags )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    request.arg0 = flags;
    return bridge_friends_rpc( &request, &response ) ? response.result : 0;
}

static CSteamID bridge_friends_by_index( void *, int32_t index, int32_t flags )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.arg0 = index;
    request.arg1 = flags;
    return bridge_iface_make_id(
        bridge_friends_rpc( &request, &response ) ? response.steam_id : 0 );
}

static uint32_t bridge_friends_id_result( uint32_t method, CSteamID steam_id,
                                          uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( steam_id );
    request.arg0 = argument;
    return bridge_friends_rpc( &request, &response ) ? response.result : 0;
}

static uint32_t bridge_friends_relationship( void *, CSteamID id )
{ return bridge_friends_id_result( 4, id ); }
static uint32_t bridge_friends_friend_state( void *, CSteamID id )
{ return bridge_friends_id_result( 5, id ); }
static int32_t bridge_friends_steam_level( void *, CSteamID id )
{ return bridge_friends_id_result( 19, id ); }
static int8_t bridge_friends_has_friend( void *, CSteamID id, int32_t flags )
{ return bridge_friends_id_result( 7, id, flags ) != 0; }
static int32_t bridge_friends_small_avatar( void *, CSteamID id )
{ return bridge_friends_id_result( 14, id ); }
static int32_t bridge_friends_medium_avatar( void *, CSteamID id )
{ return bridge_friends_id_result( 15, id ); }
static int32_t bridge_friends_large_avatar( void *, CSteamID id )
{ return bridge_friends_id_result( 16, id ); }
static int32_t bridge_friends_rich_key_count( void *, CSteamID id )
{ return bridge_friends_id_result( 21, id ); }

static const char *bridge_friends_friend_name( void *, CSteamID id )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 6;
    request.steam_id = bridge_iface_id_value( id );
    return bridge_friends_string_result( &request, false );
}

static const char *bridge_friends_player_nickname( void *, CSteamID id )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 20;
    request.steam_id = bridge_iface_id_value( id );
    return bridge_friends_string_result( &request, true );
}

static int8_t bridge_friends_game_played( void *, CSteamID id,
                                          FriendGameInfo_t *info )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 23;
    request.steam_id = bridge_iface_id_value( id );
    if (!bridge_friends_rpc( &request, &response ) || !response.result ||
        response.data_size != sizeof(*info) || !info) return 0;
    memcpy( info, response.data, sizeof(*info) );
    return 1;
}

static int8_t bridge_friends_request_info( void *, CSteamID id, int8_t name_only )
{ return bridge_friends_id_result( 8, id, name_only != 0 ) != 0; }

static int8_t bridge_friends_set_rich_presence( void *, const char *key,
                                                const char *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!key) return 0;
    request.method = 9;
    request.arg0 = value != NULL;
    bridge_iface_copy_text( request.text, key );
    bridge_iface_copy_text( request.value, value );
    return bridge_friends_rpc( &request, &response ) && response.result;
}

static void bridge_friends_clear_rich_presence( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 10;
    bridge_friends_rpc( &request, &response );
}

static const char *bridge_friends_get_rich_presence( void *, CSteamID id,
                                                     const char *key )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 11;
    request.steam_id = bridge_iface_id_value( id );
    bridge_iface_copy_text( request.text, key );
    return bridge_friends_string_result( &request, false );
}

static const char *bridge_friends_rich_key_by_index( void *, CSteamID id,
                                                      int32_t index )
{
    lsteam_bridge_game_server_request request = {};
    request.method = 22;
    request.steam_id = bridge_iface_id_value( id );
    request.arg0 = index;
    return bridge_friends_string_result( &request, false );
}

static void bridge_friends_request_rich_presence( void *, CSteamID id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 12;
    request.steam_id = bridge_iface_id_value( id );
    bridge_friends_rpc( &request, &response );
}

static int8_t bridge_friends_invite( void *, CSteamID id, const char *connect )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 13;
    request.steam_id = bridge_iface_id_value( id );
    bridge_iface_copy_text( request.text, connect );
    return bridge_friends_rpc( &request, &response ) && response.result;
}

static void bridge_friends_set_played_with( void *, CSteamID id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 17;
    request.steam_id = bridge_iface_id_value( id );
    bridge_friends_rpc( &request, &response );
}

static void bridge_friends_voice_speaking( void *, CSteamID id, int8_t speaking )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 18;
    request.steam_id = bridge_iface_id_value( id );
    request.arg0 = speaking != 0;
    bridge_friends_rpc( &request, &response );
}
