/* Bionic-side ISteamClient calls which cannot be represented by the shared
 * local handle/getter helpers. */
static void bridge_client_set_local_ipv4( void *, uint32_t ip, uint16_t port )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = port;
    request.data_size = sizeof(ip);
    memcpy( request.data, &ip, sizeof(ip) );
    bridge_iface_rpc( LSTEAM_BRIDGE_CLIENT_CALL, &request, &response );
}

static void bridge_client_set_local_ip( void *,
                                        const SteamIPAddress_t *address,
                                        uint16_t port )
{
    if (!address) return;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = port;
    request.data_size = sizeof(*address);
    memcpy( request.data, address, sizeof(*address) );
    bridge_iface_rpc( LSTEAM_BRIDGE_CLIENT_CALL, &request, &response );
}

static void bridge_client_set_callback_checker(
    void *, uint32_t (U_CDECL *callback)(int32_t) )
{
    /* A Wine callback pointer cannot cross into Steam's native process.  The
     * server installs an always-registered checker and forwards the resulting
     * callback queue back to Wine, so retaining this pointer is unnecessary. */
    (void)callback;
}
