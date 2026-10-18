static bool bridge_http_rpc( lsteam_bridge_game_server_request *request,
                             lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_HTTP_CALL, request, response );
}

static bool bridge_http_copy_text( char *target, size_t capacity,
                                   const char *source )
{
    if (!source) return false;
    size_t length = strlen( source );
    if (length >= capacity) return false;
    memcpy( target, source, length + 1 );
    return true;
}

static uint32_t bridge_http_create( void *, uint32_t method, const char *url )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = method;
    if (!bridge_http_copy_text( request.text, sizeof(request.text), url )) return 0;
    return bridge_http_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_http_context( void *, uint32_t handle, uint64_t context )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    request.arg0 = handle;
    request.steam_id = context;
    return bridge_http_rpc( &request, &response ) && response.result;
}

static int8_t bridge_http_u32( uint32_t method, uint32_t handle,
                               uint32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = handle;
    request.arg1 = value;
    return bridge_http_rpc( &request, &response ) && response.result;
}

static int8_t bridge_http_network_timeout( void *, uint32_t handle,
                                           uint32_t seconds )
{ return bridge_http_u32( 2, handle, seconds ); }

static int8_t bridge_http_two_strings( uint32_t method, uint32_t handle,
                                       const char *first, const char *second )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = handle;
    if (!bridge_http_copy_text( request.text, sizeof(request.text), first ) ||
        !bridge_http_copy_text( request.value, sizeof(request.value), second ))
        return 0;
    return bridge_http_rpc( &request, &response ) && response.result;
}

static int8_t bridge_http_header( void *, uint32_t handle,
                                  const char *name, const char *value )
{ return bridge_http_two_strings( 3, handle, name, value ); }
static int8_t bridge_http_parameter( void *, uint32_t handle,
                                     const char *name, const char *value )
{ return bridge_http_two_strings( 4, handle, name, value ); }

static int8_t bridge_http_send_common( uint32_t method, uint32_t handle,
                                       uint64_t *call )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = handle;
    if (call) *call = 0;
    if (!bridge_http_rpc( &request, &response ) || !response.result) return 0;
    if (call) *call = response.steam_id;
    return 1;
}

static int8_t bridge_http_send( void *, uint32_t handle, uint64_t *call )
{ return bridge_http_send_common( 5, handle, call ); }
static int8_t bridge_http_send_stream( void *, uint32_t handle, uint64_t *call )
{ return bridge_http_send_common( 6, handle, call ); }
static int8_t bridge_http_defer( void *, uint32_t handle )
{ return bridge_http_u32( 7, handle, 0 ); }
static int8_t bridge_http_prioritize( void *, uint32_t handle )
{ return bridge_http_u32( 8, handle, 0 ); }

static int8_t bridge_http_header_size( void *, uint32_t handle,
                                       const char *name, uint32_t *size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 9;
    request.arg0 = handle;
    if (size) *size = 0;
    if (!size || !bridge_http_copy_text( request.text, sizeof(request.text), name ) ||
        !bridge_http_rpc( &request, &response ) || !response.result)
        return 0;
    *size = response.arg0;
    return 1;
}

static int8_t bridge_http_read_chunks( uint32_t method, uint32_t handle,
                                       const char *name, uint32_t source_offset,
                                       uint8_t *buffer, uint32_t size )
{
    if (size && !buffer) return 0;
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = method;
        request.arg0 = handle;
        if (method == 10)
        {
            request.arg1 = offset;
            if (!bridge_http_copy_text( request.text, sizeof(request.text), name ))
                return 0;
        }
        else if (method == 12)
        {
            request.arg1 = offset;
        }
        else
        {
            request.arg1 = source_offset;
            request.arg2 = size;
            request.arg3 = offset;
        }
        if (!bridge_http_rpc( &request, &response ) || !response.result ||
            response.arg0 > size || offset > response.arg0 ||
            response.data_size > response.arg0 - offset ||
            (response.data_size && !buffer))
            return 0;
        if (response.data_size)
            memcpy( buffer + offset, response.data, response.data_size );
        offset += response.data_size;
        if (!response.data_size)
            return response.arg0 == offset &&
                   (method == 10 || method == 13 || response.arg0 == size);
    } while (offset < size);
    return method == 10 || method == 13 || offset == size;
}

