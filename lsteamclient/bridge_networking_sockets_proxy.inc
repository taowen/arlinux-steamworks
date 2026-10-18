/* Bionic-side SteamNetworkingSockets012/013 proxy. */
static_assert( sizeof(SteamNetworkingIdentity_144) == 136,
               "unexpected bridge identity ABI" );
static_assert( sizeof(SteamNetworkingIPAddr) == 18,
               "unexpected bridge IP address ABI" );

static bool bridge_network_sockets_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_NETWORK_SOCKETS_CALL,
                             request, response );
}

static int8_t bridge_network_sockets_pair(
    void *, uint32_t *first, uint32_t *second, int8_t network_loopback,
    const SteamNetworkingIdentity_144 *identity1,
    const SteamNetworkingIdentity_144 *identity2 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!first || !second) return 0;
    request.method = 0;
    request.arg0 = network_loopback != 0;
    request.arg1 = (identity1 ? 1 : 0) | (identity2 ? 2 : 0);
    if (identity1) {
        memcpy( request.data + request.data_size, identity1,
                sizeof(*identity1) );
        request.data_size += sizeof(*identity1);
    }
    if (identity2) {
        memcpy( request.data + request.data_size, identity2,
                sizeof(*identity2) );
        request.data_size += sizeof(*identity2);
    }
    if (!bridge_network_sockets_rpc( &request, &response ) ||
        !response.result) return 0;
    *first = response.arg0;
    *second = response.arg1;
    return 1;
}

static uint32_t bridge_network_sockets_send(
    void *, uint32_t connection, const void *data, uint32_t size,
    int32_t flags, int64_t *message_number )
{
    if ((size && !data) || size > 16u * 1024u * 1024u)
        return 8; /* k_EResultInvalidParam */
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        uint32_t chunk = size - offset;
        if (chunk > sizeof(request.data)) chunk = sizeof(request.data);
        request.method = 1;
        request.arg0 = connection;
        request.arg1 = flags;
        request.arg2 = size;
        request.arg3 = offset;
        request.data_size = chunk;
        if (chunk)
            memcpy( request.data, (const uint8_t *)data + offset, chunk );
        if (!bridge_network_sockets_rpc( &request, &response )) return 2;
        offset += chunk;
        if (offset == size) {
            if (message_number)
                *message_number = (int64_t)response.steam_id;
            return response.result;
        }
    } while (offset < size);
    return 2;
}

static void U_CDECL bridge_network_sockets_release(
    u_SteamNetworkingMessage_t_153a *message )
{
    if (!message) return;
    free( message->m_pData );
    free( message );
}

static u_SteamNetworkingMessage_t_153a *bridge_network_sockets_read_one(
    uint32_t handle, uint32_t method )
{
    const uint32_t header_size = sizeof(lsteam_bridge_network_message_header);
    lsteam_bridge_network_message_header header = {};
    uint8_t *payload = NULL;
    uint32_t offset = 0, total = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = method;
        request.arg0 = handle;
        request.arg1 = offset;
        if (!bridge_network_sockets_rpc( &request, &response ) ||
            !response.result) {
            free( payload );
            return NULL;
        }
        if (response.data_size < header_size) {
            free( payload );
            return NULL;
        }
        if (!offset) {
            total = response.arg0;
            if (total > 16u * 1024u * 1024u) return NULL;
            memcpy( &header, response.data, header_size );
            payload = total ? (uint8_t *)malloc( total ) : NULL;
            if (total && !payload) return NULL;
        }
        uint32_t chunk = response.data_size - header_size;
        if (chunk > total - offset) {
            free( payload );
            return NULL;
        }
        if (chunk) memcpy( payload + offset,
                           response.data + header_size, chunk );
        offset += chunk;
        if (offset < total && !chunk) {
            free( payload );
            return NULL;
        }
    } while (offset < total);

    u_SteamNetworkingMessage_t_153a *message =
        (u_SteamNetworkingMessage_t_153a *)calloc( 1, sizeof(*message) );
    if (!message) {
        free( payload );
        return NULL;
    }
    message->m_pData = payload;
    message->m_cbSize = total;
    message->m_conn = header.connection;
    memcpy( &message->m_identityPeer, header.identity,
            sizeof(message->m_identityPeer) );
    message->m_nConnUserData = header.connection_user_data;
    message->m_usecTimeReceived = header.time_received;
    message->m_nMessageNumber = header.message_number;
    message->m_pfnRelease = bridge_network_sockets_release;
    message->m_nChannel = header.channel;
    message->m_nFlags = header.flags;
    message->m_nUserData = header.user_data;
    message->m_idxLane = header.lane;
    return message;
}

