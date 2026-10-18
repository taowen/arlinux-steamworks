/* Bionic-side SteamUtils010 data that must come from native Steam. */
static bool bridge_utils_rpc( lsteam_bridge_game_server_request *request,
                              lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_UTILS_CALL, request, response );
}

static int8_t bridge_utils_image_size( void *, int32_t image, uint32_t *width,
                                       uint32_t *height )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = image;
    if (!bridge_utils_rpc( &request, &response ) || !response.result) return 0;
    if (width) *width = response.arg0;
    if (height) *height = response.arg1;
    return 1;
}

static int8_t bridge_utils_image_rgba( void *, int32_t image, uint8_t *data,
                                       int32_t capacity )
{
    if (!data || capacity <= 0 || capacity > 4 * 1024 * 1024) return 0;
    uint32_t offset = 0;
    while (offset < (uint32_t)capacity)
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 1;
        request.arg0 = image;
        request.arg1 = capacity;
        request.arg2 = offset;
        if (!bridge_utils_rpc( &request, &response ) || !response.result ||
            !response.data_size ||
            response.data_size > (uint32_t)capacity - offset) return 0;
        memcpy( data + offset, response.data, response.data_size );
        offset += response.data_size;
    }
    return 1;
}

static uint32_t bridge_utils_u32( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    return bridge_utils_rpc( &request, &response ) ? response.result : 0;
}
static uint32_t bridge_utils_universe( void * ) { return bridge_utils_u32( 2 ); }
static uint32_t bridge_utils_real_time( void * ) { return bridge_utils_u32( 3 ); }
static uint8_t bridge_utils_battery( void * ) { return bridge_utils_u32( 5 ); }
static int8_t bridge_utils_overlay_enabled( void * )
{ return bridge_utils_u32( 6 ) != 0; }
static int8_t bridge_utils_big_picture( void * )
{ return bridge_utils_u32( 7 ) != 0; }
static int8_t bridge_utils_steam_deck( void * )
{ return bridge_utils_u32( 9 ) != 0; }

static __thread char bridge_utils_string[LSTEAM_BRIDGE_MAX_TICKET];

static const char *bridge_utils_string_call( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    if (!bridge_utils_rpc( &request, &response ) ||
        response.data_size > sizeof(bridge_utils_string)) return "";
    if (response.data_size)
        memcpy( bridge_utils_string, response.data, response.data_size );
    if (!response.data_size) bridge_utils_string[0] = 0;
    else bridge_utils_string[response.data_size - 1] = 0;
    return bridge_utils_string;
}
static const char *bridge_utils_country( void * )
{ return bridge_utils_string_call( 4 ); }
static const char *bridge_utils_language( void * )
{ return bridge_utils_string_call( 8 ); }

static int8_t bridge_utils_init_filter( void *, uint32_t options )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 10;
    request.arg0 = options;
    return bridge_utils_rpc( &request, &response ) && response.result;
}

static int32_t bridge_utils_filter_text( void *, uint32_t context,
                                         CSteamID source, const char *input,
                                         char *output, uint32_t output_size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!output || !output_size) return 0;
    request.method = 11;
    request.arg0 = context;
    request.steam_id = bridge_iface_id_value( source );
    bridge_iface_copy_text( request.text, input );
    if (!bridge_utils_rpc( &request, &response ) ||
        response.data_size > output_size) return 0;
    if (response.data_size) memcpy( output, response.data, response.data_size );
    else output[0] = 0;
    return response.result;
}

static void bridge_utils_overlay_position( void *, uint32_t position )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 12;
    request.arg0 = position;
    bridge_utils_rpc( &request, &response );
}
