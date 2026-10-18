/* Bionic-side SteamNetworkingMessages002 proxy. */
static_assert( sizeof(SteamNetworkingIdentity_144) == 136,
               "unexpected bridge identity ABI" );

static bool bridge_network_messages_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_NETWORK_MESSAGES_CALL,
                             request, response );
}

static uint32_t bridge_network_messages_send(
    void *, const SteamNetworkingIdentity_144 *identity, const void *data,
    uint32_t size, int32_t flags, int32_t channel )
{
    if (!identity || (size && !data) || size > 16u * 1024u * 1024u)
        return 2; /* k_EResultFail */
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        uint32_t prefix = offset ? 0 : sizeof(*identity);
        uint32_t chunk = size - offset;
        if (chunk > sizeof(request.data) - prefix)
            chunk = sizeof(request.data) - prefix;
        request.method = 0;
        request.arg0 = flags;
        request.arg1 = channel;
        request.arg2 = size;
        request.arg3 = offset;
        request.data_size = prefix + chunk;
        if (prefix) memcpy( request.data, identity, prefix );
        if (chunk) memcpy( request.data + prefix,
                           (const uint8_t *)data + offset, chunk );
        if (!bridge_network_messages_rpc( &request, &response )) return 2;
        offset += chunk;
        if (offset == size) return response.result;
    } while (offset < size);
    return 2;
}

static void U_CDECL bridge_network_messages_release(
    u_SteamNetworkingMessage_t_153a *message )
{
    if (!message) return;
    free( message->m_pData );
    free( message );
}

static u_SteamNetworkingMessage_t_153a *bridge_network_messages_read_one(
    int32_t channel )
{
    const uint32_t header_size = sizeof(lsteam_bridge_network_message_header);
    lsteam_bridge_network_message_header header = {};
    uint8_t *payload = NULL;
    uint32_t offset = 0, total = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 1;
        request.arg0 = channel;
        request.arg1 = offset;
        if (!bridge_network_messages_rpc( &request, &response ) ||
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
    message->m_pfnRelease = bridge_network_messages_release;
    message->m_nChannel = header.channel;
    message->m_nFlags = header.flags;
    message->m_nUserData = header.user_data;
    message->m_idxLane = header.lane;
    return message;
}

static int32_t bridge_network_messages_receive(
    void *, int32_t channel, u_SteamNetworkingMessage_t_153a **messages,
    int32_t maximum )
{
    if (!messages || maximum <= 0 || maximum > 1024) return 0;
    int32_t count = 0;
    while (count < maximum)
    {
        u_SteamNetworkingMessage_t_153a *message =
            bridge_network_messages_read_one( channel );
        if (!message) break;
        messages[count++] = message;
    }
    return count;
}

static int8_t bridge_network_messages_identity_bool(
    uint32_t method, const SteamNetworkingIdentity_144 *identity,
    uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!identity) return 0;
    request.method = method;
    request.arg0 = argument;
    request.data_size = sizeof(*identity);
    memcpy( request.data, identity, sizeof(*identity) );
    return bridge_network_messages_rpc( &request, &response ) &&
           response.result;
}

static int8_t bridge_network_messages_accept(
    void *, const SteamNetworkingIdentity_144 *identity )
{ return bridge_network_messages_identity_bool( 2, identity ); }
static int8_t bridge_network_messages_close(
    void *, const SteamNetworkingIdentity_144 *identity )
{ return bridge_network_messages_identity_bool( 3, identity ); }
static int8_t bridge_network_messages_close_channel(
    void *, const SteamNetworkingIdentity_144 *identity, int32_t channel )
{ return bridge_network_messages_identity_bool( 4, identity, channel ); }

static uint32_t bridge_network_messages_info(
    void *, const SteamNetworkingIdentity_144 *identity,
    SteamNetConnectionInfo_t_153a *info,
    SteamNetConnectionRealTimeStatus_t_164 *quick )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!identity) return 0;
    request.method = 5;
    request.arg0 = (info ? 1 : 0) | (quick ? 2 : 0);
    request.data_size = sizeof(*identity);
    memcpy( request.data, identity, sizeof(*identity) );
    if (!bridge_network_messages_rpc( &request, &response )) return 0;
    uint32_t expected = (info ? sizeof(*info) : 0) +
                        (quick ? sizeof(*quick) : 0);
    if (response.data_size != expected) return 0;
    uint32_t offset = 0;
    if (info) {
        memcpy( info, response.data, sizeof(*info) );
        offset += sizeof(*info);
    }
    if (quick) memcpy( quick, response.data + offset, sizeof(*quick) );
    return response.result;
}
