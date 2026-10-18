/* Bionic-side SteamNetworkingUtils003/004 proxy.  Debug callbacks and pointer
 * config values cannot safely cross the process/ABI boundary; scalar and
 * string config, identity/address conversion, ping data and allocation do. */
static_assert( sizeof(SteamNetworkingIdentity_144) == 136,
               "unexpected bridge identity ABI" );
static_assert( sizeof(SteamNetworkingIPAddr) == 18,
               "unexpected bridge IP address ABI" );
static_assert( sizeof(SteamNetworkPingLocation_t) <= LSTEAM_BRIDGE_MAX_TICKET,
               "ping location does not fit bridge packet" );
static_assert( sizeof(SteamRelayNetworkStatus_t) <= LSTEAM_BRIDGE_MAX_TICKET,
               "relay status does not fit bridge packet" );

enum bridge_network_config_type
{
    BRIDGE_NETCONFIG_INT32 = 1,
    BRIDGE_NETCONFIG_INT64 = 2,
    BRIDGE_NETCONFIG_FLOAT = 3,
    BRIDGE_NETCONFIG_STRING = 4,
    BRIDGE_NETCONFIG_PTR = 5,
};

static bool bridge_network_utils_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_NETWORK_UTILS_CALL,
                             request, response );
}

static void U_CDECL bridge_network_utils_free4(
    u_SteamNetworkingMessage_t_153a *message )
{
    if (!message) return;
    free( message->m_pData );
    message->m_pData = NULL;
}

static void U_CDECL bridge_network_utils_release4(
    u_SteamNetworkingMessage_t_153a *message )
{
    if (!message) return;
    if (message->m_pfnFreeData) message->m_pfnFreeData( message );
    free( message );
}

static u_SteamNetworkingMessage_t_153a *bridge_network_utils_allocate4(
    void *, int32_t size )
{
    if (size < 0 || (uint32_t)size > 16u * 1024u * 1024u) return NULL;
    u_SteamNetworkingMessage_t_153a *message =
        (u_SteamNetworkingMessage_t_153a *)calloc( 1, sizeof(*message) );
    if (!message) return NULL;
    if (size && !(message->m_pData = calloc( 1, size ))) {
        free( message );
        return NULL;
    }
    message->m_cbSize = size;
    message->m_pfnFreeData = bridge_network_utils_free4;
    message->m_pfnRelease = bridge_network_utils_release4;
    return message;
}

static void U_CDECL bridge_network_utils_free3(
    u_SteamNetworkingMessage_t_147 *message )
{
    if (!message) return;
    free( message->m_pData );
    message->m_pData = NULL;
}

static void U_CDECL bridge_network_utils_release3(
    u_SteamNetworkingMessage_t_147 *message )
{
    if (!message) return;
    if (message->m_pfnFreeData) message->m_pfnFreeData( message );
    free( message );
}

static u_SteamNetworkingMessage_t_147 *bridge_network_utils_allocate3(
    void *, int32_t size )
{
    if (size < 0 || (uint32_t)size > 16u * 1024u * 1024u) return NULL;
    u_SteamNetworkingMessage_t_147 *message =
        (u_SteamNetworkingMessage_t_147 *)calloc( 1, sizeof(*message) );
    if (!message) return NULL;
    if (size && !(message->m_pData = calloc( 1, size ))) {
        free( message );
        return NULL;
    }
    message->m_cbSize = size;
    message->m_pfnFreeData = bridge_network_utils_free3;
    message->m_pfnRelease = bridge_network_utils_release3;
    return message;
}

static uint32_t bridge_network_utils_relay_status(
    void *, SteamRelayNetworkStatus_t *details )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = details != NULL;
    if (!bridge_network_utils_rpc( &request, &response )) return 0;
    if (details) {
        if (response.data_size != sizeof(*details)) return 0;
        memcpy( details, response.data, sizeof(*details) );
    }
    return response.result;
}

static float bridge_network_utils_local_ping(
    void *, SteamNetworkPingLocation_t *location )
{
    if (!location) return -1.0f;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    if (!bridge_network_utils_rpc( &request, &response ) ||
        response.data_size != sizeof(*location)) return -1.0f;
    memcpy( location, response.data, sizeof(*location) );
    float age = -1.0f;
    memcpy( &age, &response.arg0, sizeof(age) );
    return age;
}

static int32_t bridge_network_utils_estimate_two(
    void *, const SteamNetworkPingLocation_t *first,
    const SteamNetworkPingLocation_t *second )
{
    if (!first || !second) return -1;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    request.data_size = sizeof(*first) + sizeof(*second);
    memcpy( request.data, first, sizeof(*first) );
    memcpy( request.data + sizeof(*first), second, sizeof(*second) );
    return bridge_network_utils_rpc( &request, &response )
        ? (int32_t)(int64_t)response.result : -1;
}

static int32_t bridge_network_utils_estimate_local(
    void *, const SteamNetworkPingLocation_t *location )
{
    if (!location) return -1;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.data_size = sizeof(*location);
    memcpy( request.data, location, sizeof(*location) );
    return bridge_network_utils_rpc( &request, &response )
        ? (int32_t)(int64_t)response.result : -1;
}