static int32_t bridge_network_sockets_receive_common(
    uint32_t handle, u_SteamNetworkingMessage_t_153a **messages,
    int32_t maximum, bool poll_group )
{
    if (!messages || maximum <= 0 || maximum > 1024) return 0;
    int32_t count = 0;
    while (count < maximum) {
        u_SteamNetworkingMessage_t_153a *message =
            bridge_network_sockets_read_one( handle, poll_group ? 20 : 2 );
        if (!message) break;
        messages[count++] = message;
    }
    return count;
}

static int32_t bridge_network_sockets_receive(
    void *, uint32_t connection, u_SteamNetworkingMessage_t_153a **messages,
    int32_t maximum )
{
    return bridge_network_sockets_receive_common(
        connection, messages, maximum, false );
}

static int32_t bridge_network_sockets_receive_poll(
    void *, uint32_t group, u_SteamNetworkingMessage_t_153a **messages,
    int32_t maximum )
{
    return bridge_network_sockets_receive_common(
        group, messages, maximum, true );
}

static uint32_t bridge_network_sockets_handle_result(
    uint32_t method, uint32_t handle )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = handle;
    return bridge_network_sockets_rpc( &request, &response )
        ? response.result : 2;
}

static uint32_t bridge_network_sockets_accept(void *, uint32_t connection)
{ return bridge_network_sockets_handle_result( 3, connection ); }
static int8_t bridge_network_sockets_close_listen(void *, uint32_t socket)
{ return bridge_network_sockets_handle_result( 5, socket ) != 0; }
static uint32_t bridge_network_sockets_flush(void *, uint32_t connection)
{ return bridge_network_sockets_handle_result( 10, connection ); }
static uint32_t bridge_network_sockets_create_poll(void *)
{ return bridge_network_sockets_handle_result( 17, 0 ); }
static int8_t bridge_network_sockets_destroy_poll(void *, uint32_t group)
{ return bridge_network_sockets_handle_result( 18, group ) != 0; }

static int8_t bridge_network_sockets_close(
    void *, uint32_t connection, int32_t reason, const char *debug,
    int8_t linger )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.arg0 = connection;
    request.arg1 = reason;
    request.arg2 = linger != 0;
    if (debug) snprintf( request.text, sizeof(request.text), "%s", debug );
    return bridge_network_sockets_rpc( &request, &response ) &&
           response.result;
}

static int8_t bridge_network_sockets_set_userdata(
    void *, uint32_t connection, int64_t user_data )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    request.arg0 = connection;
    request.steam_id = (uint64_t)user_data;
    return bridge_network_sockets_rpc( &request, &response ) &&
           response.result;
}

static int64_t bridge_network_sockets_get_userdata(
    void *, uint32_t connection )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 7;
    request.arg0 = connection;
    return bridge_network_sockets_rpc( &request, &response )
        ? (int64_t)response.steam_id : -1;
}

static void bridge_network_sockets_set_name(
    void *, uint32_t connection, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 8;
    request.arg0 = connection;
    if (name) snprintf( request.text, sizeof(request.text), "%s", name );
    bridge_network_sockets_rpc( &request, &response );
}

static int8_t bridge_network_sockets_get_name(
    void *, uint32_t connection, char *name, int32_t capacity )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!name || capacity <= 0) return 0;
    request.method = 9;
    request.arg0 = connection;
    request.arg1 = capacity;
    if (!bridge_network_sockets_rpc( &request, &response ) ||
        !response.result || !response.data_size) return 0;
    uint32_t copy = response.data_size;
    if (copy >= (uint32_t)capacity) copy = capacity - 1;
    memcpy( name, response.data, copy );
    name[copy] = 0;
    return 1;
}

static int8_t bridge_network_sockets_info(
    void *, uint32_t connection, SteamNetConnectionInfo_t_153a *info )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!info) return 0;
    request.method = 11;
    request.arg0 = connection;
    if (!bridge_network_sockets_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*info)) return 0;
    memcpy( info, response.data, sizeof(*info) );
    return 1;
}