static int8_t bridge_http_header_value( void *, uint32_t handle,
                                        const char *name, uint8_t *buffer,
                                        uint32_t size )
{ return bridge_http_read_chunks( 10, handle, name, 0, buffer, size ); }

static int8_t bridge_http_body_size( void *, uint32_t handle, uint32_t *size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    request.arg0 = handle;
    if (size) *size = 0;
    if (!size || !bridge_http_rpc( &request, &response ) || !response.result)
        return 0;
    *size = response.arg0;
    return 1;
}

static int8_t bridge_http_body( void *, uint32_t handle, uint8_t *buffer,
                                uint32_t size )
{ return bridge_http_read_chunks( 12, handle, NULL, 0, buffer, size ); }

static int8_t bridge_http_stream_body( void *, uint32_t handle,
                                       uint32_t source_offset, uint8_t *buffer,
                                       uint32_t size )
{
    return bridge_http_read_chunks( 13, handle, NULL, source_offset,
                                    buffer, size );
}

static int8_t bridge_http_release( void *, uint32_t handle )
{ return bridge_http_u32( 14, handle, 0 ); }

static int8_t bridge_http_progress( void *, uint32_t handle, float *percent )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 15;
    request.arg0 = handle;
    if (percent) *percent = 0.0f;
    if (!percent || !bridge_http_rpc( &request, &response ) ||
        !response.result || response.data_size != sizeof(*percent))
        return 0;
    memcpy( percent, response.data, sizeof(*percent) );
    return 1;
}

static int8_t bridge_http_raw_post( void *, uint32_t handle,
                                    const char *content_type, uint8_t *body,
                                    uint32_t size )
{
    if ((size && !body) || !content_type) return 0;
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 16;
        request.arg0 = handle;
        request.arg1 = offset;
        request.arg2 = size;
        if (!bridge_http_copy_text( request.text, sizeof(request.text),
                                    content_type ))
            return 0;
        uint32_t count = size - offset;
        if (count > sizeof(request.data)) count = sizeof(request.data);
        request.data_size = count;
        if (count) memcpy( request.data, body + offset, count );
        if (!bridge_http_rpc( &request, &response ) || !response.result)
            return 0;
        offset += count;
        if (!size) break;
    } while (offset < size);
    return 1;
}

static uint32_t bridge_http_create_cookie( void *, int8_t allow_modify )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 17;
    request.arg0 = allow_modify != 0;
    return bridge_http_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_http_release_cookie( void *, uint32_t handle )
{ return bridge_http_u32( 18, handle, 0 ); }

static int8_t bridge_http_set_cookie( void *, uint32_t handle,
                                      const char *host, const char *url,
                                      const char *cookie )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 19;
    request.arg0 = handle;
    if (!bridge_http_copy_text( request.text, sizeof(request.text), host ) ||
        !bridge_http_copy_text( request.value, sizeof(request.value), url ) ||
        !cookie)
        return 0;
    size_t length = strlen( cookie ) + 1;
    if (length > sizeof(request.data)) return 0;
    memcpy( request.data, cookie, length );
    request.data_size = length;
    return bridge_http_rpc( &request, &response ) && response.result;
}

static int8_t bridge_http_cookie_container( void *, uint32_t request_handle,
                                            uint32_t cookie_handle )
{ return bridge_http_u32( 20, request_handle, cookie_handle ); }

static int8_t bridge_http_user_agent( void *, uint32_t handle,
                                      const char *agent )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 21;
    request.arg0 = handle;
    if (!bridge_http_copy_text( request.text, sizeof(request.text), agent )) return 0;
    return bridge_http_rpc( &request, &response ) && response.result;
}

static int8_t bridge_http_verified( void *, uint32_t handle, int8_t verified )
{ return bridge_http_u32( 22, handle, verified != 0 ); }
static int8_t bridge_http_absolute_timeout( void *, uint32_t handle,
                                            uint32_t milliseconds )
{ return bridge_http_u32( 23, handle, milliseconds ); }

static int8_t bridge_http_timed_out( void *, uint32_t handle, int8_t *timed_out )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 24;
    request.arg0 = handle;
    if (timed_out) *timed_out = 0;
    if (!timed_out || !bridge_http_rpc( &request, &response ) || !response.result)
        return 0;
    *timed_out = response.arg0 != 0;
    return 1;
}
