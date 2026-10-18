/* SteamUser voice calls transported to the native Steam process. */
static bool bridge_voice_rpc( lsteam_bridge_voice_request *voice_request,
                              lsteam_bridge_voice_response *response )
{
    static uint32_t next_sequence = 24000;
    const char *path;
    int fd;
    if (!bridge_rpc_connect( &fd, &path )) return false;
    lsteam_bridge_request request = {
        LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION, LSTEAM_BRIDGE_VOICE_CALL,
        __atomic_add_fetch( &next_sequence, 1, __ATOMIC_RELAXED )
    };
    memset( response, 0, sizeof(*response) );
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, voice_request, sizeof(*voice_request),
                    MSG_NOSIGNAL ) == sizeof(*voice_request) &&
              recv( fd, response, sizeof(*response), 0 ) == sizeof(*response);
    close( fd );
    ok = ok && response->magic == LSTEAM_BRIDGE_MAGIC &&
         response->version == LSTEAM_BRIDGE_VERSION &&
         response->opcode == LSTEAM_BRIDGE_VOICE_CALL &&
         response->sequence == request.sequence && !response->status &&
         response->data_size <= sizeof(response->data);
    if (!ok)
        bridge_trace( "voice method=%u failed status=%d", voice_request->method,
                      response->status );
    return ok;
}

static void bridge_user_start_voice( void * )
{
    lsteam_bridge_voice_request request = {};
    lsteam_bridge_voice_response response = {};
    request.method = 0;
    bridge_voice_rpc( &request, &response );
}

static void bridge_user_stop_voice( void * )
{
    lsteam_bridge_voice_request request = {};
    lsteam_bridge_voice_response response = {};
    request.method = 1;
    bridge_voice_rpc( &request, &response );
}

static uint32_t bridge_user_available_voice( void *, uint32_t *compressed,
                                             uint32_t *uncompressed,
                                             uint32_t sample_rate )
{
    lsteam_bridge_voice_request request = {};
    lsteam_bridge_voice_response response = {};
    if (compressed) *compressed = 0;
    if (uncompressed) *uncompressed = 0;
    request.method = 2;
    request.flags = (compressed ? 1u : 0u) | (uncompressed ? 2u : 0u);
    request.arg0 = sample_rate;
    if (!bridge_voice_rpc( &request, &response )) return 1;
    if (compressed) *compressed = response.compressed_size;
    if (uncompressed) *uncompressed = response.uncompressed_size;
    return response.result;
}

static uint32_t bridge_user_get_voice(
    void *, int8_t want_compressed, void *compressed, uint32_t compressed_capacity,
    uint32_t *compressed_written, int8_t want_uncompressed, void *uncompressed,
    uint32_t uncompressed_capacity, uint32_t *uncompressed_written,
    uint32_t sample_rate )
{
    lsteam_bridge_voice_request request = {};
    lsteam_bridge_voice_response response = {};
    if (compressed_written) *compressed_written = 0;
    if (uncompressed_written) *uncompressed_written = 0;
    if ((want_compressed && compressed_capacity && !compressed) ||
        (want_uncompressed && uncompressed_capacity && !uncompressed) ||
        compressed_capacity > LSTEAM_BRIDGE_MAX_VOICE ||
        uncompressed_capacity > LSTEAM_BRIDGE_MAX_VOICE - compressed_capacity)
        return 4;
    request.method = 3;
    request.flags = (want_compressed ? 1u : 0u) |
                    (want_uncompressed ? 2u : 0u);
    request.arg0 = want_compressed ? compressed_capacity : 0;
    request.arg1 = want_uncompressed ? uncompressed_capacity : 0;
    request.arg2 = sample_rate;
    if (!bridge_voice_rpc( &request, &response )) return 1;
    if (compressed_written) *compressed_written = response.compressed_size;
    if (uncompressed_written) *uncompressed_written = response.uncompressed_size;
    if (response.result != 0) return response.result;
    if (response.compressed_size > request.arg0 ||
        response.uncompressed_size > request.arg1 ||
        response.compressed_size > response.data_size ||
        response.uncompressed_size > response.data_size - response.compressed_size)
        return 5;
    if (response.compressed_size)
        memcpy( compressed, response.data, response.compressed_size );
    if (response.uncompressed_size)
        memcpy( uncompressed, response.data + response.compressed_size,
                response.uncompressed_size );
    return response.result;
}

static uint32_t bridge_user_decompress_voice(
    void *, const void *compressed, uint32_t compressed_size, void *destination,
    uint32_t destination_capacity, uint32_t *written, uint32_t sample_rate )
{
    lsteam_bridge_voice_request request = {};
    lsteam_bridge_voice_response response = {};
    if (written) *written = 0;
    if ((compressed_size && !compressed) ||
        (destination_capacity && !destination) ||
        compressed_size > LSTEAM_BRIDGE_MAX_VOICE ||
        destination_capacity > LSTEAM_BRIDGE_MAX_VOICE)
        return 4;
    request.method = 4;
    request.arg0 = destination_capacity;
    request.arg1 = sample_rate;
    request.data_size = compressed_size;
    if (compressed_size) memcpy( request.data, compressed, compressed_size );
    if (!bridge_voice_rpc( &request, &response )) return 1;
    if (written) *written = response.uncompressed_size;
    if (response.result != 0) return response.result;
    if (response.uncompressed_size > destination_capacity ||
        response.uncompressed_size > response.data_size)
        return 5;
    if (response.uncompressed_size)
        memcpy( destination, response.data, response.uncompressed_size );
    return response.result;
}

static uint32_t bridge_user_voice_sample_rate( void * )
{
    lsteam_bridge_voice_request request = {};
    lsteam_bridge_voice_response response = {};
    request.method = 5;
    return bridge_voice_rpc( &request, &response ) && response.result ?
           response.result : 48000;
}

/* SteamUser013-015 predate the sample-rate arguments on capture. */
static uint32_t bridge_user_available_voice_legacy(
    void *iface, uint32_t *compressed, uint32_t *uncompressed )
{
    return bridge_user_available_voice( iface, compressed, uncompressed, 11025 );
}

static uint32_t bridge_user_get_voice_legacy(
    void *iface, int8_t want_compressed, void *compressed,
    uint32_t compressed_capacity, uint32_t *compressed_written,
    int8_t want_uncompressed, void *uncompressed,
    uint32_t uncompressed_capacity, uint32_t *uncompressed_written )
{
    return bridge_user_get_voice(
        iface, want_compressed, compressed, compressed_capacity,
        compressed_written, want_uncompressed, uncompressed,
        uncompressed_capacity, uncompressed_written, 11025 );
}

/* SteamUser011/012 expose GetCompressedVoice instead of GetVoice. */
static uint32_t bridge_user_get_compressed_voice(
    void *iface, void *compressed, uint32_t capacity, uint32_t *written )
{
    return bridge_user_get_voice( iface, 1, compressed, capacity, written,
                                  0, NULL, 0, NULL, 11025 );
}

/* SteamUser011-014 always decompressed to signed 11025 Hz PCM. */
static uint32_t bridge_user_decompress_voice_legacy(
    void *iface, const void *compressed, uint32_t compressed_size,
    void *destination, uint32_t destination_capacity, uint32_t *written )
{
    return bridge_user_decompress_voice(
        iface, compressed, compressed_size, destination,
        destination_capacity, written, 11025 );
}