static void bridge_network_utils_ping_to_string(
    void *, const SteamNetworkPingLocation_t *location, char *buffer,
    int32_t capacity )
{
    if (!location || !buffer || capacity <= 0) return;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.arg0 = capacity;
    request.data_size = sizeof(*location);
    memcpy( request.data, location, sizeof(*location) );
    if (!bridge_network_utils_rpc( &request, &response )) {
        buffer[0] = 0;
        return;
    }
    size_t copied = std::min<size_t>(
        response.data_size, (size_t)capacity );
    if (copied) memcpy( buffer, response.data, copied );
    buffer[capacity - 1] = 0;
}

static int8_t bridge_network_utils_parse_ping(
    void *, const char *text, SteamNetworkPingLocation_t *location )
{
    if (!text || !location) return 0;
    size_t size = strnlen( text, LSTEAM_BRIDGE_MAX_TICKET );
    if (size == LSTEAM_BRIDGE_MAX_TICKET) return 0;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 5;
    request.data_size = size + 1;
    memcpy( request.data, text, size + 1 );
    if (!bridge_network_utils_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*location)) return 0;
    memcpy( location, response.data, sizeof(*location) );
    return 1;
}

static int8_t bridge_network_utils_check_ping(void *, float max_age)
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    memcpy( &request.arg0, &max_age, sizeof(max_age) );
    return bridge_network_utils_rpc( &request, &response ) &&
           response.result;
}

static int32_t bridge_network_utils_ping_dc(
    void *, uint32_t pop, uint32_t *via_relay )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 7;
    request.arg0 = pop;
    request.arg1 = via_relay != NULL;
    if (!bridge_network_utils_rpc( &request, &response )) return -1;
    if (via_relay) *via_relay = response.arg0;
    return (int32_t)(int64_t)response.result;
}

static int32_t bridge_network_utils_direct_ping(void *, uint32_t pop)
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 8;
    request.arg0 = pop;
    return bridge_network_utils_rpc( &request, &response )
        ? (int32_t)(int64_t)response.result : -1;
}

static int32_t bridge_network_utils_pop_count(void *)
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 9;
    return bridge_network_utils_rpc( &request, &response )
        ? (int32_t)(int64_t)response.result : 0;
}

static int32_t bridge_network_utils_pop_list(
    void *, uint32_t *list, int32_t capacity )
{
    if (capacity < 0 || (capacity && !list)) return 0;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 10;
    request.arg0 = std::min<uint32_t>(
        capacity, sizeof(response.data) / sizeof(uint32_t) );
    if (!bridge_network_utils_rpc( &request, &response )) return 0;
    uint32_t count = response.data_size / sizeof(uint32_t);
    if (count) memcpy( list, response.data, count * sizeof(uint32_t) );
    return (int32_t)(int64_t)response.result;
}

static int64_t bridge_network_utils_timestamp(void *)
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    return bridge_network_utils_rpc( &request, &response )
        ? (int64_t)response.result : 0;
}

static void bridge_network_utils_debug(
    void *, uint32_t detail, void (U_CDECL *)(uint32_t, const char *) )
{
    bridge_trace( "SteamNetworkingUtils SetDebugOutputFunction level=%u ignored",
                  detail );
}

static uint32_t bridge_network_utils_fake_ipv4(void *, uint32_t ipv4)
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 12;
    request.arg0 = ipv4;
    return bridge_network_utils_rpc( &request, &response )
        ? response.result : 0;
}

static uint32_t bridge_network_utils_real_identity(
    void *, const SteamNetworkingIPAddr *address,
    SteamNetworkingIdentity_144 *identity )
{
    if (!address || !identity) return 8;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 13;
    request.data_size = sizeof(*address);
    memcpy( request.data, address, sizeof(*address) );
    if (!bridge_network_utils_rpc( &request, &response )) return 2;
    if (response.result == 1 && response.data_size == sizeof(*identity))
        memcpy( identity, response.data, sizeof(*identity) );
    return response.result;
}

static int8_t bridge_network_utils_set_config(
    void *, uint32_t value, uint32_t scope, int32_t scope_object,
    uint32_t type, const void *data )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 14;
    request.arg0 = value;
    request.arg1 = scope;
    request.arg2 = scope_object;
    request.arg3 = type;
    if (data) {
        switch (type) {
        case BRIDGE_NETCONFIG_INT32:
        case BRIDGE_NETCONFIG_FLOAT:
            request.data_size = sizeof(uint32_t);
            break;
        case BRIDGE_NETCONFIG_INT64:
            request.data_size = sizeof(uint64_t);
            break;
        case BRIDGE_NETCONFIG_STRING: {
            size_t size = strnlen(
                (const char *)data, sizeof(request.data) );
            if (size == sizeof(request.data)) return 0;
            request.data_size = size + 1;
            break;
        }
        case BRIDGE_NETCONFIG_PTR:
        default:
            return 0;
        }
        memcpy( request.data, data, request.data_size );
    }
    return bridge_network_utils_rpc( &request, &response ) &&
           response.result;
}