static uint32_t bridge_network_sockets_quick(
    void *, uint32_t connection, SteamNetConnectionRealTimeStatus_t_164 *quick,
    int32_t lane_count, SteamNetConnectionRealTimeLaneStatus_t *lanes )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (lane_count < 0 || lane_count > 16 || (lane_count && !lanes)) return 8;
    request.method = 12;
    request.arg0 = connection;
    request.arg1 = lane_count;
    request.arg2 = quick ? 1 : 0;
    if (!bridge_network_sockets_rpc( &request, &response )) return 2;
    size_t expected = (quick ? sizeof(*quick) : 0) +
                      lane_count * sizeof(*lanes);
    if (response.data_size != expected) return 2;
    size_t offset = 0;
    if (quick) {
        memcpy( quick, response.data, sizeof(*quick) );
        offset += sizeof(*quick);
    }
    if (lane_count)
        memcpy( lanes, response.data + offset, lane_count * sizeof(*lanes) );
    return response.result;
}

static int32_t bridge_network_sockets_detailed(
    void *, uint32_t connection, char *buffer, int32_t capacity )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (capacity < 0 || (capacity && !buffer)) return -1;
    request.method = 13;
    request.arg0 = connection;
    request.arg1 = capacity;
    if (!bridge_network_sockets_rpc( &request, &response )) return -1;
    if (capacity && response.data_size) {
        uint32_t copy = response.data_size;
        if (copy >= (uint32_t)capacity) copy = capacity - 1;
        memcpy( buffer, response.data, copy );
        buffer[copy] = 0;
    }
    return (int32_t)(int64_t)response.result;
}

static int8_t bridge_network_sockets_identity(
    void *, SteamNetworkingIdentity_144 *identity )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!identity) return 0;
    request.method = 14;
    if (!bridge_network_sockets_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*identity)) return 0;
    memcpy( identity, response.data, sizeof(*identity) );
    return 1;
}

static uint32_t bridge_network_sockets_init_auth(void *)
{ return bridge_network_sockets_handle_result( 15, 0 ); }

static uint32_t bridge_network_sockets_auth_status(
    void *, SteamNetAuthenticationStatus_t *details )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 16;
    request.arg0 = details != NULL;
    if (!bridge_network_sockets_rpc( &request, &response )) return 0;
    if (details) {
        if (response.data_size != sizeof(*details)) return 0;
        memcpy( details, response.data, sizeof(*details) );
    }
    return response.result;
}

static int8_t bridge_network_sockets_set_poll(
    void *, uint32_t connection, uint32_t group )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 19;
    request.arg0 = connection;
    request.arg1 = group;
    return bridge_network_sockets_rpc( &request, &response ) &&
           response.result;
}

static uint32_t bridge_network_sockets_listen_p2p(
    void *, int32_t port, int32_t option_count,
    const SteamNetworkingConfigValue_t * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (option_count) return 0;
    request.method = 21;
    request.arg0 = port;
    return bridge_network_sockets_rpc( &request, &response )
        ? response.result : 0;
}

static uint32_t bridge_network_sockets_connect_p2p(
    void *, const SteamNetworkingIdentity_144 *identity, int32_t port,
    int32_t option_count, const SteamNetworkingConfigValue_t * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!identity || option_count) return 0;
    request.method = 22;
    request.arg0 = port;
    request.data_size = sizeof(*identity);
    memcpy( request.data, identity, sizeof(*identity) );
    return bridge_network_sockets_rpc( &request, &response )
        ? response.result : 0;
}

static uint32_t bridge_network_sockets_ip_common(
    uint32_t method, const SteamNetworkingIPAddr *address,
    int32_t option_count )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!address || option_count) return 0;
    request.method = method;
    request.data_size = sizeof(*address);
    memcpy( request.data, address, sizeof(*address) );
    return bridge_network_sockets_rpc( &request, &response )
        ? response.result : 0;
}

static uint32_t bridge_network_sockets_listen_ip(
    void *, const SteamNetworkingIPAddr *address, int32_t option_count,
    const SteamNetworkingConfigValue_t * )
{ return bridge_network_sockets_ip_common( 23, address, option_count ); }

static uint32_t bridge_network_sockets_connect_ip(
    void *, const SteamNetworkingIPAddr *address, int32_t option_count,
    const SteamNetworkingConfigValue_t * )
{ return bridge_network_sockets_ip_common( 24, address, option_count ); }

