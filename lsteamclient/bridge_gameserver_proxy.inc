/* Bionic-side ISteamGameServer013 proxy. Keep signatures in vtable order. */
static uint64_t bridge_gs_id_value( CSteamID id )
{
    uint64_t value = 0;
    memcpy( &value, &id, sizeof(value) );
    return value;
}

static CSteamID bridge_gs_make_id( uint64_t value )
{
    CSteamID id = {};
    memcpy( &id, &value, sizeof(value) );
    return id;
}

static void bridge_gs_copy_text( char *dst, const char *src )
{
    snprintf( dst, LSTEAM_BRIDGE_MAX_TEXT, "%s", src ? src : "" );
}

static bool bridge_gs_rpc( lsteam_bridge_game_server_request *game,
                           lsteam_bridge_game_server_response *response )
{
    const char *path = getenv( "LSTEAM_BRIDGE_SOCKET" );
    if (!path || !*path) path = LSTEAM_BRIDGE_DEFAULT_SOCKET;
    if (strlen(path) >= sizeof(((sockaddr_un *)0)->sun_path)) return false;

    int fd = socket( AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0 );
    if (fd < 0) return false;
    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    memcpy( address.sun_path, path, strlen(path) + 1 );
    if (connect( fd, (sockaddr *)&address,
                 offsetof(sockaddr_un, sun_path) + strlen(path) + 1 ) < 0)
    {
        close( fd );
        return false;
    }

    static uint32_t next_sequence = 100;
    lsteam_bridge_request request = { LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        LSTEAM_BRIDGE_GAME_SERVER_CALL,
        __atomic_add_fetch( &next_sequence, 1, __ATOMIC_RELAXED ) };
    memset( response, 0, sizeof(*response) );
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, game, sizeof(*game), MSG_NOSIGNAL ) == sizeof(*game) &&
              recv( fd, response, sizeof(*response), 0 ) == sizeof(*response);
    close( fd );
    ok = ok && response->magic == LSTEAM_BRIDGE_MAGIC &&
         response->version == LSTEAM_BRIDGE_VERSION &&
         response->opcode == LSTEAM_BRIDGE_GAME_SERVER_CALL &&
         response->sequence == request.sequence && !response->status;
    if (!ok) bridge_trace( "GameServer method=%u failed status=%d", game->method,
                           response->status );
    return ok;
}

static uint64_t bridge_gs_result( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    return bridge_gs_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_gs_init( void *, uint32_t ip, uint16_t game_port,
                              uint16_t query_port, uint32_t flags, uint32_t appid,
                              const char *version )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = ip;
    request.arg1 = game_port;
    request.arg2 = query_port;
    request.arg3 = flags;
    request.steam_id = appid;
    bridge_gs_copy_text( request.text, version );
    int8_t ok = bridge_gs_rpc( &request, &response ) && response.result;
    bridge_trace( "GameServer Init appid=%u ports=%u/%u ok=%d", appid,
                  game_port, query_port, ok );
    return ok;
}

static void bridge_gs_string_call( uint32_t method, const char *text )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    bridge_gs_copy_text( request.text, text );
    bridge_gs_rpc( &request, &response );
}
#define BRIDGE_GS_STRING(name, slot) \
    static void name( void *, const char *text ) { bridge_gs_string_call( slot, text ); }
BRIDGE_GS_STRING(bridge_gs_set_product, 1)
BRIDGE_GS_STRING(bridge_gs_set_description, 2)
BRIDGE_GS_STRING(bridge_gs_set_mod_dir, 3)
BRIDGE_GS_STRING(bridge_gs_log_on, 5)
BRIDGE_GS_STRING(bridge_gs_set_server_name, 14)
BRIDGE_GS_STRING(bridge_gs_set_map_name, 15)
BRIDGE_GS_STRING(bridge_gs_set_spectator_name, 18)
BRIDGE_GS_STRING(bridge_gs_set_tags, 21)
BRIDGE_GS_STRING(bridge_gs_set_data, 22)
BRIDGE_GS_STRING(bridge_gs_set_region, 23)
#undef BRIDGE_GS_STRING

