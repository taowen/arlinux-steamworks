static uint32_t bridge_get_auth_ticket( void *, void *ticket, int32_t max_ticket,
                                        uint32_t *ticket_size )
{
    const char *path = getenv( "LSTEAM_BRIDGE_SOCKET" );
    if (!path || !*path) path = LSTEAM_BRIDGE_DEFAULT_SOCKET;
    if (ticket_size) *ticket_size = 0;
    if (!ticket || max_ticket <= 0 ||
        strlen(path) >= sizeof(((sockaddr_un *)0)->sun_path)) return 0;

    int fd = socket( AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0 );
    if (fd < 0) return 0;
    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    memcpy( address.sun_path, path, strlen(path) + 1 );
    if (connect( fd, (sockaddr *)&address,
                 offsetof(sockaddr_un, sun_path) + strlen(path) + 1 ) < 0)
    {
        close( fd );
        return 0;
    }

    lsteam_bridge_request request = {
        LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION, LSTEAM_BRIDGE_GET_AUTH_TICKET, 2
    };
    lsteam_bridge_ticket_response response = {};
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              recv( fd, &response, sizeof(response), 0 ) == sizeof(response);
    close( fd );
    ok = ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
         response.version == LSTEAM_BRIDGE_VERSION && !response.status &&
         response.ticket_size <= LSTEAM_BRIDGE_MAX_TICKET &&
         response.ticket_size <= (uint32_t)max_ticket;
    if (!ok) return 0;

    memcpy( ticket, response.ticket, response.ticket_size );
    if (ticket_size) *ticket_size = response.ticket_size;
    bridge_trace( "GetAuthSessionTicket handle=%u size=%u", response.ticket_handle,
                  response.ticket_size );
    return response.ticket_handle;
}

static int32_t bridge_initiate_game_connection( void *, void *blob, int32_t max_blob,
                                                CSteamID server_steam_id,
                                                uint32_t server_ip,
                                                uint16_t server_port, int8_t secure )
{
    const char *path = getenv( "LSTEAM_BRIDGE_SOCKET" );
    if (!path || !*path) path = LSTEAM_BRIDGE_DEFAULT_SOCKET;
    if (!blob || max_blob <= 0 ||
        strlen(path) >= sizeof(((sockaddr_un *)0)->sun_path)) return 0;

    int fd = socket( AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0 );
    if (fd < 0) return 0;
    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    memcpy( address.sun_path, path, strlen(path) + 1 );
    if (connect( fd, (sockaddr *)&address,
                 offsetof(sockaddr_un, sun_path) + strlen(path) + 1 ) < 0)
    {
        close( fd );
        return 0;
    }

    lsteam_bridge_request request = {
        LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        LSTEAM_BRIDGE_INIT_GAME_CONNECTION, 3
    };
    lsteam_bridge_game_connection_request game_request = {};
    game_request.max_blob = max_blob;
    game_request.server_ip = server_ip;
    memcpy( &game_request.server_steam_id, &server_steam_id,
            sizeof(game_request.server_steam_id) );
    game_request.server_port = server_port;
    game_request.secure = secure != 0;
    lsteam_bridge_blob_response response = {};
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, &game_request, sizeof(game_request), MSG_NOSIGNAL ) ==
                  sizeof(game_request) &&
              recv( fd, &response, sizeof(response), 0 ) == sizeof(response);
    close( fd );
    ok = ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
         response.version == LSTEAM_BRIDGE_VERSION && !response.status &&
         response.blob_size <= LSTEAM_BRIDGE_MAX_TICKET &&
         response.blob_size <= (uint32_t)max_blob;
    if (!ok) return 0;

    memcpy( blob, response.blob, response.blob_size );
    bridge_trace( "InitiateGameConnection server=%llu ip=%u port=%u secure=%d size=%u",
                  (unsigned long long)game_request.server_steam_id, server_ip,
                  server_port, secure != 0, response.blob_size );
    return response.blob_size;
}

// SteamUser023 returns the handle immediately and the real native Steam
// callback 168 supplies the ticket. Never synthesize successful authentication.
static uint32_t bridge_get_web_api_ticket( void *, const char *identity )
{
    if (!identity || strnlen(identity, 256) >= 256) return 0;
    const char *path;
    int fd;
    if (!bridge_rpc_connect( &fd, &path )) return 0;
    lsteam_bridge_request request = {
        LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION, LSTEAM_BRIDGE_WEB_API_TICKET, 4
    };
    lsteam_bridge_ticket_response response = {};
    size_t length = strlen(identity) + 1;
    bool ok = send(fd, &request, sizeof(request), MSG_NOSIGNAL) == sizeof(request) &&
              send(fd, identity, length, MSG_NOSIGNAL) == (ssize_t)length &&
              recv(fd, &response, sizeof(response), 0) == sizeof(response);
    close(fd);
    ok = ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
         response.version == LSTEAM_BRIDGE_VERSION &&
         response.opcode == request.opcode && response.sequence == request.sequence &&
         !response.status;
    bridge_trace("GetAuthTicketForWebApi success=%d", ok && response.ticket_handle);
    return ok ? response.ticket_handle : 0;
}