static int8_t bridge_network_sockets_listen_address(
    void *, uint32_t socket, SteamNetworkingIPAddr *address )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!address) return 0;
    request.method = 25;
    request.arg0 = socket;
    if (!bridge_network_sockets_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*address)) return 0;
    memcpy( address, response.data, sizeof(*address) );
    return 1;
}

static uint32_t bridge_network_sockets_lanes(
    void *, uint32_t connection, int32_t count, const int32_t *priorities,
    const uint16_t *weights )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (count < 0 || count > 128 ||
        (count && (!priorities || !weights))) return 8;
    request.method = 26;
    request.arg0 = connection;
    request.arg1 = count;
    request.data_size = count * (sizeof(*priorities) + sizeof(*weights));
    if (count) {
        memcpy( request.data, priorities, count * sizeof(*priorities) );
        memcpy( request.data + count * sizeof(*priorities), weights,
                count * sizeof(*weights) );
    }
    return bridge_network_sockets_rpc( &request, &response )
        ? response.result : 2;
}

static void bridge_network_sockets_send_many_common(
    void *iface, int32_t count, u_SteamNetworkingMessage_t_153a **messages,
    int64_t *results, bool delete_failed )
{
    if (count < 0 || count > 1024 || (count && !messages)) return;
    for (int32_t index = 0; index < count; ++index) {
        u_SteamNetworkingMessage_t_153a *message = messages[index];
        uint32_t result = message
            ? bridge_network_sockets_send(
                iface, message->m_conn, message->m_pData, message->m_cbSize,
                message->m_nFlags, results ? &results[index] : NULL)
            : 8;
        if (results && result != 1) results[index] = -(int64_t)result;
        if (message && (result == 1 || delete_failed) && message->m_pfnRelease) {
            message->m_pfnRelease( message );
        }
    }
}

static void *bridge_network_sockets_create_fake( void *, int32_t port )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 27;
    request.arg0 = port;
    return bridge_network_sockets_rpc( &request, &response ) && response.result
        ? &bridge_network_fake_udp : NULL;
}

static void bridge_network_fake_udp_destroy( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 28;
    bridge_network_sockets_rpc( &request, &response );
}

static uint32_t bridge_network_fake_udp_send(
    void *, const SteamNetworkingIPAddr *address, const void *data,
    uint32_t size, int32_t flags )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!address || (size && !data) ||
        size > sizeof(request.data) - sizeof(*address)) return 8;
    request.method = 29;
    request.arg0 = flags;
    memcpy( request.data, address, sizeof(*address) );
    if (size) memcpy( request.data + sizeof(*address), data, size );
    request.data_size = sizeof(*address) + size;
    return bridge_network_sockets_rpc( &request, &response )
        ? response.result : 2;
}

static int32_t bridge_network_fake_udp_receive(
    void *, u_SteamNetworkingMessage_t_153a **messages, int32_t maximum )
{
    if (!messages || maximum <= 0 || maximum > 1024) return 0;
    int32_t count = 0;
    while (count < maximum) {
        u_SteamNetworkingMessage_t_153a *message =
            bridge_network_sockets_read_one( 0, 30 );
        if (!message) break;
        messages[count++] = message;
    }
    return count;
}

static void bridge_network_fake_udp_cleanup(
    void *, const SteamNetworkingIPAddr *address )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!address) return;
    request.method = 31;
    request.data_size = sizeof(*address);
    memcpy( request.data, address, sizeof(*address) );
    bridge_network_sockets_rpc( &request, &response );
}

/* SteamNetworkingSockets012 owns every message accepted by SendMessages and
 * has no bDeleteFailed argument.  013 added that argument, so sharing a raw
 * vtable entry would read a garbage fifth register for 012 callers. */
static void bridge_network_sockets_send_many12(
    void *iface, int32_t count,
    u_SteamNetworkingMessage_t_153a *const *messages, int64_t *results )
{
    bridge_network_sockets_send_many_common(
        iface, count,
        const_cast<u_SteamNetworkingMessage_t_153a **>( messages ),
        results, true );
}

static void bridge_network_sockets_send_many13(
    void *iface, int32_t count, u_SteamNetworkingMessage_t_153a **messages,
    int64_t *results, int8_t delete_failed )
{
    bridge_network_sockets_send_many_common(
        iface, count, messages, results, delete_failed != 0 );
}
