/* Bionic-side proxy for the private app ownership ticket interface used by
 * Steam Simple DRM.  Ticket contents are deliberately never traced. */
static bool bridge_app_ticket_range_valid( uint32_t offset, uint32_t size,
                                           uint32_t total )
{
    return offset <= total && size <= total - offset;
}

static uint32_t bridge_app_ticket_get_ownership_data(
    void *, uint32_t app_id, void *buffer, uint32_t capacity,
    uint32_t *app_id_offset, uint32_t *steam_id_offset,
    uint32_t *signature_offset, uint32_t *signature_size )
{
    static uint32_t sequence = 34000;
    lsteam_bridge_app_ticket_request ticket_request = {};
    lsteam_bridge_app_ticket_response response = {};
    lsteam_bridge_request request = {};
    const char *path;
    uint32_t request_capacity;
    int fd;

    if (app_id_offset) *app_id_offset = 0;
    if (steam_id_offset) *steam_id_offset = 0;
    if (signature_offset) *signature_offset = 0;
    if (signature_size) *signature_size = 0;
    if (!app_id || !buffer || !capacity || !bridge_rpc_connect( &fd, &path ))
        return 0;

    request_capacity = capacity;
    if (request_capacity > LSTEAM_BRIDGE_MAX_TICKET)
        request_capacity = LSTEAM_BRIDGE_MAX_TICKET;
    request.magic = LSTEAM_BRIDGE_MAGIC;
    request.version = LSTEAM_BRIDGE_VERSION;
    request.opcode = LSTEAM_BRIDGE_APP_TICKET_CALL;
    request.sequence = __atomic_add_fetch( &sequence, 1, __ATOMIC_RELAXED );
    ticket_request.app_id = app_id;
    ticket_request.capacity = request_capacity;

    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, &ticket_request, sizeof(ticket_request), MSG_NOSIGNAL ) ==
                  sizeof(ticket_request) &&
              recv( fd, &response, sizeof(response), 0 ) == sizeof(response);
    close( fd );
    ok = ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
         response.version == LSTEAM_BRIDGE_VERSION &&
         response.opcode == LSTEAM_BRIDGE_APP_TICKET_CALL &&
         response.sequence == request.sequence && !response.status &&
         response.returned_size &&
         response.returned_size <= request_capacity &&
         response.returned_size <= capacity &&
         response.returned_size <= LSTEAM_BRIDGE_MAX_TICKET &&
         bridge_app_ticket_range_valid( response.app_id_offset,
                                        sizeof(uint32_t),
                                        response.returned_size ) &&
         bridge_app_ticket_range_valid( response.steam_id_offset,
                                        sizeof(uint64_t),
                                        response.returned_size ) &&
         response.signature_size &&
         bridge_app_ticket_range_valid( response.signature_offset,
                                        response.signature_size,
                                        response.returned_size );
    if (!ok) return 0;

    memcpy( buffer, response.ticket, response.returned_size );
    if (app_id_offset) *app_id_offset = response.app_id_offset;
    if (steam_id_offset) *steam_id_offset = response.steam_id_offset;
    if (signature_offset) *signature_offset = response.signature_offset;
    if (signature_size) *signature_size = response.signature_size;
    bridge_trace( "GetAppOwnershipTicketData appid=%u size=%u offsets=%u/%u/%u signature_size=%u",
                  app_id, response.returned_size, response.app_id_offset,
                  response.steam_id_offset, response.signature_offset,
                  response.signature_size );
    return response.returned_size;
}
