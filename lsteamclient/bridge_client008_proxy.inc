/* Exact Bionic-side SteamClient008 ABI adapter.
 *
 * Keep every entry explicitly typed and in the order declared by
 * steamworks_sdk_104/isteamclient.h. SteamClient008 predates the shutdown
 * and callback-checker methods present in newer ISteamClient revisions, so
 * it must not share one of their vtables.
 */
static int32_t bridge_client008_create_steam_pipe( void *self )
{
    bridge_trace( "SteamClient008::CreateSteamPipe" );
    return bridge_create_pipe( self );
}

static int8_t bridge_client008_release_steam_pipe( void *self, int32_t pipe )
{
    bridge_trace( "SteamClient008::BReleaseSteamPipe" );
    return bridge_release_pipe( self, pipe );
}

static int32_t bridge_client008_connect_global_user( void *self, int32_t pipe )
{
    bridge_trace( "SteamClient008::ConnectToGlobalUser" );
    return bridge_connect_user( self, pipe );
}

static int32_t bridge_client008_create_local_user( void *self, int32_t *pipe,
                                                   uint32_t account_type )
{
    bridge_trace( "SteamClient008::CreateLocalUser" );
    return bridge_create_local_user( self, pipe, account_type );
}

static void bridge_client008_release_user( void *self, int32_t pipe,
                                           int32_t user )
{
    bridge_trace( "SteamClient008::ReleaseUser" );
    bridge_release_user( self, pipe, user );
}

static void *bridge_client008_get_user( void *self, int32_t user, int32_t pipe,
                                        const char *version )
{
    bridge_trace( "SteamClient008::GetISteamUser" );
    return bridge_get_user( self, user, pipe, version );
}

static void *bridge_client008_get_game_server( void *self, int32_t user,
                                               int32_t pipe,
                                               const char *version )
{
    bridge_trace( "SteamClient008::GetISteamGameServer" );
    return bridge_get_game_server( self, user, pipe, version );
}

static void bridge_client008_set_local_ip_binding( void *self, uint32_t ip,
                                                   uint16_t port )
{
    bridge_trace( "SteamClient008::SetLocalIPBinding" );
    bridge_client_set_local_ipv4( self, ip, port );
}

static void *bridge_client008_get_friends( void *self, int32_t user,
                                           int32_t pipe,
                                           const char *version )
{
    bridge_trace( "SteamClient008::GetISteamFriends" );
    return bridge_get_friends( self, user, pipe, version );
}

static void *bridge_client008_get_utils( void *self, int32_t pipe,
                                         const char *version )
{
    bridge_trace( "SteamClient008::GetISteamUtils" );
    return bridge_get_utils( self, pipe, version );
}

static void *bridge_client008_get_matchmaking( void *, int32_t, int32_t,
                                               const char *version )
{
    bridge_trace( "SteamClient008::GetISteamMatchmaking" );
    if (version && !strcmp( version, "SteamMatchMaking008" ))
        return &bridge_lobby8;
    if (version && !strcmp( version, "SteamMatchMaking009" ))
        return &bridge_lobby;
    return NULL;
}

static void *bridge_client008_get_master_server_updater( void *, int32_t,
                                                         int32_t,
                                                         const char * )
{
    bridge_trace( "SteamClient008::GetISteamMasterServerUpdater" );
    return NULL;
}

static void *bridge_client008_get_matchmaking_servers( void *, int32_t,
                                                       int32_t,
                                                       const char *version )
{
    bridge_trace( "SteamClient008::GetISteamMatchmakingServers" );
    if (version && !strcmp( version, "SteamMatchMakingServers002" ))
        return &bridge_matchmaking_servers;
    return NULL;
}

static void *bridge_client008_get_generic( void *self, int32_t user,
                                           int32_t pipe,
                                           const char *version )
{
    bridge_trace( "SteamClient008::GetISteamGenericInterface" );
    return bridge_get_generic( self, user, pipe, version );
}