static void bridge_gs_simple_call( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    bridge_gs_rpc( &request, &response );
}
#define BRIDGE_GS_SIMPLE(name, slot) static void name( void * ) { bridge_gs_simple_call( slot ); }
BRIDGE_GS_SIMPLE(bridge_gs_log_on_anonymous, 6)
BRIDGE_GS_SIMPLE(bridge_gs_log_off, 7)
BRIDGE_GS_SIMPLE(bridge_gs_clear_values, 19)
BRIDGE_GS_SIMPLE(bridge_gs_gameplay_stats, 34)
BRIDGE_GS_SIMPLE(bridge_gs_force_heartbeat, 41)
#undef BRIDGE_GS_SIMPLE

#define BRIDGE_GS_BOOL(name, slot) static int8_t name( void * ) { return bridge_gs_result( slot ) != 0; }
BRIDGE_GS_BOOL(bridge_gs_logged_on, 8)
BRIDGE_GS_BOOL(bridge_gs_secure, 9)
BRIDGE_GS_BOOL(bridge_gs_restart_requested, 11)
#undef BRIDGE_GS_BOOL

static CSteamID bridge_gs_id_result( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    return bridge_gs_make_id( bridge_gs_rpc( &request, &response ) ? response.steam_id : 0 );
}
static CSteamID bridge_gs_get_steamid( void * ) { return bridge_gs_id_result( 10 ); }
static CSteamID bridge_gs_create_unauthenticated( void * ) { return bridge_gs_id_result( 25 ); }

static void bridge_gs_u32_call( uint32_t method, uint32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = value;
    bridge_gs_rpc( &request, &response );
}
#define BRIDGE_GS_U32(name, slot, type) \
    static void name( void *, type value ) { bridge_gs_u32_call( slot, (uint32_t)value ); }
BRIDGE_GS_U32(bridge_gs_set_dedicated, 4, int8_t)
BRIDGE_GS_U32(bridge_gs_set_max_players, 12, int32_t)
BRIDGE_GS_U32(bridge_gs_set_bot_players, 13, int32_t)
BRIDGE_GS_U32(bridge_gs_set_password, 16, int8_t)
BRIDGE_GS_U32(bridge_gs_set_spectator_port, 17, uint16_t)
BRIDGE_GS_U32(bridge_gs_cancel_ticket, 31, uint32_t)
BRIDGE_GS_U32(bridge_gs_enable_heartbeats, 39, int8_t)
BRIDGE_GS_U32(bridge_gs_set_heartbeat_interval, 40, int32_t)
#undef BRIDGE_GS_U32

static void bridge_gs_set_key_value( void *, const char *key, const char *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    bridge_gs_copy_text( request.text, key );
    bridge_gs_copy_text( request.value, value );
    bridge_gs_rpc( &request, &response );
}

static int8_t bridge_gs_connect_user( void *, uint32_t ip, const void *blob,
                                      uint32_t size, CSteamID *steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!blob || size > LSTEAM_BRIDGE_MAX_TICKET) return 0;
    request.method = 24;
    request.arg0 = ip;
    request.data_size = size;
    if (size) memcpy( request.data, blob, size );
    int8_t ok = bridge_gs_rpc( &request, &response ) && response.result;
    if (steam_id) *steam_id = bridge_gs_make_id( response.steam_id );
    return ok;
}

static void bridge_gs_id_void( uint32_t method, CSteamID steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_gs_id_value( steam_id );
    bridge_gs_rpc( &request, &response );
}
static void bridge_gs_disconnect_user( void *, CSteamID id ) { bridge_gs_id_void( 26, id ); }
static void bridge_gs_end_auth( void *, CSteamID id ) { bridge_gs_id_void( 30, id ); }

static int8_t bridge_gs_update_user( void *, CSteamID id, const char *name, uint32_t score )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 27;
    request.steam_id = bridge_gs_id_value( id );
    request.arg0 = score;
    bridge_gs_copy_text( request.text, name );
    return bridge_gs_rpc( &request, &response ) && response.result;
}

