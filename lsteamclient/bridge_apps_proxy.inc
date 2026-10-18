/* Bionic-side read-only SteamApps008 bridge. */
static bool bridge_apps_rpc( lsteam_bridge_game_server_request *request,
                             lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_APPS_CALL, request, response );
}

static uint64_t bridge_apps_integer( uint32_t method, uint32_t argument = 0 )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = argument;
    return bridge_apps_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_apps_subscribed( void * )
{ return bridge_apps_integer( 0 ) != 0; }
static int8_t bridge_apps_low_violence( void * )
{ return bridge_apps_integer( 1 ) != 0; }
static int8_t bridge_apps_cybercafe( void * )
{ return bridge_apps_integer( 2 ) != 0; }
static int8_t bridge_apps_vac_banned( void * )
{ return bridge_apps_integer( 3 ) != 0; }
static int8_t bridge_apps_subscribed_app( void *, uint32_t app_id )
{ return bridge_apps_integer( 6, app_id ) != 0; }
static int8_t bridge_apps_dlc_installed( void *, uint32_t app_id )
{ return bridge_apps_integer( 7, app_id ) != 0; }
static uint32_t bridge_apps_purchase_time( void *, uint32_t app_id )
{ return bridge_apps_integer( 8, app_id ); }
static int8_t bridge_apps_free_weekend( void * )
{ return bridge_apps_integer( 9 ) != 0; }
static int32_t bridge_apps_dlc_count( void * )
{ return (int32_t)bridge_apps_integer( 10 ); }
static int8_t bridge_apps_installed( void *, uint32_t app_id )
{ return bridge_apps_integer( 15, app_id ) != 0; }
static int32_t bridge_apps_build_id( void * )
{ return (int32_t)bridge_apps_integer( 19 ); }
static int8_t bridge_apps_family_sharing( void * )
{ return bridge_apps_integer( 21 ) != 0; }

static __thread char bridge_apps_string[LSTEAM_BRIDGE_MAX_TICKET];

static const char *bridge_apps_string_call( uint32_t method,
                                            const char *argument = NULL )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    if (argument) bridge_iface_copy_text( request.text, argument );
    if (!bridge_apps_rpc( &request, &response ) ||
        response.data_size > sizeof(bridge_apps_string))
        return "";
    if (response.data_size)
        memcpy( bridge_apps_string, response.data, response.data_size );
    if (!response.data_size) bridge_apps_string[0] = 0;
    else bridge_apps_string[response.data_size - 1] = 0;
    return bridge_apps_string;
}

static const char *bridge_apps_current_language( void * )
{ return bridge_apps_string_call( 4 ); }
static const char *bridge_apps_available_languages( void * )
{ return bridge_apps_string_call( 5 ); }
static const char *bridge_apps_launch_query( void *, const char *key )
{ return bridge_apps_string_call( 17, key ); }

static int8_t bridge_apps_dlc_data(
    void *, int32_t index, uint32_t *app_id, int8_t *available,
    char *name, int32_t name_size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    request.arg0 = index;
    if (!bridge_apps_rpc( &request, &response )) return 0;
    if (app_id) *app_id = response.arg0;
    if (available) *available = response.arg1 != 0;
    if (name && name_size > 0)
    {
        uint32_t copy = response.data_size;
        if (copy >= (uint32_t)name_size) copy = name_size - 1;
        if (copy) memcpy( name, response.data, copy );
        name[copy] = 0;
    }
    return response.result != 0;
}

static int8_t bridge_apps_current_beta( void *, char *name, int32_t size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 12;
    if (!bridge_apps_rpc( &request, &response )) return 0;
    if (name && size > 0)
    {
        uint32_t copy = response.data_size;
        if (copy >= (uint32_t)size) copy = size - 1;
        if (copy) memcpy( name, response.data, copy );
        name[copy] = 0;
    }
    return response.result != 0;
}

static uint32_t bridge_apps_installed_depots(
    void *, uint32_t app_id, uint32_t *depots, uint32_t maximum )
{
    if (maximum && !depots) return 0;
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 13;
    request.arg0 = app_id;
    request.arg1 = maximum;
    if (!bridge_apps_rpc( &request, &response ) ||
        response.data_size % sizeof(uint32_t) ||
        response.data_size > maximum * sizeof(uint32_t))
        return 0;
    if (response.data_size) memcpy( depots, response.data, response.data_size );
    return response.result;
}

/* SteamApps005 omitted the AppID argument from GetInstalledDepots. */
static uint32_t bridge_apps5_installed_depots(
    void *iface, uint32_t *depots, uint32_t maximum )
{
    const char *value = getenv( "SteamAppId" );
    if (!value || !*value) value = getenv( "SteamGameId" );
    uint32_t app_id = value ? strtoul( value, NULL, 10 ) : 0;
    return bridge_apps_installed_depots(
        iface, app_id, depots, maximum );
}

static uint32_t bridge_apps_install_dir(
    void *, uint32_t app_id, char *folder, uint32_t size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 14;
    request.arg0 = app_id;
    if (!bridge_apps_rpc( &request, &response )) return 0;
    if (folder && size)
    {
        uint32_t copy = response.data_size;
        if (copy >= size) copy = size - 1;
        if (copy) memcpy( folder, response.data, copy );
        folder[copy] = 0;
    }
    return response.result;
}

static CSteamID bridge_apps_owner( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    CSteamID owner = {};
    request.method = 16;
    uint64_t value = bridge_apps_rpc( &request, &response ) ?
                     response.steam_id : 0;
    memcpy( &owner, &value, sizeof(value) );
    return owner;
}

static int8_t bridge_apps_dlc_progress(
    void *, uint32_t app_id, uint64_t *downloaded, uint64_t *total )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 18;
    request.arg0 = app_id;
    if (!bridge_apps_rpc( &request, &response ) || response.data_size != 16)
        return 0;
    if (downloaded) memcpy( downloaded, response.data, 8 );
    if (total) memcpy( total, response.data + 8, 8 );
    return response.result != 0;
}

static int32_t bridge_apps_launch_command( void *, char *command, int32_t size )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    if (!bridge_apps_rpc( &request, &response )) return 0;
    if (command && size > 0)
    {
        uint32_t copy = response.data_size;
        if (copy >= (uint32_t)size) copy = size - 1;
        if (copy) memcpy( command, response.data, copy );
        command[copy] = 0;
    }
    return (int32_t)response.result;
}

static int8_t bridge_apps_timed_trial(
    void *, uint32_t *allowed, uint32_t *played )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 22;
    if (!bridge_apps_rpc( &request, &response )) return 0;
    if (allowed) *allowed = response.arg0;
    if (played) *played = response.arg1;
    return response.result != 0;
}

static uint64_t bridge_apps_file_details( void *, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 23;
    bridge_iface_copy_text( request.text, name );
    return bridge_apps_rpc( &request, &response ) ? response.result : 0;
}