static void *bridge_client008_get_user_stats( void *, int32_t, int32_t,
                                              const char *version )
{
    bridge_trace( "SteamClient008::GetISteamUserStats" );
    if (version && !strcmp( version, "STEAMUSERSTATS_INTERFACE_VERSION007" ))
        return &bridge_user_stats7;
    if (version && !strcmp( version, "STEAMUSERSTATS_INTERFACE_VERSION011" ))
        return &bridge_user_stats11;
    if (version && !strcmp( version, "STEAMUSERSTATS_INTERFACE_VERSION012" ))
        return &bridge_user_stats;
    return NULL;
}

static void *bridge_client008_get_apps( void *self, int32_t user, int32_t pipe,
                                        const char *version )
{
    bridge_trace( "SteamClient008::GetISteamApps" );
    return bridge_get_apps( self, user, pipe, version );
}

static void *bridge_client008_get_networking( void *, int32_t, int32_t,
                                              const char *version )
{
    bridge_trace( "SteamClient008::GetISteamNetworking" );
    if (version && !strcmp( version, "SteamNetworking003" ))
        return &bridge_networking3;
    if (version && (!strcmp( version, "SteamNetworking005" ) ||
                    !strcmp( version, "SteamNetworking006" )))
        return &bridge_networking;
    return NULL;
}

static void *bridge_client008_get_remote_storage( void *, int32_t, int32_t,
                                                  const char *version )
{
    bridge_trace( "SteamClient008::GetISteamRemoteStorage" );
    if (version && !strcmp( version,
                            "STEAMREMOTESTORAGE_INTERFACE_VERSION002" ))
        return &bridge_remote_storage2;
    if (version && !strcmp( version,
                            "STEAMREMOTESTORAGE_INTERFACE_VERSION011" ))
        return &bridge_remote_storage11;
    if (version && !strcmp( version,
                            "STEAMREMOTESTORAGE_INTERFACE_VERSION014" ))
        return &bridge_remote_storage;
    return NULL;
}

static void bridge_client008_run_frame( void * )
{
    /* Callback delivery is driven by bridge_Steam_BGetCallback. */
    bridge_trace( "SteamClient008::RunFrame" );
}

static uint32_t bridge_client008_get_ipc_call_count( void * )
{
    bridge_trace( "SteamClient008::GetIPCCallCount" );
    return 0;
}

static void bridge_client008_set_warning_hook(
    void *, void (U_CDECL *callback)(int32_t, const char *) )
{
    /* A function pointer in the Wine process cannot be invoked by the native
     * bridge server. Ignoring it is the safe cross-process behavior. */
    bridge_trace( "SteamClient008::SetWarningMessageHook" );
    (void)callback;
}

static void bridge_init_client008_layout( void **vtable )
{
    vtable[0] = (void *)bridge_client008_create_steam_pipe;
    vtable[1] = (void *)bridge_client008_release_steam_pipe;
    vtable[2] = (void *)bridge_client008_connect_global_user;
    vtable[3] = (void *)bridge_client008_create_local_user;
    vtable[4] = (void *)bridge_client008_release_user;
    vtable[5] = (void *)bridge_client008_get_user;
    vtable[6] = (void *)bridge_client008_get_game_server;
    vtable[7] = (void *)bridge_client008_set_local_ip_binding;
    vtable[8] = (void *)bridge_client008_get_friends;
    vtable[9] = (void *)bridge_client008_get_utils;
    vtable[10] = (void *)bridge_client008_get_matchmaking;
    vtable[11] = (void *)bridge_client008_get_master_server_updater;
    vtable[12] = (void *)bridge_client008_get_matchmaking_servers;
    vtable[13] = (void *)bridge_client008_get_generic;
    vtable[14] = (void *)bridge_client008_get_user_stats;
    vtable[15] = (void *)bridge_client008_get_apps;
    vtable[16] = (void *)bridge_client008_get_networking;
    vtable[17] = (void *)bridge_client008_get_remote_storage;
    vtable[18] = (void *)bridge_client008_run_frame;
    vtable[19] = (void *)bridge_client008_get_ipc_call_count;
    vtable[20] = (void *)bridge_client008_set_warning_hook;
}