static uint32_t bridge_network_utils_get_config(
    void *, uint32_t value, uint32_t scope, int32_t scope_object,
    uint32_t *type, void *output, uint32_t *size )
{
    if (!size) return 0;
    uint32_t capacity = *size;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 15;
    request.arg0 = value;
    request.arg1 = scope;
    request.arg2 = scope_object;
    request.arg3 = capacity;
    request.steam_id = output != NULL;
    if (!bridge_network_utils_rpc( &request, &response )) return 0;
    if (type) *type = response.arg0;
    *size = response.arg1;
    if (output && response.data_size &&
        response.data_size <= capacity)
        memcpy( output, response.data, response.data_size );
    return (uint32_t)response.result;
}

static const char *bridge_network_utils_config_info4(
    void *, uint32_t value, uint32_t *type, uint32_t *scope )
{
    static __thread char name[LSTEAM_BRIDGE_MAX_TICKET];
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 16;
    request.arg0 = value;
    request.arg1 = (type ? 1 : 0) | (scope ? 2 : 0);
    if (!bridge_network_utils_rpc( &request, &response ) ||
        !response.result || !response.data_size) return NULL;
    if (type) *type = response.arg0;
    if (scope) *scope = response.arg1;
    size_t copied = std::min<size_t>(
        response.data_size, sizeof(name) );
    memcpy( name, response.data, copied );
    name[sizeof(name) - 1] = 0;
    return name;
}

static uint32_t bridge_network_utils_iterate(
    void *, uint32_t current, int8_t include_dev )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 17;
    request.arg0 = current;
    request.arg1 = include_dev != 0;
    return bridge_network_utils_rpc( &request, &response )
        ? response.result : 0;
}

static int8_t bridge_network_utils_config_info3(
    void *iface, uint32_t value, const char **name, uint32_t *type,
    uint32_t *scope, uint32_t *next )
{
    const char *result =
        bridge_network_utils_config_info4( iface, value, type, scope );
    if (!result) return 0;
    if (name) *name = result;
    if (next) *next = bridge_network_utils_iterate( iface, value, 1 );
    return 1;
}

static uint32_t bridge_network_utils_first_config(void *iface)
{
    return bridge_network_utils_iterate(
        iface, 0 /* k_ESteamNetworkingConfig_Invalid */, 1 );
}

static void bridge_network_utils_ip_to_string(
    void *, const SteamNetworkingIPAddr *address, char *buffer,
    uint32_t capacity, int8_t with_port )
{
    if (!address || !buffer || !capacity) return;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 18;
    request.arg0 = capacity;
    request.arg1 = with_port != 0;
    request.data_size = sizeof(*address);
    memcpy( request.data, address, sizeof(*address) );
    if (!bridge_network_utils_rpc( &request, &response )) {
        buffer[0] = 0;
        return;
    }
    size_t copied = std::min<size_t>( response.data_size, capacity );
    if (copied) memcpy( buffer, response.data, copied );
    buffer[capacity - 1] = 0;
}

static int8_t bridge_network_utils_ip_parse(
    void *, SteamNetworkingIPAddr *address, const char *text )
{
    if (!address || !text) return 0;
    size_t size = strnlen( text, LSTEAM_BRIDGE_MAX_TICKET );
    if (size == LSTEAM_BRIDGE_MAX_TICKET) return 0;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 19;
    request.data_size = size + 1;
    memcpy( request.data, text, size + 1 );
    if (!bridge_network_utils_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*address)) return 0;
    memcpy( address, response.data, sizeof(*address) );
    return 1;
}

static uint32_t bridge_network_utils_ip_fake_type(
    void *, const SteamNetworkingIPAddr *address )
{
    if (!address) return 0;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    request.data_size = sizeof(*address);
    memcpy( request.data, address, sizeof(*address) );
    return bridge_network_utils_rpc( &request, &response )
        ? response.result : 0;
}

static void bridge_network_utils_identity_to_string(
    void *, const SteamNetworkingIdentity_144 *identity, char *buffer,
    uint32_t capacity )
{
    if (!identity || !buffer || !capacity) return;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 21;
    request.arg0 = capacity;
    request.data_size = sizeof(*identity);
    memcpy( request.data, identity, sizeof(*identity) );
    if (!bridge_network_utils_rpc( &request, &response )) {
        buffer[0] = 0;
        return;
    }
    size_t copied = std::min<size_t>( response.data_size, capacity );
    if (copied) memcpy( buffer, response.data, copied );
    buffer[capacity - 1] = 0;
}

static int8_t bridge_network_utils_identity_parse(
    void *, SteamNetworkingIdentity_144 *identity, const char *text )
{
    if (!identity || !text) return 0;
    size_t size = strnlen( text, LSTEAM_BRIDGE_MAX_TICKET );
    if (size == LSTEAM_BRIDGE_MAX_TICKET) return 0;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 22;
    request.data_size = size + 1;
    memcpy( request.data, text, size + 1 );
    if (!bridge_network_utils_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*identity)) return 0;
    memcpy( identity, response.data, sizeof(*identity) );
    return 1;
}