static uint32_t bridge_gs_begin_auth( void *, const void *blob, int32_t size, CSteamID id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!blob || size < 0 || (uint32_t)size > LSTEAM_BRIDGE_MAX_TICKET) return 2;
    request.method = 29;
    request.steam_id = bridge_gs_id_value( id );
    request.data_size = size;
    if (size) memcpy( request.data, blob, size );
    return bridge_gs_rpc( &request, &response ) ? response.result : 2;
}

static uint32_t bridge_gs_get_auth_ticket( void *, void *ticket, int32_t maximum,
                                           uint32_t *ticket_size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (ticket_size) *ticket_size = 0;
    if (!ticket || maximum <= 0) return 0;
    request.method = 28;
    if (!bridge_gs_rpc( &request, &response ) ||
        response.data_size > (uint32_t)maximum) return 0;
    if (response.data_size) memcpy( ticket, response.data, response.data_size );
    if (ticket_size) *ticket_size = response.data_size;
    return response.result;
}

/* SteamGameServer015 adds a networking identity argument. The native bridge
 * intentionally stays on the stable 013 ABI, so ignore the optional identity. */
static uint32_t bridge_gs_get_auth_ticket15( void *iface, void *ticket,
                                             int32_t maximum,
                                             uint32_t *ticket_size,
                                             const void *networking_identity )
{
    (void)networking_identity;
    return bridge_gs_get_auth_ticket( iface, ticket, maximum, ticket_size );
}

static void bridge_gs_set_advertise( void *, int8_t active )
{
    bridge_gs_u32_call( 39, active != 0 );
}

static uint32_t bridge_gs_has_license( void *, CSteamID id, uint32_t appid )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 32;
    request.steam_id = bridge_gs_id_value( id );
    request.arg0 = appid;
    return bridge_gs_rpc( &request, &response ) ? response.result : 2;
}

static int8_t bridge_gs_group_status( void *, CSteamID user, CSteamID group )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    uint64_t group_value = bridge_gs_id_value( group );
    request.method = 33;
    request.steam_id = bridge_gs_id_value( user );
    request.data_size = sizeof(group_value);
    memcpy( request.data, &group_value, sizeof(group_value) );
    return bridge_gs_rpc( &request, &response ) && response.result;
}

static SteamIPAddress_t bridge_gs_public_ip( void * )
{
    SteamIPAddress_t address = {};
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 36;
    if (bridge_gs_rpc( &request, &response ) &&
        response.data_size == sizeof(address))
        memcpy( &address, response.data, sizeof(address) );
    return address;
}

static uint32_t bridge_gs_public_ip_v4( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 36;
    return bridge_gs_rpc( &request, &response ) ? response.arg0 : 0;
}

static uint64_t bridge_gs_async_call( uint32_t method, uint64_t steam_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = steam_id;
    return bridge_gs_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_gs_reputation( void * )
{
    return bridge_gs_async_call( 35, 0 );
}

static uint64_t bridge_gs_associate_clan( void *, CSteamID clan )
{
    return bridge_gs_async_call( 42, bridge_gs_id_value( clan ) );
}

static uint64_t bridge_gs_compute_compatibility( void *, CSteamID player )
{
    return bridge_gs_async_call( 43, bridge_gs_id_value( player ) );
}

static int8_t bridge_gs_incoming_packet( void *, const void *data, int32_t size,
                                         uint32_t ip, uint16_t port )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!data || size < 0 || (uint32_t)size > LSTEAM_BRIDGE_MAX_TICKET) return 0;
    request.method = 37;
    request.arg0 = ip;
    request.arg1 = port;
    request.data_size = size;
    if (size) memcpy( request.data, data, size );
    return bridge_gs_rpc( &request, &response ) && response.result;
}

static int32_t bridge_gs_outgoing_packet( void *, void *data, int32_t maximum,
                                          uint32_t *ip, uint16_t *port )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!data || maximum <= 0) return 0;
    request.method = 38;
    if (!bridge_gs_rpc( &request, &response ) ||
        response.data_size > (uint32_t)maximum) return 0;
    memcpy( data, response.data, response.data_size );
    if (ip) *ip = response.arg0;
    if (port) *port = response.arg1;
    return response.data_size;
}
