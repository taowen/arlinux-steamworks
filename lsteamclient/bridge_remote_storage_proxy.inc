/* Bionic-side Steam Remote Storage v014 core file/cloud operations. */
static bool bridge_remote_rpc( lsteam_bridge_game_server_request *request,
                               lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_REMOTE_STORAGE_CALL,
                             request, response );
}

static int8_t bridge_remote_file_write( void *, const char *name,
                                        const void *data, int32_t size )
{
    if (!name || size < 0 || (size && !data) ||
        (uint32_t)size > 64u * 1024u * 1024u) return 0;
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        uint32_t chunk = (uint32_t)size - offset;
        if (chunk > LSTEAM_BRIDGE_MAX_TICKET)
            chunk = LSTEAM_BRIDGE_MAX_TICKET;
        request.method = 0;
        request.arg0 = size;
        request.arg1 = offset;
        request.data_size = chunk;
        bridge_iface_copy_text( request.text, name );
        if (chunk) memcpy( request.data, (const uint8_t *)data + offset, chunk );
        if (!bridge_remote_rpc( &request, &response ) || !response.result)
            return 0;
        offset += chunk;
    } while (offset < (uint32_t)size);
    return 1;
}

static int32_t bridge_remote_file_read( void *, const char *name, void *data,
                                        int32_t capacity )
{
    if (!name || !data || capacity <= 0 ||
        (uint32_t)capacity > 64u * 1024u * 1024u) return 0;
    uint32_t offset = 0, total = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 1;
        request.arg0 = capacity;
        request.arg1 = offset;
        bridge_iface_copy_text( request.text, name );
        if (!bridge_remote_rpc( &request, &response )) return 0;
        if (!offset) total = response.arg0;
        if (total > (uint32_t)capacity ||
            response.data_size > total - offset) return 0;
        if (response.data_size)
            memcpy( (uint8_t *)data + offset, response.data,
                    response.data_size );
        offset += response.data_size;
        if (offset < total && !response.data_size) return 0;
    } while (offset < total);
    return total;
}

static int8_t bridge_remote_text_bool( uint32_t method, const char *name,
                                       uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = argument;
    bridge_iface_copy_text( request.text, name );
    return bridge_remote_rpc( &request, &response ) && response.result;
}
static int8_t bridge_remote_forget( void *, const char *name )
{ return bridge_remote_text_bool( 5, name ); }
static int8_t bridge_remote_delete( void *, const char *name )
{ return bridge_remote_text_bool( 6, name ); }
static int8_t bridge_remote_set_sync( void *, const char *name, uint32_t platforms )
{ return bridge_remote_text_bool( 8, name, platforms ); }
static int8_t bridge_remote_exists( void *, const char *name )
{ return bridge_remote_text_bool( 13, name ); }
static int8_t bridge_remote_persisted( void *, const char *name )
{ return bridge_remote_text_bool( 14, name ); }

static uint64_t bridge_remote_file_share( void *, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 7;
    bridge_iface_copy_text( request.text, name );
    return bridge_remote_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_remote_stream_open( void *, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 9;
    bridge_iface_copy_text( request.text, name );
    return bridge_remote_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_remote_stream_chunk( void *, uint64_t stream,
                                          const void *data, int32_t size )
{
    if (size < 0 || (size && !data)) return 0;
    uint32_t offset = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        uint32_t chunk = (uint32_t)size - offset;
        if (chunk > LSTEAM_BRIDGE_MAX_TICKET)
            chunk = LSTEAM_BRIDGE_MAX_TICKET;
        request.method = 10;
        request.steam_id = stream;
        request.data_size = chunk;
        if (chunk) memcpy( request.data, (const uint8_t *)data + offset, chunk );
        if (!bridge_remote_rpc( &request, &response ) || !response.result)
            return 0;
        offset += chunk;
    } while (offset < (uint32_t)size);
    return 1;
}

static int8_t bridge_remote_stream_finish( uint32_t method, uint64_t stream )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = stream;
    return bridge_remote_rpc( &request, &response ) && response.result;
}
static int8_t bridge_remote_stream_close( void *, uint64_t stream )
{ return bridge_remote_stream_finish( 11, stream ); }
static int8_t bridge_remote_stream_cancel( void *, uint64_t stream )
{ return bridge_remote_stream_finish( 12, stream ); }

static int64_t bridge_remote_text_integer( uint32_t method, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    bridge_iface_copy_text( request.text, name );
    return bridge_remote_rpc( &request, &response ) ?
           (int64_t)response.result : 0;
}
static int32_t bridge_remote_file_size( void *, const char *name )
{ return bridge_remote_text_integer( 15, name ); }
static int64_t bridge_remote_timestamp( void *, const char *name )
{ return bridge_remote_text_integer( 16, name ); }
static uint32_t bridge_remote_sync_platforms( void *, const char *name )
{ return bridge_remote_text_integer( 17, name ); }

static int32_t bridge_remote_file_count( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 18;
    return bridge_remote_rpc( &request, &response ) ? response.result : 0;
}

static __thread char bridge_remote_name[LSTEAM_BRIDGE_MAX_TICKET];

static const char *bridge_remote_name_size( void *, int32_t index, int32_t *size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (size) *size = 0;
    request.method = 19;
    request.arg0 = index;
    if (!bridge_remote_rpc( &request, &response ) ||
        response.data_size > sizeof(bridge_remote_name)) return "";
    if (response.data_size)
        memcpy( bridge_remote_name, response.data, response.data_size );
    if (!response.data_size) bridge_remote_name[0] = 0;
    else bridge_remote_name[response.data_size - 1] = 0;
    if (size) *size = response.arg0;
    return bridge_remote_name;
}

static int8_t bridge_remote_quota( void *, uint64_t *total, uint64_t *available )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    if (!bridge_remote_rpc( &request, &response ) || !response.result ||
        response.data_size != 16) return 0;
    if (total) memcpy( total, response.data, 8 );
    if (available) memcpy( available, response.data + 8, 8 );
    return 1;
}

/* RemoteStorage011 used 32-bit quota outputs and predates async file slots. */
static int8_t bridge_remote_quota32( void *, int32_t *total,
                                     int32_t *available )
{
    uint64_t total64 = 0, available64 = 0;
    if (!bridge_remote_quota( NULL, &total64, &available64 )) return 0;
    if (total) *total = total64 > INT32_MAX ? INT32_MAX : (int32_t)total64;
    if (available)
        *available = available64 > INT32_MAX ? INT32_MAX :
                     (int32_t)available64;
    return 1;
}

static int8_t bridge_remote_cloud_bool( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    return bridge_remote_rpc( &request, &response ) && response.result;
}
static int8_t bridge_remote_cloud_account( void * )
{ return bridge_remote_cloud_bool( 21 ); }
static int8_t bridge_remote_cloud_app( void * )
{ return bridge_remote_cloud_bool( 22 ); }

static void bridge_remote_set_cloud_app( void *, int8_t enabled )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 23;
    request.arg0 = enabled != 0;
    bridge_remote_rpc( &request, &response );
}
