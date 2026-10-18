/* Bionic-side SteamVideo and SteamParentalSettings proxies. */
static bool bridge_video_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_VIDEO_CALL, request, response );
}

static void bridge_video_url( void *, uint32_t app_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = app_id;
    bridge_video_rpc( &request, &response );
}

static int8_t bridge_video_broadcasting( void *, int32_t *viewers )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    if (!bridge_video_rpc( &request, &response )) {
        if (viewers) *viewers = 0;
        return 0;
    }
    if (viewers) *viewers = (int32_t)response.arg0;
    return response.result != 0;
}

static void bridge_video_opf_settings( void *, uint32_t app_id )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    request.arg0 = app_id;
    bridge_video_rpc( &request, &response );
}

static int8_t bridge_video_opf_string(
    void *, uint32_t app_id, char *output, int32_t *size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (!size || *size < 0) return 0;
    uint32_t capacity = *size;
    if (capacity > LSTEAM_BRIDGE_MAX_TICKET)
        capacity = LSTEAM_BRIDGE_MAX_TICKET;
    request.method = 3;
    request.arg0 = app_id;
    request.arg1 = capacity;
    if (!bridge_video_rpc( &request, &response )) return 0;
    if (output && capacity) {
        uint32_t copy = response.data_size;
        if (copy > capacity) copy = capacity;
        if (copy) memcpy( output, response.data, copy );
        if (!copy) output[0] = 0;
        else output[copy - 1] = 0;
    }
    *size = (int32_t)response.arg0;
    return response.result != 0;
}

static bool bridge_parental_rpc( uint32_t method, uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = argument;
    return bridge_iface_rpc( LSTEAM_BRIDGE_PARENTAL_CALL,
                             &request, &response ) && response.result;
}

static int8_t bridge_parental_lock_enabled( void * )
{ return bridge_parental_rpc( 0 ); }
static int8_t bridge_parental_lock_locked( void * )
{ return bridge_parental_rpc( 1 ); }
static int8_t bridge_parental_app_blocked( void *, uint32_t app_id )
{ return bridge_parental_rpc( 2, app_id ); }
static int8_t bridge_parental_app_in_list( void *, uint32_t app_id )
{ return bridge_parental_rpc( 3, app_id ); }
static int8_t bridge_parental_feature_blocked( void *, uint32_t feature )
{ return bridge_parental_rpc( 4, feature ); }
static int8_t bridge_parental_feature_in_list( void *, uint32_t feature )
{ return bridge_parental_rpc( 5, feature ); }
