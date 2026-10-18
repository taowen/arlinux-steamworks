#include "protocol.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

struct bridge_proxy_object { void **vtable; };

static struct lsteam_bridge_response bridge_status;
static bridge_proxy_object bridge_client, bridge_client008, bridge_client009;
static bridge_proxy_object bridge_client13;
static bridge_proxy_object bridge_client14;
static bridge_proxy_object bridge_client15, bridge_client16, bridge_client17;
static bridge_proxy_object bridge_client18, bridge_client19, bridge_client21;
static bridge_proxy_object bridge_client23;
static bridge_proxy_object bridge_user, bridge_user11, bridge_user12;
static bridge_proxy_object bridge_user13, bridge_user14, bridge_user15;
static bridge_proxy_object bridge_friends;
static bridge_proxy_object bridge_friends13, bridge_friends14;
static bridge_proxy_object bridge_game_server, bridge_game_server12;
static bridge_proxy_object bridge_game_server15;
static bridge_proxy_object bridge_utils, bridge_utils5, bridge_apps, bridge_apps5, bridge_generic;
static bridge_proxy_object bridge_matchmaking_servers;
static bridge_proxy_object bridge_lobby, bridge_lobby8;
static bridge_proxy_object bridge_remote_storage, bridge_remote_storage2;
static bridge_proxy_object bridge_remote_storage11;
static bridge_proxy_object bridge_networking, bridge_networking3;
static bridge_proxy_object bridge_game_server_stats;
static bridge_proxy_object bridge_user_stats, bridge_user_stats11, bridge_user_stats7;
static bridge_proxy_object bridge_ugc14, bridge_ugc15;
static bridge_proxy_object bridge_http, bridge_http1;
static bridge_proxy_object bridge_inventory, bridge_inventory1, bridge_inventory2;
static bridge_proxy_object bridge_game_search;
static bridge_proxy_object bridge_app_list;
static bridge_proxy_object bridge_video, bridge_parental;
static bridge_proxy_object bridge_network_messages;
static bridge_proxy_object bridge_network_sockets, bridge_network_sockets13;
static bridge_proxy_object bridge_network_fake_udp;
static bridge_proxy_object bridge_network_utils3, bridge_network_utils4;
static bridge_proxy_object bridge_html, bridge_app_ticket;
static void *bridge_client_vtable[64], *bridge_client008_vtable[21];
static void *bridge_client009_vtable[22];
static void *bridge_client13_vtable[64];
static void *bridge_client14_vtable[64], *bridge_client15_vtable[64];
static void *bridge_client16_vtable[64], *bridge_client17_vtable[64];
static void *bridge_client18_vtable[64], *bridge_client19_vtable[64];
static void *bridge_client21_vtable[64];
static void *bridge_client23_vtable[64], *bridge_user_vtable[128];
static void *bridge_user11_vtable[32], *bridge_user12_vtable[32];
static void *bridge_user13_vtable[32], *bridge_user14_vtable[32];
static void *bridge_user15_vtable[32];
static void *bridge_friends_vtable[128], *bridge_friends13_vtable[128];
static void *bridge_friends14_vtable[128], *bridge_utils_vtable[128];
static void *bridge_utils5_vtable[23];
static void *bridge_apps_vtable[128], *bridge_apps5_vtable[128];
static void *bridge_generic_vtable[128];
static void *bridge_matchmaking_servers_vtable[32];
static void *bridge_lobby_vtable[64], *bridge_lobby8_vtable[36];
static void *bridge_remote_storage_vtable[64];
static void *bridge_remote_storage11_vtable[64];
static void *bridge_remote_storage2_vtable[7];
static void *bridge_networking_vtable[32], *bridge_networking3_vtable[20];
static void *bridge_game_server_stats_vtable[16];
static void *bridge_user_stats_vtable[64], *bridge_user_stats11_vtable[64];
static void *bridge_user_stats7_vtable[30];
static void *bridge_ugc14_vtable[96], *bridge_ugc15_vtable[96];
static void *bridge_http_vtable[32], *bridge_http1_vtable[16];
static void *bridge_inventory_vtable[48];
static void *bridge_inventory1_vtable[32], *bridge_inventory2_vtable[40];
static void *bridge_game_search_vtable[16];
static void *bridge_app_list_vtable[8];
static void *bridge_video_vtable[8], *bridge_parental_vtable[8];
static void *bridge_network_messages_vtable[8];
static void *bridge_network_sockets_vtable[48];
static void *bridge_network_sockets13_vtable[48];
static void *bridge_network_fake_udp_vtable[4];
static void *bridge_network_utils3_vtable[32];
static void *bridge_network_utils4_vtable[32];
static void *bridge_html_vtable[48], *bridge_app_ticket_vtable[1];
static void *bridge_game_server_vtable[64];
static void *bridge_game_server12_vtable[64];
static void *bridge_game_server15_vtable[64];
static void *bridge_user23_vtable[128];
static bridge_proxy_object bridge_user23;
static bool bridge_proxy_ready;
static bool bridge_rpc_connect( int *fd, const char **path );

static void bridge_trace( const char *format, ... )
{
    const char *path = getenv( "LSTEAM_BRIDGE_TRACE" );
    if (!path || !*path) return;
    FILE *file = fopen( path, "a" );
    if (!file) return;
    va_list args;
    va_start( args, format );
    vfprintf( file, format, args );
    va_end( args );
    fputc( '\n', file );
    fclose( file );
}

static bool bridge_query_status(void)
{
    const char *path = getenv( "LSTEAM_BRIDGE_SOCKET" );
    if (!path || !*path) path = LSTEAM_BRIDGE_DEFAULT_SOCKET;
    if (strlen( path ) >= sizeof(((sockaddr_un *)0)->sun_path)) return false;

    int fd = socket( AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0 );
    if (fd < 0) return false;
    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    memcpy( address.sun_path, path, strlen( path ) + 1 );
    if (connect( fd, (sockaddr *)&address,
                 offsetof(sockaddr_un, sun_path) + strlen( path ) + 1 ) < 0)
    {
        close( fd );
        return false;
    }
    ucred credentials = {};
    socklen_t credentials_size = sizeof(credentials);
    if (getsockopt( fd, SOL_SOCKET, SO_PEERCRED, &credentials, &credentials_size ) < 0 ||
        credentials.uid != getuid())
    {
        close( fd );
        return false;
    }
    lsteam_bridge_request request = {
        LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION, LSTEAM_BRIDGE_STATUS, 1
    };
    memset( &bridge_status, 0, sizeof(bridge_status) );
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              recv( fd, &bridge_status, sizeof(bridge_status), 0 ) == sizeof(bridge_status);
    close( fd );
    ok = ok && bridge_status.magic == LSTEAM_BRIDGE_MAGIC &&
         bridge_status.version == LSTEAM_BRIDGE_VERSION && !bridge_status.status;
    bridge_trace( "status ok=%d pipe=%d user=%d online=%d steamid=%llu", ok,
                  bridge_status.steam_pipe, bridge_status.steam_user,
                  bridge_status.logged_on,
                  (unsigned long long)bridge_status.steam_id );
    return ok;
}

static uintptr_t bridge_zero( void *, ... )
{
    void *return_address = __builtin_return_address( 0 );
    Dl_info info = {};
    const char *name = NULL;
    if (dladdr( return_address, &info )) name = info.dli_sname;
    if (name && strstr( name, "RunFrame" )) return 0;

    static unsigned int trace_count;
    unsigned int index = __atomic_fetch_add( &trace_count, 1, __ATOMIC_RELAXED );
    if (index < 4096)
    {
        if (name) bridge_trace( "zero %s", name );
        else bridge_trace( "zero caller=%p", return_address );
    }
    return 0;
}
static uintptr_t bridge_true( void *, ... ) { return 1; }
static int32_t bridge_create_pipe( void * ) { return bridge_status.steam_pipe; }
static int8_t bridge_release_pipe( void *, int32_t ) { return 1; }
static int32_t bridge_create_local_user( void *, int32_t *pipe, uint32_t account_type )
{
    if (pipe) *pipe = bridge_status.steam_pipe;
    bridge_trace( "CreateLocalUser account_type=%u", account_type );
    return bridge_status.steam_user;
}
static int32_t bridge_connect_user( void *, int32_t ) { return bridge_status.steam_user; }
static void bridge_release_user( void *, int32_t, int32_t ) {}
static int32_t bridge_get_hsteamuser( void * ) { return bridge_status.steam_user; }
static int8_t bridge_logged_on( void * ) { return bridge_status.logged_on; }
#include "bridge_auth_proxy.inc"
#include "bridge_user_friends_proxy.inc"
#include "bridge_user_voice_proxy.inc"
#include "bridge_client_proxy.inc"
#include "bridge_html_proxy.inc"
#include "bridge_utils_proxy.inc"
#include "bridge_apps_proxy.inc"
#include "bridge_remote_storage_proxy.inc"
#include "bridge_networking_proxy.inc"
#include "bridge_gameserver_stats_proxy.inc"
#include "bridge_user_stats_proxy.inc"
#include "bridge_ugc_proxy.inc"
#include "bridge_http_proxy.inc"
#include "bridge_inventory_proxy.inc"
#include "bridge_gamesearch_proxy.inc"
#include "bridge_applist_proxy.inc"
#include "bridge_video_parental_proxy.inc"
#include "bridge_networking_messages_proxy.inc"
#include "bridge_networking_sockets_proxy.inc"
#include "bridge_networking_utils_proxy.inc"
#include "bridge_lobby_proxy.inc"
#include "bridge_gameserver_proxy.inc"
#include "bridge_matchmaking_proxy.inc"
static uint64_t bridge_get_steamid( void * ) { return bridge_status.steam_id; }
static const char *bridge_empty_string( void *, ... ) { return ""; }
static const char *bridge_language( void * ) { return "english"; }
static uint32_t bridge_get_appid( void * )
{
    const char *value = getenv( "SteamAppId" );
    return value ? strtoul( value, NULL, 10 ) : 0;
}
static uint32_t bridge_server_time( void * ) { return (uint32_t)time( NULL ); }
static uint64_t bridge_fake_api_call( void *, ... ) { return 1; }
static int8_t bridge_api_completed( void *, uint64_t, int8_t *failed )
{
    if (failed) *failed = 0;
    return 1;
}
static int8_t bridge_get_api_result( void *, uint64_t, void *callback, int32_t size,
                                     int32_t, int8_t *failed )
{
    if (failed) *failed = 0;
    if (callback && size > 0)
    {
        memset( callback, 0, size );
        if (size >= (int32_t)sizeof(uint32_t)) *(uint32_t *)callback = 1;
    }
    return 1;
}
static int8_t bridge_is_source_app( void *, uint32_t appid )
{
    uint32_t current = bridge_get_appid( NULL );
    int8_t allowed = appid == current || appid == 224260 || appid == 215 ||
                     appid == 220 || appid == 243750;
    bridge_trace( "BIsSubscribedApp appid=%u allowed=%d", appid, allowed );
    return allowed;
}
static void *bridge_get_user( void *, int32_t, int32_t, const char *version )
{
    bridge_trace( "GetISteamUser %s", version ? version : "(null)" );
    if (version && !strcmp( version, "SteamUser011" )) return &bridge_user11;
    if (version && !strcmp( version, "SteamUser012" )) return &bridge_user12;
    if (version && !strcmp( version, "SteamUser013" )) return &bridge_user13;
    if (version && !strcmp( version, "SteamUser014" )) return &bridge_user14;
    if (version && !strcmp( version, "SteamUser015" )) return &bridge_user15;
    if (version && !strcmp(version, "SteamUser023")) return &bridge_user23;
    return &bridge_user;
}
static void *bridge_get_friends( void *, int32_t, int32_t, const char *version )
{
    bridge_trace( "GetISteamFriends %s", version ? version : "(null)" );
    if (version && (!strcmp( version, "SteamFriends012" ) ||
                    !strcmp( version, "SteamFriends013" )))
        return &bridge_friends13;
    if (version && !strcmp( version, "SteamFriends014" ))
        return &bridge_friends14;
    return &bridge_friends;
}
static void *bridge_get_apps( void *, int32_t, int32_t, const char *version )
{
    bridge_trace( "GetISteamApps %s", version ? version : "(null)" );
    if (version && !strcmp( version, "STEAMAPPS_INTERFACE_VERSION001" ))
        return &bridge_generic;
    if (version && !strcmp( version, "STEAMAPPS_INTERFACE_VERSION005" ))
        return &bridge_apps5;
    return &bridge_apps;
}
static void *bridge_get_utils( void *, int32_t, const char *version )
{
    bridge_trace( "GetISteamUtils %s", version ? version : "(null)" );
    if (version && !strcmp( version, "SteamUtils005" )) return &bridge_utils5;
    return &bridge_utils;
}
static void *bridge_get_game_server( void *, int32_t, int32_t, const char *version )
{
    bridge_trace( "GetISteamGameServer %s", version ? version : "(null)" );
    if (version && !strcmp( version, "SteamGameServer012" )) return &bridge_game_server12;
    if (version && !strcmp( version, "SteamGameServer013" )) return &bridge_game_server;
    if (version && (!strcmp( version, "SteamGameServer014" ) ||
                    !strcmp( version, "SteamGameServer015" )))
        return &bridge_game_server15;
    bridge_trace( "unsupported GameServer ABI %s", version ? version : "(null)" );
    return &bridge_generic;
}
static void *bridge_get_generic( void *, int32_t, int32_t, const char *version )
{
    bridge_trace( "GetInterface %s", version ? version : "(null)" );
    if (version && !strcmp( version,
                            "STEAMAPPTICKET_INTERFACE_VERSION001" ))
        return &bridge_app_ticket;
    if (version && !strncmp( version, "SteamUtils", 10 ))
        return bridge_get_utils( NULL, 0, version );
    if (version && !strncmp( version, "SteamUser", 9 ))
        return bridge_get_user( NULL, 0, 0, version );
    if (version && !strncmp( version, "SteamFriends", 12 ))
        return bridge_get_friends( NULL, 0, 0, version );
    if (version && !strncmp( version, "STEAMAPPS", 9 ))
        return bridge_get_apps( NULL, 0, 0, version );
    if (version && !strcmp( version, "SteamMatchMakingServers002" ))
        return &bridge_matchmaking_servers;
    if (version && !strcmp( version, "SteamMatchMaking008" ))
        return &bridge_lobby8;
    if (version && !strcmp( version, "SteamMatchMaking009" ))
        return &bridge_lobby;
    if (version && !strcmp( version, "STEAMREMOTESTORAGE_INTERFACE_VERSION014" ))
        return &bridge_remote_storage;
    if (version && !strcmp( version, "STEAMREMOTESTORAGE_INTERFACE_VERSION002" ))
        return &bridge_remote_storage2;
    if (version && !strcmp( version, "STEAMREMOTESTORAGE_INTERFACE_VERSION011" ))
        return &bridge_remote_storage11;
    if (version && !strcmp( version, "SteamNetworking003" ))
        return &bridge_networking3;
    if (version && (!strcmp( version, "SteamNetworking006" ) ||
                    !strcmp( version, "SteamNetworking005" )))
        return &bridge_networking;
    if (version && !strcmp( version, "SteamGameServerStats001" ))
        return &bridge_game_server_stats;
    if (version && !strcmp( version, "STEAMUSERSTATS_INTERFACE_VERSION012" ))
        return &bridge_user_stats;
    if (version && !strcmp( version, "STEAMUSERSTATS_INTERFACE_VERSION007" ))
        return &bridge_user_stats7;
    if (version && !strcmp( version, "STEAMUSERSTATS_INTERFACE_VERSION011" ))
        return &bridge_user_stats11;
    if (version && !strcmp( version, "STEAMUGC_INTERFACE_VERSION014" ))
        return &bridge_ugc14;
    if (version && !strcmp( version, "STEAMUGC_INTERFACE_VERSION015" ))
        return &bridge_ugc15;
    if (version && (!strcmp( version, "STEAMHTTP_INTERFACE_VERSION003" ) ||
                    !strcmp( version, "STEAMHTTP_INTERFACE_VERSION002" )))
        return &bridge_http;
    if (version && !strcmp( version, "STEAMHTTP_INTERFACE_VERSION001" ))
        return &bridge_http1;
    if (version && !strcmp( version, "STEAMINVENTORY_INTERFACE_V003" ))
        return &bridge_inventory;
    if (version && !strcmp( version, "STEAMINVENTORY_INTERFACE_V002" ))
        return &bridge_inventory2;
    if (version && !strcmp( version, "STEAMINVENTORY_INTERFACE_V001" ))
        return &bridge_inventory1;
    if (version && !strcmp( version, "SteamMatchGameSearch001" ))
        return &bridge_game_search;
    if (version && !strcmp( version, "STEAMAPPLIST_INTERFACE_VERSION001" ))
        return &bridge_app_list;
    if (version && (!strcmp( version, "STEAMVIDEO_INTERFACE_V001" ) ||
                    !strcmp( version, "STEAMVIDEO_INTERFACE_V002" ) ||
                    !strcmp( version, "STEAMVIDEO_INTERFACE_V007" )))
        return &bridge_video;
    if (version && !strcmp( version,
                            "STEAMPARENTALSETTINGS_INTERFACE_VERSION001" ))
        return &bridge_parental;
    if (version && !strcmp( version, "SteamNetworkingMessages002" ))
        return &bridge_network_messages;
    if (version && !strcmp( version, "SteamNetworkingSockets012" ))
        return &bridge_network_sockets;
    if (version && !strcmp( version, "SteamNetworkingSockets013" ))
        return &bridge_network_sockets13;
    if (version && !strcmp( version, "SteamNetworkingUtils003" ))
        return &bridge_network_utils3;
    if (version && !strcmp( version, "SteamNetworkingUtils004" ))
        return &bridge_network_utils4;
    if (version && !strncmp( version, "SteamGameServer", 15 ))
        return bridge_get_game_server( NULL, 0, 0, version );
    if (version && !strcmp( version, "STEAMHTMLSURFACE_INTERFACE_VERSION_005" ))
    {
        bridge_trace( "HTML interface enabled (stub)" );
        return &bridge_html;
    }
    return &bridge_generic;
}

#include "bridge_client008_proxy.inc"
#include "bridge_appticket_proxy.inc"

/* SteamClient009 inserts GetISteamGameServerStats before GetISteamApps.
 * Reusing a newer ISteamClient vtable shifts every following method and makes
 * Fallout: New Vegas call the wrong functions. */
static void *bridge_client009_get_game_server_stats( void *self, int32_t user,
                                                      int32_t pipe,
                                                      const char *version )
{
    bridge_trace( "SteamClient009::GetISteamGameServerStats" );
    return bridge_get_generic( self, user, pipe, version );
}

static void bridge_init_client009_layout( void **vtable )
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
    vtable[15] = (void *)bridge_client009_get_game_server_stats;
    vtable[16] = (void *)bridge_client008_get_apps;
    vtable[17] = (void *)bridge_client008_get_networking;
    vtable[18] = (void *)bridge_client008_get_remote_storage;
    vtable[19] = (void *)bridge_client008_run_frame;
    vtable[20] = (void *)bridge_client008_get_ipc_call_count;
    vtable[21] = (void *)bridge_client008_set_warning_hook;
}

static void bridge_fill_vtable( void **vtable, size_t count )
{
    for (size_t i = 0; i < count; ++i) vtable[i] = (void *)bridge_zero;
}

static void bridge_init_client_layout( void **vtable, unsigned int version )
{
    vtable[0] = (void *)bridge_create_pipe;
    vtable[1] = (void *)bridge_release_pipe;
    vtable[2] = (void *)bridge_connect_user;
    vtable[3] = (void *)bridge_create_local_user;
    vtable[4] = (void *)bridge_release_user;
    vtable[5] = (void *)bridge_get_user;
    vtable[6] = (void *)bridge_get_game_server;
    vtable[8] = (void *)bridge_get_friends;
    vtable[9] = (void *)bridge_get_utils;
    for (size_t i = 10; i <= 14; ++i)
        vtable[i] = (void *)bridge_get_generic;
    vtable[15] = (void *)bridge_get_apps;
    for (size_t i = 16; i <= 18; ++i)
        vtable[i] = (void *)bridge_get_generic;

    if (version >= 18 && version <= 21)
        vtable[19] = (void *)bridge_get_generic; /* GameSearch */

    unsigned int shutdown_slot =
        version >= 18 && version <= 21 ? 23 : 22;
    vtable[shutdown_slot] = (void *)bridge_true;

    switch (version)
    {
    case 13:
        for (size_t i = 23; i <= 29; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 14:
        for (size_t i = 23; i <= 28; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 15:
        for (size_t i = 23; i <= 29; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 16:
        for (size_t i = 23; i <= 30; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 17:
        for (size_t i = 23; i <= 30; ++i)
            vtable[i] = (void *)bridge_get_generic;
        for (size_t i = 34; i <= 36; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 18:
        for (size_t i = 24; i <= 31; ++i)
            vtable[i] = (void *)bridge_get_generic;
        for (size_t i = 35; i <= 39; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 19:
    case 20:
        for (size_t i = 24; i <= 31; ++i)
            vtable[i] = (void *)bridge_get_generic;
        for (size_t i = 35; i <= 40; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 21:
        for (size_t i = 24; i <= 29; ++i)
            vtable[i] = (void *)bridge_get_generic;
        for (size_t i = 33; i <= 38; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    case 23:
        for (size_t i = 23; i <= 27; ++i)
            vtable[i] = (void *)bridge_get_generic;
        for (size_t i = 31; i <= 36; ++i)
            vtable[i] = (void *)bridge_get_generic;
        break;
    }
}

static void bridge_init_objects(void)
{
    if (bridge_proxy_ready) return;
    bridge_fill_vtable( bridge_client_vtable, ARRAY_SIZE(bridge_client_vtable) );
    static_assert( ARRAY_SIZE(bridge_client008_vtable) == 21,
                   "SteamClient008 must have exactly 21 ABI slots" );
    static_assert( ARRAY_SIZE(bridge_client009_vtable) == 22,
                   "SteamClient009 must have exactly 22 ABI slots" );
    bridge_fill_vtable( bridge_client008_vtable,
                        ARRAY_SIZE(bridge_client008_vtable) );
    bridge_fill_vtable( bridge_client009_vtable,
                        ARRAY_SIZE(bridge_client009_vtable) );
    bridge_fill_vtable( bridge_client13_vtable, ARRAY_SIZE(bridge_client13_vtable) );
    bridge_fill_vtable( bridge_client14_vtable, ARRAY_SIZE(bridge_client14_vtable) );
    bridge_fill_vtable( bridge_client15_vtable, ARRAY_SIZE(bridge_client15_vtable) );
    bridge_fill_vtable( bridge_client16_vtable, ARRAY_SIZE(bridge_client16_vtable) );
    bridge_fill_vtable( bridge_client17_vtable, ARRAY_SIZE(bridge_client17_vtable) );
    bridge_fill_vtable( bridge_client18_vtable, ARRAY_SIZE(bridge_client18_vtable) );
    bridge_fill_vtable( bridge_client19_vtable, ARRAY_SIZE(bridge_client19_vtable) );
    bridge_fill_vtable( bridge_client21_vtable, ARRAY_SIZE(bridge_client21_vtable) );
    bridge_fill_vtable( bridge_client23_vtable, ARRAY_SIZE(bridge_client23_vtable) );
    bridge_fill_vtable( bridge_user_vtable, ARRAY_SIZE(bridge_user_vtable) );
    bridge_fill_vtable( bridge_user11_vtable, ARRAY_SIZE(bridge_user11_vtable) );
    bridge_fill_vtable( bridge_user12_vtable, ARRAY_SIZE(bridge_user12_vtable) );
    bridge_fill_vtable( bridge_user13_vtable, ARRAY_SIZE(bridge_user13_vtable) );
    bridge_fill_vtable( bridge_user14_vtable, ARRAY_SIZE(bridge_user14_vtable) );
    bridge_fill_vtable( bridge_user15_vtable, ARRAY_SIZE(bridge_user15_vtable) );
    bridge_fill_vtable( bridge_friends_vtable, ARRAY_SIZE(bridge_friends_vtable) );
    bridge_fill_vtable( bridge_friends13_vtable, ARRAY_SIZE(bridge_friends13_vtable) );
    bridge_fill_vtable( bridge_friends14_vtable, ARRAY_SIZE(bridge_friends14_vtable) );
    bridge_fill_vtable( bridge_utils_vtable, ARRAY_SIZE(bridge_utils_vtable) );
    bridge_fill_vtable( bridge_utils5_vtable, ARRAY_SIZE(bridge_utils5_vtable) );
    bridge_fill_vtable( bridge_apps_vtable, ARRAY_SIZE(bridge_apps_vtable) );
    bridge_fill_vtable( bridge_apps5_vtable, ARRAY_SIZE(bridge_apps5_vtable) );
    bridge_fill_vtable( bridge_generic_vtable, ARRAY_SIZE(bridge_generic_vtable) );
    bridge_fill_vtable( bridge_game_server_vtable, ARRAY_SIZE(bridge_game_server_vtable) );
    bridge_fill_vtable( bridge_game_server12_vtable, ARRAY_SIZE(bridge_game_server12_vtable) );
    bridge_fill_vtable( bridge_matchmaking_servers_vtable,
                        ARRAY_SIZE(bridge_matchmaking_servers_vtable) );
    bridge_fill_vtable( bridge_lobby_vtable, ARRAY_SIZE(bridge_lobby_vtable) );
    bridge_fill_vtable( bridge_lobby8_vtable, ARRAY_SIZE(bridge_lobby8_vtable) );
    bridge_fill_vtable( bridge_remote_storage_vtable,
                        ARRAY_SIZE(bridge_remote_storage_vtable) );
    bridge_fill_vtable( bridge_remote_storage11_vtable,
                        ARRAY_SIZE(bridge_remote_storage11_vtable) );
    bridge_fill_vtable( bridge_remote_storage2_vtable,
                        ARRAY_SIZE(bridge_remote_storage2_vtable) );
    bridge_fill_vtable( bridge_networking_vtable,
                        ARRAY_SIZE(bridge_networking_vtable) );
    bridge_fill_vtable( bridge_networking3_vtable,
                        ARRAY_SIZE(bridge_networking3_vtable) );
    bridge_fill_vtable( bridge_game_server_stats_vtable,
                        ARRAY_SIZE(bridge_game_server_stats_vtable) );
    bridge_fill_vtable( bridge_user_stats_vtable,
                        ARRAY_SIZE(bridge_user_stats_vtable) );
    bridge_fill_vtable( bridge_user_stats11_vtable,
                        ARRAY_SIZE(bridge_user_stats11_vtable) );
    bridge_fill_vtable( bridge_user_stats7_vtable,
                        ARRAY_SIZE(bridge_user_stats7_vtable) );
    bridge_fill_vtable( bridge_ugc14_vtable, ARRAY_SIZE(bridge_ugc14_vtable) );
    bridge_fill_vtable( bridge_ugc15_vtable, ARRAY_SIZE(bridge_ugc15_vtable) );
    bridge_fill_vtable( bridge_http_vtable, ARRAY_SIZE(bridge_http_vtable) );
    bridge_fill_vtable( bridge_http1_vtable,
                        ARRAY_SIZE(bridge_http1_vtable) );
    bridge_fill_vtable( bridge_inventory_vtable,
                        ARRAY_SIZE(bridge_inventory_vtable) );
    bridge_fill_vtable( bridge_inventory1_vtable,
                        ARRAY_SIZE(bridge_inventory1_vtable) );
    bridge_fill_vtable( bridge_inventory2_vtable,
                        ARRAY_SIZE(bridge_inventory2_vtable) );
    bridge_fill_vtable( bridge_game_search_vtable,
                        ARRAY_SIZE(bridge_game_search_vtable) );
    bridge_fill_vtable( bridge_app_list_vtable,
                        ARRAY_SIZE(bridge_app_list_vtable) );
    bridge_fill_vtable( bridge_video_vtable,
                        ARRAY_SIZE(bridge_video_vtable) );
    bridge_fill_vtable( bridge_parental_vtable,
                        ARRAY_SIZE(bridge_parental_vtable) );
    bridge_fill_vtable( bridge_network_messages_vtable,
                        ARRAY_SIZE(bridge_network_messages_vtable) );
    bridge_fill_vtable( bridge_network_sockets_vtable,
                        ARRAY_SIZE(bridge_network_sockets_vtable) );
    bridge_fill_vtable( bridge_network_sockets13_vtable,
                        ARRAY_SIZE(bridge_network_sockets13_vtable) );
    bridge_fill_vtable( bridge_network_fake_udp_vtable,
                        ARRAY_SIZE(bridge_network_fake_udp_vtable) );
    bridge_fill_vtable( bridge_network_utils3_vtable,
                        ARRAY_SIZE(bridge_network_utils3_vtable) );
    bridge_fill_vtable( bridge_network_utils4_vtable,
                        ARRAY_SIZE(bridge_network_utils4_vtable) );
    bridge_fill_vtable( bridge_html_vtable, ARRAY_SIZE(bridge_html_vtable) );
    bridge_fill_vtable( bridge_app_ticket_vtable,
                        ARRAY_SIZE(bridge_app_ticket_vtable) );
    bridge_client.vtable = bridge_client_vtable;
    bridge_client008.vtable = bridge_client008_vtable;
    bridge_client009.vtable = bridge_client009_vtable;
    bridge_client13.vtable = bridge_client13_vtable;
    bridge_client14.vtable = bridge_client14_vtable;
    bridge_client15.vtable = bridge_client15_vtable;
    bridge_client16.vtable = bridge_client16_vtable;
    bridge_client17.vtable = bridge_client17_vtable;
    bridge_client18.vtable = bridge_client18_vtable;
    bridge_client19.vtable = bridge_client19_vtable;
    bridge_client21.vtable = bridge_client21_vtable;
    bridge_client23.vtable = bridge_client23_vtable;
    bridge_user.vtable = bridge_user_vtable;
    bridge_user11.vtable = bridge_user11_vtable;
    bridge_user12.vtable = bridge_user12_vtable;
    bridge_user13.vtable = bridge_user13_vtable;
    bridge_user14.vtable = bridge_user14_vtable;
    bridge_user15.vtable = bridge_user15_vtable;
    bridge_friends.vtable = bridge_friends_vtable;
    bridge_friends13.vtable = bridge_friends13_vtable;
    bridge_friends14.vtable = bridge_friends14_vtable;
    bridge_utils.vtable = bridge_utils_vtable;
    bridge_utils5.vtable = bridge_utils5_vtable;
    bridge_apps.vtable = bridge_apps_vtable;
    bridge_apps5.vtable = bridge_apps5_vtable;
    bridge_generic.vtable = bridge_generic_vtable;
    bridge_game_server.vtable = bridge_game_server_vtable;
    bridge_game_server12.vtable = bridge_game_server12_vtable;
    bridge_game_server15.vtable = bridge_game_server15_vtable;
    bridge_matchmaking_servers.vtable = bridge_matchmaking_servers_vtable;
    bridge_lobby.vtable = bridge_lobby_vtable;
    bridge_lobby8.vtable = bridge_lobby8_vtable;
    bridge_remote_storage.vtable = bridge_remote_storage_vtable;
    bridge_remote_storage11.vtable = bridge_remote_storage11_vtable;
    bridge_remote_storage2.vtable = bridge_remote_storage2_vtable;
    bridge_networking.vtable = bridge_networking_vtable;
    bridge_networking3.vtable = bridge_networking3_vtable;
    bridge_game_server_stats.vtable = bridge_game_server_stats_vtable;
    bridge_user_stats.vtable = bridge_user_stats_vtable;
    bridge_user_stats11.vtable = bridge_user_stats11_vtable;
    bridge_user_stats7.vtable = bridge_user_stats7_vtable;
    bridge_ugc14.vtable = bridge_ugc14_vtable;
    bridge_ugc15.vtable = bridge_ugc15_vtable;
    bridge_http.vtable = bridge_http_vtable;
    bridge_http1.vtable = bridge_http1_vtable;
    bridge_inventory.vtable = bridge_inventory_vtable;
    bridge_inventory1.vtable = bridge_inventory1_vtable;
    bridge_inventory2.vtable = bridge_inventory2_vtable;
    bridge_game_search.vtable = bridge_game_search_vtable;
    bridge_app_list.vtable = bridge_app_list_vtable;
    bridge_video.vtable = bridge_video_vtable;
    bridge_parental.vtable = bridge_parental_vtable;
    bridge_network_messages.vtable = bridge_network_messages_vtable;
    bridge_network_sockets.vtable = bridge_network_sockets_vtable;
    bridge_network_sockets13.vtable = bridge_network_sockets13_vtable;
    bridge_network_fake_udp.vtable = bridge_network_fake_udp_vtable;
    bridge_network_utils3.vtable = bridge_network_utils3_vtable;
    bridge_network_utils4.vtable = bridge_network_utils4_vtable;
    bridge_html.vtable = bridge_html_vtable;
    bridge_app_ticket.vtable = bridge_app_ticket_vtable;

    /* u_ISteamHTMLSurface has two Itanium destructor entries before the
     * actual interface methods. The generated Windows wrapper removes them. */
    bridge_html_vtable[0] = (void *)bridge_html_destructor;
    bridge_html_vtable[1] = (void *)bridge_html_destructor;
    bridge_html_vtable[2] = (void *)bridge_html_init;
    bridge_html_vtable[3] = (void *)bridge_html_shutdown;
    bridge_html_vtable[4] = (void *)bridge_html_create;
    bridge_html_vtable[5] = (void *)bridge_html_remove;
    bridge_html_vtable[7] = (void *)bridge_html_set_size;
    bridge_app_ticket_vtable[0] =
        (void *)bridge_app_ticket_get_ownership_data;

    bridge_init_client008_layout( bridge_client008_vtable );
    bridge_init_client009_layout( bridge_client009_vtable );
    bridge_init_client_layout( bridge_client13_vtable, 13 );
    bridge_init_client_layout( bridge_client14_vtable, 14 );
    bridge_init_client_layout( bridge_client15_vtable, 15 );
    bridge_init_client_layout( bridge_client16_vtable, 16 );
    bridge_init_client_layout( bridge_client17_vtable, 17 );
    bridge_init_client_layout( bridge_client18_vtable, 18 );
    bridge_init_client_layout( bridge_client19_vtable, 19 );
    bridge_init_client_layout( bridge_client_vtable, 20 );
    bridge_init_client_layout( bridge_client21_vtable, 21 );
    bridge_init_client_layout( bridge_client23_vtable, 23 );

    bridge_client13_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client14_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client15_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client16_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client17_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client18_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client19_vtable[7] = (void *)bridge_client_set_local_ipv4;
    bridge_client_vtable[7] = (void *)bridge_client_set_local_ip;
    bridge_client21_vtable[7] = (void *)bridge_client_set_local_ip;
    bridge_client23_vtable[7] = (void *)bridge_client_set_local_ip;
    bridge_client16_vtable[33] = (void *)bridge_client_set_callback_checker;
    bridge_client17_vtable[33] = (void *)bridge_client_set_callback_checker;
    bridge_client18_vtable[34] = (void *)bridge_client_set_callback_checker;
    bridge_client19_vtable[34] = (void *)bridge_client_set_callback_checker;
    bridge_client_vtable[34] = (void *)bridge_client_set_callback_checker;
    bridge_client21_vtable[32] = (void *)bridge_client_set_callback_checker;
    bridge_client23_vtable[30] = (void *)bridge_client_set_callback_checker;

    bridge_user_vtable[0] = (void *)bridge_get_hsteamuser;
    bridge_user_vtable[1] = (void *)bridge_logged_on;
    bridge_user_vtable[13] = (void *)bridge_get_auth_ticket;
    bridge_user_vtable[2] = (void *)bridge_get_steamid;
    bridge_user_vtable[3] = (void *)bridge_initiate_game_connection;
    bridge_user_vtable[4] = (void *)bridge_user_terminate_connection;
    bridge_user_vtable[5] = (void *)bridge_user_track_usage;
    bridge_user_vtable[6] = (void *)bridge_user_data_folder;
    bridge_user_vtable[7] = (void *)bridge_user_start_voice;
    bridge_user_vtable[8] = (void *)bridge_user_stop_voice;
    bridge_user_vtable[9] = (void *)bridge_user_available_voice;
    bridge_user_vtable[10] = (void *)bridge_user_get_voice;
    bridge_user_vtable[11] = (void *)bridge_user_decompress_voice;
    bridge_user_vtable[12] = (void *)bridge_user_voice_sample_rate;
    bridge_user_vtable[14] = (void *)bridge_user_begin_auth;
    bridge_user_vtable[15] = (void *)bridge_user_end_auth;
    bridge_user_vtable[16] = (void *)bridge_user_cancel_ticket;
    bridge_user_vtable[17] = (void *)bridge_user_has_license;
    bridge_user_vtable[18] = (void *)bridge_user_is_behind_nat;
    bridge_user_vtable[19] = (void *)bridge_user_advertise_game;
    bridge_user_vtable[20] = (void *)bridge_user_request_encrypted_ticket;
    bridge_user_vtable[21] = (void *)bridge_user_get_encrypted_ticket;
    bridge_user_vtable[22] = (void *)bridge_user_badge_level;
    bridge_user_vtable[23] = (void *)bridge_user_player_level;
    bridge_user_vtable[24] = (void *)bridge_user_store_auth_url;
    bridge_user_vtable[25] = (void *)bridge_user_phone_verified;
    bridge_user_vtable[26] = (void *)bridge_user_two_factor;
    bridge_user_vtable[27] = (void *)bridge_user_phone_identifying;
    bridge_user_vtable[28] = (void *)bridge_user_phone_requiring_verification;
    bridge_user_vtable[29] = (void *)bridge_user_market_eligibility;
    bridge_user_vtable[30] = (void *)bridge_user_duration_control;
    bridge_user_vtable[31] = (void *)bridge_user_set_duration_state;
    // SteamUser023 inserts GetAuthTicketForWebApi at slot 14; all following
    // methods shift by one. Keep SteamUser011..022 on their original table.
    memcpy(bridge_user23_vtable, bridge_user_vtable, sizeof(bridge_user_vtable));
    for (size_t i = 32; i > 14; --i)
        bridge_user23_vtable[i] = bridge_user_vtable[i - 1];
    bridge_user23_vtable[14] = (void *)bridge_get_web_api_ticket;
    bridge_user23.vtable = bridge_user23_vtable;
    void **legacy_user_tables[] = {
        bridge_user11_vtable, bridge_user12_vtable, bridge_user13_vtable,
        bridge_user14_vtable, bridge_user15_vtable
    };
    for (size_t i = 0; i < ARRAY_SIZE(legacy_user_tables); ++i)
    {
        void **table = legacy_user_tables[i];
        table[0] = (void *)bridge_get_hsteamuser;
        table[1] = (void *)bridge_logged_on;
        table[2] = (void *)bridge_get_steamid;
        table[3] = (void *)bridge_initiate_game_connection;
        table[4] = (void *)bridge_user_terminate_connection;
        table[5] = (void *)bridge_user_track_usage;
        table[6] = (void *)bridge_user_data_folder;
        table[7] = (void *)bridge_user_start_voice;
        table[8] = (void *)bridge_user_stop_voice;
    }
    bridge_user11_vtable[9] = (void *)bridge_user_get_compressed_voice;
    bridge_user11_vtable[10] = (void *)bridge_user_decompress_voice_legacy;
    memcpy( bridge_user12_vtable, bridge_user11_vtable,
            11 * sizeof(*bridge_user12_vtable) );
    bridge_user12_vtable[11] = (void *)bridge_get_auth_ticket;
    bridge_user12_vtable[12] = (void *)bridge_user_begin_auth;
    bridge_user12_vtable[13] = (void *)bridge_user_end_auth;
    bridge_user12_vtable[14] = (void *)bridge_user_cancel_ticket;
    bridge_user12_vtable[15] = (void *)bridge_user_has_license;
    void **legacy_available_tables[] = {
        bridge_user13_vtable, bridge_user14_vtable
    };
    for (size_t i = 0; i < ARRAY_SIZE(legacy_available_tables); ++i)
    {
        void **table = legacy_available_tables[i];
        table[9] = (void *)bridge_user_available_voice_legacy;
        table[10] = (void *)bridge_user_get_voice_legacy;
        table[11] = (void *)bridge_user_decompress_voice_legacy;
        table[12] = (void *)bridge_get_auth_ticket;
        table[13] = (void *)bridge_user_begin_auth;
        table[14] = (void *)bridge_user_end_auth;
        table[15] = (void *)bridge_user_cancel_ticket;
        table[16] = (void *)bridge_user_has_license;
    }
    bridge_user14_vtable[17] = (void *)bridge_user_is_behind_nat;
    bridge_user14_vtable[18] = (void *)bridge_user_advertise_game;
    bridge_user14_vtable[19] = (void *)bridge_user_request_encrypted_ticket;
    bridge_user14_vtable[20] = (void *)bridge_user_get_encrypted_ticket;
    bridge_user15_vtable[9] = (void *)bridge_user_available_voice_legacy;
    bridge_user15_vtable[10] = (void *)bridge_user_get_voice_legacy;
    bridge_user15_vtable[11] = (void *)bridge_user_decompress_voice;
    bridge_user15_vtable[12] = (void *)bridge_user_voice_sample_rate;
    bridge_user15_vtable[13] = (void *)bridge_get_auth_ticket;
    bridge_user15_vtable[14] = (void *)bridge_user_begin_auth;
    bridge_user15_vtable[15] = (void *)bridge_user_end_auth;
    bridge_user15_vtable[16] = (void *)bridge_user_cancel_ticket;
    bridge_user15_vtable[17] = (void *)bridge_user_has_license;
    bridge_user15_vtable[18] = (void *)bridge_user_is_behind_nat;
    bridge_user15_vtable[19] = (void *)bridge_user_advertise_game;
    bridge_user15_vtable[20] = (void *)bridge_user_request_encrypted_ticket;
    bridge_user15_vtable[21] = (void *)bridge_user_get_encrypted_ticket;
    bridge_friends_vtable[0] = (void *)bridge_friends_persona_name;
    bridge_friends_vtable[2] = (void *)bridge_friends_persona_state;
    bridge_friends_vtable[3] = (void *)bridge_friends_count;
    bridge_friends_vtable[4] = (void *)bridge_friends_by_index;
    bridge_friends_vtable[5] = (void *)bridge_friends_relationship;
    bridge_friends_vtable[6] = (void *)bridge_friends_friend_state;
    bridge_friends_vtable[7] = (void *)bridge_friends_friend_name;
    bridge_friends_vtable[8] = (void *)bridge_friends_game_played;
    bridge_friends_vtable[9] = (void *)bridge_empty_string;
    bridge_friends_vtable[10] = (void *)bridge_friends_steam_level;
    bridge_friends_vtable[11] = (void *)bridge_empty_string;
    bridge_friends_vtable[20] = (void *)bridge_empty_string;
    bridge_friends_vtable[21] = (void *)bridge_empty_string;
    bridge_friends_vtable[11] = (void *)bridge_friends_player_nickname;
    bridge_friends_vtable[17] = (void *)bridge_friends_has_friend;
    bridge_friends_vtable[27] = (void *)bridge_friends_voice_speaking;
    bridge_friends_vtable[32] = (void *)bridge_friends_set_played_with;
    bridge_friends_vtable[34] = (void *)bridge_friends_small_avatar;
    bridge_friends_vtable[35] = (void *)bridge_friends_medium_avatar;
    bridge_friends_vtable[36] = (void *)bridge_friends_large_avatar;
    bridge_friends_vtable[37] = (void *)bridge_friends_request_info;
    bridge_friends_vtable[43] = (void *)bridge_friends_set_rich_presence;
    bridge_friends_vtable[44] = (void *)bridge_friends_clear_rich_presence;
    bridge_friends_vtable[45] = (void *)bridge_friends_get_rich_presence;
    bridge_friends_vtable[46] = (void *)bridge_friends_rich_key_count;
    bridge_friends_vtable[47] = (void *)bridge_friends_rich_key_by_index;
    bridge_friends_vtable[48] = (void *)bridge_friends_request_rich_presence;
    bridge_friends_vtable[49] = (void *)bridge_friends_invite;
    bridge_friends13_vtable[0] = (void *)bridge_friends_persona_name;
    bridge_friends13_vtable[2] = (void *)bridge_friends_persona_state;
    bridge_friends13_vtable[3] = (void *)bridge_friends_count;
    bridge_friends13_vtable[4] = (void *)bridge_friends_by_index;
    bridge_friends13_vtable[5] = (void *)bridge_friends_relationship;
    bridge_friends13_vtable[6] = (void *)bridge_friends_friend_state;
    bridge_friends13_vtable[7] = (void *)bridge_friends_friend_name;
    bridge_friends13_vtable[8] = (void *)bridge_friends_game_played;
    bridge_friends13_vtable[10] = (void *)bridge_friends_has_friend;
    bridge_friends13_vtable[20] = (void *)bridge_friends_voice_speaking;
    bridge_friends13_vtable[25] = (void *)bridge_friends_set_played_with;
    bridge_friends13_vtable[27] = (void *)bridge_friends_small_avatar;
    bridge_friends13_vtable[28] = (void *)bridge_friends_medium_avatar;
    bridge_friends13_vtable[29] = (void *)bridge_friends_large_avatar;
    bridge_friends13_vtable[30] = (void *)bridge_friends_request_info;
    bridge_friends13_vtable[36] = (void *)bridge_friends_set_rich_presence;
    bridge_friends13_vtable[37] = (void *)bridge_friends_clear_rich_presence;
    bridge_friends13_vtable[38] = (void *)bridge_friends_get_rich_presence;
    bridge_friends13_vtable[39] = (void *)bridge_friends_rich_key_count;
    bridge_friends13_vtable[40] = (void *)bridge_friends_rich_key_by_index;
    bridge_friends13_vtable[41] = (void *)bridge_friends_request_rich_presence;
    bridge_friends13_vtable[42] = (void *)bridge_friends_invite;
    bridge_friends14_vtable[0] = (void *)bridge_friends_persona_name;
    bridge_friends14_vtable[2] = (void *)bridge_friends_persona_state;
    bridge_friends14_vtable[3] = (void *)bridge_friends_count;
    bridge_friends14_vtable[4] = (void *)bridge_friends_by_index;
    bridge_friends14_vtable[5] = (void *)bridge_friends_relationship;
    bridge_friends14_vtable[6] = (void *)bridge_friends_friend_state;
    bridge_friends14_vtable[7] = (void *)bridge_friends_friend_name;
    bridge_friends14_vtable[8] = (void *)bridge_friends_game_played;
    bridge_friends14_vtable[10] = (void *)bridge_friends_player_nickname;
    bridge_friends14_vtable[11] = (void *)bridge_friends_has_friend;
    bridge_friends14_vtable[21] = (void *)bridge_friends_voice_speaking;
    bridge_friends14_vtable[26] = (void *)bridge_friends_set_played_with;
    bridge_friends14_vtable[28] = (void *)bridge_friends_small_avatar;
    bridge_friends14_vtable[29] = (void *)bridge_friends_medium_avatar;
    bridge_friends14_vtable[30] = (void *)bridge_friends_large_avatar;
    bridge_friends14_vtable[31] = (void *)bridge_friends_request_info;
    bridge_friends14_vtable[37] = (void *)bridge_friends_set_rich_presence;
    bridge_friends14_vtable[38] = (void *)bridge_friends_clear_rich_presence;
    bridge_friends14_vtable[39] = (void *)bridge_friends_get_rich_presence;
    bridge_friends14_vtable[40] = (void *)bridge_friends_rich_key_count;
    bridge_friends14_vtable[41] = (void *)bridge_friends_rich_key_by_index;
    bridge_friends14_vtable[42] = (void *)bridge_friends_request_rich_presence;
    bridge_friends14_vtable[43] = (void *)bridge_friends_invite;
    bridge_utils_vtable[2] = (void *)bridge_utils_universe;
    bridge_utils_vtable[3] = (void *)bridge_utils_real_time;
    bridge_utils_vtable[4] = (void *)bridge_utils_country;
    bridge_utils_vtable[5] = (void *)bridge_utils_image_size;
    bridge_utils_vtable[6] = (void *)bridge_utils_image_rgba;
    bridge_utils_vtable[8] = (void *)bridge_utils_battery;
    bridge_utils_vtable[9] = (void *)bridge_get_appid;
    bridge_utils_vtable[10] = (void *)bridge_utils_overlay_position;
    bridge_utils_vtable[11] = (void *)bridge_api_completed;
    bridge_utils_vtable[13] = (void *)bridge_get_api_result;
    bridge_utils_vtable[17] = (void *)bridge_utils_overlay_enabled;
    bridge_utils_vtable[19] = (void *)bridge_fake_api_call;
    bridge_utils_vtable[23] = (void *)bridge_utils_language;
    bridge_utils_vtable[26] = (void *)bridge_utils_big_picture;
    bridge_utils_vtable[31] = (void *)bridge_utils_init_filter;
    bridge_utils_vtable[32] = (void *)bridge_utils_filter_text;
    bridge_utils_vtable[34] = (void *)bridge_utils_steam_deck;
    memcpy( bridge_utils5_vtable, bridge_utils_vtable,
            ARRAY_SIZE(bridge_utils5_vtable) * sizeof(*bridge_utils5_vtable) );
    bridge_apps_vtable[0] = (void *)bridge_apps_subscribed;
    bridge_apps_vtable[1] = (void *)bridge_apps_low_violence;
    bridge_apps_vtable[2] = (void *)bridge_apps_cybercafe;
    bridge_apps_vtable[3] = (void *)bridge_apps_vac_banned;
    bridge_apps_vtable[4] = (void *)bridge_apps_current_language;
    bridge_apps_vtable[5] = (void *)bridge_apps_available_languages;
    bridge_apps_vtable[6] = (void *)bridge_apps_subscribed_app;
    bridge_apps_vtable[7] = (void *)bridge_apps_dlc_installed;
    bridge_apps_vtable[8] = (void *)bridge_apps_purchase_time;
    bridge_apps_vtable[9] = (void *)bridge_apps_free_weekend;
    bridge_apps_vtable[10] = (void *)bridge_apps_dlc_count;
    bridge_apps_vtable[11] = (void *)bridge_apps_dlc_data;
    bridge_apps_vtable[15] = (void *)bridge_apps_current_beta;
    bridge_apps_vtable[17] = (void *)bridge_apps_installed_depots;
    bridge_apps_vtable[18] = (void *)bridge_apps_install_dir;
    bridge_apps_vtable[19] = (void *)bridge_apps_installed;
    bridge_apps_vtable[20] = (void *)bridge_apps_owner;
    bridge_apps_vtable[21] = (void *)bridge_apps_launch_query;
    bridge_apps_vtable[22] = (void *)bridge_apps_dlc_progress;
    bridge_apps_vtable[23] = (void *)bridge_apps_build_id;
    bridge_apps_vtable[25] = (void *)bridge_apps_file_details;
    bridge_apps_vtable[26] = (void *)bridge_apps_launch_command;
    bridge_apps_vtable[27] = (void *)bridge_apps_family_sharing;
    bridge_apps_vtable[28] = (void *)bridge_apps_timed_trial;
    memcpy( bridge_apps5_vtable, bridge_apps_vtable,
            sizeof(bridge_apps5_vtable) );
    bridge_apps5_vtable[17] = (void *)bridge_apps5_installed_depots;
    bridge_game_server_vtable[0] = (void *)bridge_gs_init;
    bridge_game_server_vtable[1] = (void *)bridge_gs_set_product;
    bridge_game_server_vtable[2] = (void *)bridge_gs_set_description;
    bridge_game_server_vtable[3] = (void *)bridge_gs_set_mod_dir;
    bridge_game_server_vtable[4] = (void *)bridge_gs_set_dedicated;
    bridge_game_server_vtable[5] = (void *)bridge_gs_log_on;
    bridge_game_server_vtable[6] = (void *)bridge_gs_log_on_anonymous;
    bridge_game_server_vtable[7] = (void *)bridge_gs_log_off;
    bridge_game_server_vtable[8] = (void *)bridge_gs_logged_on;
    bridge_game_server_vtable[9] = (void *)bridge_gs_secure;
    bridge_game_server_vtable[10] = (void *)bridge_gs_get_steamid;
    bridge_game_server_vtable[11] = (void *)bridge_gs_restart_requested;
    bridge_game_server_vtable[12] = (void *)bridge_gs_set_max_players;
    bridge_game_server_vtable[13] = (void *)bridge_gs_set_bot_players;
    bridge_game_server_vtable[14] = (void *)bridge_gs_set_server_name;
    bridge_game_server_vtable[15] = (void *)bridge_gs_set_map_name;
    bridge_game_server_vtable[16] = (void *)bridge_gs_set_password;
    bridge_game_server_vtable[17] = (void *)bridge_gs_set_spectator_port;
    bridge_game_server_vtable[18] = (void *)bridge_gs_set_spectator_name;
    bridge_game_server_vtable[19] = (void *)bridge_gs_clear_values;
    bridge_game_server_vtable[20] = (void *)bridge_gs_set_key_value;
    bridge_game_server_vtable[21] = (void *)bridge_gs_set_tags;
    bridge_game_server_vtable[22] = (void *)bridge_gs_set_data;
    bridge_game_server_vtable[23] = (void *)bridge_gs_set_region;
    bridge_game_server_vtable[24] = (void *)bridge_gs_connect_user;
    bridge_game_server_vtable[25] = (void *)bridge_gs_create_unauthenticated;
    bridge_game_server_vtable[26] = (void *)bridge_gs_disconnect_user;
    bridge_game_server_vtable[27] = (void *)bridge_gs_update_user;
    bridge_game_server_vtable[28] = (void *)bridge_gs_get_auth_ticket;
    bridge_game_server_vtable[29] = (void *)bridge_gs_begin_auth;
    bridge_game_server_vtable[30] = (void *)bridge_gs_end_auth;
    bridge_game_server_vtable[31] = (void *)bridge_gs_cancel_ticket;
    bridge_game_server_vtable[32] = (void *)bridge_gs_has_license;
    bridge_game_server_vtable[33] = (void *)bridge_gs_group_status;
    bridge_game_server_vtable[34] = (void *)bridge_gs_gameplay_stats;
    bridge_game_server_vtable[35] = (void *)bridge_gs_reputation;
    bridge_game_server_vtable[36] = (void *)bridge_gs_public_ip;
    bridge_game_server_vtable[37] = (void *)bridge_gs_incoming_packet;
    bridge_game_server_vtable[38] = (void *)bridge_gs_outgoing_packet;
    bridge_game_server_vtable[39] = (void *)bridge_gs_enable_heartbeats;
    bridge_game_server_vtable[40] = (void *)bridge_gs_set_heartbeat_interval;
    bridge_game_server_vtable[41] = (void *)bridge_gs_force_heartbeat;
    bridge_game_server_vtable[42] = (void *)bridge_gs_associate_clan;
    bridge_game_server_vtable[43] = (void *)bridge_gs_compute_compatibility;
    memcpy( bridge_game_server12_vtable, bridge_game_server_vtable,
            sizeof(bridge_game_server_vtable) );
    bridge_game_server12_vtable[36] = (void *)bridge_gs_public_ip_v4;
    /* 014/015 moved deprecated user methods to the end. Adapt them to the
     * stable SteamGameServer013 bridge ABI explicitly. */
    memcpy( bridge_game_server15_vtable, bridge_game_server_vtable,
            24 * sizeof(*bridge_game_server15_vtable) );
    bridge_game_server15_vtable[24] = (void *)bridge_gs_set_advertise;
    bridge_game_server15_vtable[25] = (void *)bridge_gs_get_auth_ticket15;
    bridge_game_server15_vtable[26] = (void *)bridge_gs_begin_auth;
    bridge_game_server15_vtable[27] = (void *)bridge_gs_end_auth;
    bridge_game_server15_vtable[28] = (void *)bridge_gs_cancel_ticket;
    bridge_game_server15_vtable[29] = (void *)bridge_gs_has_license;
    bridge_game_server15_vtable[30] = (void *)bridge_gs_group_status;
    bridge_game_server15_vtable[31] = (void *)bridge_gs_gameplay_stats;
    bridge_game_server15_vtable[32] = (void *)bridge_gs_reputation;
    bridge_game_server15_vtable[33] = (void *)bridge_gs_public_ip;
    bridge_game_server15_vtable[34] = (void *)bridge_gs_incoming_packet;
    bridge_game_server15_vtable[35] = (void *)bridge_gs_outgoing_packet;
    bridge_game_server15_vtable[36] = (void *)bridge_gs_associate_clan;
    bridge_game_server15_vtable[37] = (void *)bridge_gs_compute_compatibility;
    bridge_game_server15_vtable[38] = (void *)bridge_gs_connect_user;
    bridge_game_server15_vtable[39] = (void *)bridge_gs_create_unauthenticated;
    bridge_game_server15_vtable[40] = (void *)bridge_gs_disconnect_user;
    bridge_game_server15_vtable[41] = (void *)bridge_gs_update_user;
    bridge_game_server15_vtable[42] = (void *)bridge_gs_set_heartbeat_interval;
    bridge_game_server15_vtable[43] = (void *)bridge_gs_force_heartbeat;
    bridge_matchmaking_servers_vtable[0] = (void *)bridge_mm_request_internet;
    bridge_matchmaking_servers_vtable[1] = (void *)bridge_mm_request_lan;
    bridge_matchmaking_servers_vtable[2] = (void *)bridge_mm_request_friends;
    bridge_matchmaking_servers_vtable[3] = (void *)bridge_mm_request_favorites;
    bridge_matchmaking_servers_vtable[4] = (void *)bridge_mm_request_history;
    bridge_matchmaking_servers_vtable[5] = (void *)bridge_mm_request_spectator;
    bridge_matchmaking_servers_vtable[6] = (void *)bridge_mm_release;
    bridge_matchmaking_servers_vtable[7] = (void *)bridge_mm_details;
    bridge_matchmaking_servers_vtable[8] = (void *)bridge_mm_cancel;
    bridge_matchmaking_servers_vtable[9] = (void *)bridge_mm_refresh;
    bridge_matchmaking_servers_vtable[10] = (void *)bridge_mm_is_refreshing;
    bridge_matchmaking_servers_vtable[11] = (void *)bridge_mm_count;
    bridge_matchmaking_servers_vtable[12] = (void *)bridge_mm_refresh_server;
    bridge_matchmaking_servers_vtable[13] = (void *)bridge_mm_ping;
    bridge_matchmaking_servers_vtable[14] = (void *)bridge_mm_players;
    bridge_matchmaking_servers_vtable[15] = (void *)bridge_mm_rules;
    bridge_matchmaking_servers_vtable[16] = (void *)bridge_mm_cancel_server;
    bridge_lobby_vtable[0] = (void *)bridge_lobby_favorite_count;
    bridge_lobby_vtable[1] = (void *)bridge_lobby_get_favorite;
    bridge_lobby_vtable[2] = (void *)bridge_lobby_add_favorite;
    bridge_lobby_vtable[3] = (void *)bridge_lobby_remove_favorite;
    bridge_lobby_vtable[4] = (void *)bridge_lobby_request_list;
    bridge_lobby_vtable[5] = (void *)bridge_lobby_string_filter;
    bridge_lobby_vtable[6] = (void *)bridge_lobby_numerical_filter;
    bridge_lobby_vtable[7] = (void *)bridge_lobby_near_filter;
    bridge_lobby_vtable[8] = (void *)bridge_lobby_slots_filter;
    bridge_lobby_vtable[9] = (void *)bridge_lobby_distance_filter;
    bridge_lobby_vtable[10] = (void *)bridge_lobby_result_filter;
    bridge_lobby_vtable[11] = (void *)bridge_lobby_compatible_filter;
    bridge_lobby_vtable[12] = (void *)bridge_lobby_by_index;
    bridge_lobby_vtable[13] = (void *)bridge_lobby_create;
    bridge_lobby_vtable[14] = (void *)bridge_lobby_join;
    bridge_lobby_vtable[15] = (void *)bridge_lobby_leave;
    bridge_lobby_vtable[16] = (void *)bridge_lobby_invite;
    bridge_lobby_vtable[17] = (void *)bridge_lobby_member_count;
    bridge_lobby_vtable[18] = (void *)bridge_lobby_member_by_index;
    bridge_lobby_vtable[19] = (void *)bridge_lobby_get_data;
    bridge_lobby_vtable[20] = (void *)bridge_lobby_set_data;
    bridge_lobby_vtable[21] = (void *)bridge_lobby_data_count;
    bridge_lobby_vtable[22] = (void *)bridge_lobby_data_by_index;
    bridge_lobby_vtable[23] = (void *)bridge_lobby_delete_data;
    bridge_lobby_vtable[24] = (void *)bridge_lobby_member_data;
    bridge_lobby_vtable[25] = (void *)bridge_lobby_set_member_data;
    bridge_lobby_vtable[26] = (void *)bridge_lobby_send_chat;
    bridge_lobby_vtable[27] = (void *)bridge_lobby_get_chat;
    bridge_lobby_vtable[28] = (void *)bridge_lobby_request_data;
    bridge_lobby_vtable[29] = (void *)bridge_lobby_set_game_server;
    bridge_lobby_vtable[30] = (void *)bridge_lobby_get_game_server;
    bridge_lobby_vtable[31] = (void *)bridge_lobby_set_limit;
    bridge_lobby_vtable[32] = (void *)bridge_lobby_member_limit;
    bridge_lobby_vtable[33] = (void *)bridge_lobby_set_type;
    bridge_lobby_vtable[34] = (void *)bridge_lobby_set_joinable;
    bridge_lobby_vtable[35] = (void *)bridge_lobby_owner;
    bridge_lobby_vtable[36] = (void *)bridge_lobby_set_owner;
    bridge_lobby_vtable[37] = (void *)bridge_lobby_set_linked;
    /* SteamMatchMaking009 inserted CompatibleMembersFilter at slot 11 and
     * SetLinkedLobby at the end. SteamMatchMaking008 therefore uses slots
     * 0..10 unchanged and 009 slots 12..37 as its slots 11..36. */
    memcpy( bridge_lobby8_vtable, bridge_lobby_vtable,
            11 * sizeof(*bridge_lobby8_vtable) );
    memcpy( bridge_lobby8_vtable + 11, bridge_lobby_vtable + 12,
            25 * sizeof(*bridge_lobby8_vtable) );
    bridge_remote_storage_vtable[0] = (void *)bridge_remote_file_write;
    bridge_remote_storage_vtable[1] = (void *)bridge_remote_file_read;
    bridge_remote_storage_vtable[5] = (void *)bridge_remote_forget;
    bridge_remote_storage_vtable[6] = (void *)bridge_remote_delete;
    bridge_remote_storage_vtable[7] = (void *)bridge_remote_file_share;
    bridge_remote_storage_vtable[8] = (void *)bridge_remote_set_sync;
    bridge_remote_storage_vtable[9] = (void *)bridge_remote_stream_open;
    bridge_remote_storage_vtable[10] = (void *)bridge_remote_stream_chunk;
    bridge_remote_storage_vtable[11] = (void *)bridge_remote_stream_close;
    bridge_remote_storage_vtable[12] = (void *)bridge_remote_stream_cancel;
    bridge_remote_storage_vtable[13] = (void *)bridge_remote_exists;
    bridge_remote_storage_vtable[14] = (void *)bridge_remote_persisted;
    bridge_remote_storage_vtable[15] = (void *)bridge_remote_file_size;
    bridge_remote_storage_vtable[16] = (void *)bridge_remote_timestamp;
    bridge_remote_storage_vtable[17] = (void *)bridge_remote_sync_platforms;
    bridge_remote_storage_vtable[18] = (void *)bridge_remote_file_count;
    bridge_remote_storage_vtable[19] = (void *)bridge_remote_name_size;
    bridge_remote_storage_vtable[20] = (void *)bridge_remote_quota;
    bridge_remote_storage_vtable[21] = (void *)bridge_remote_cloud_account;
    bridge_remote_storage_vtable[22] = (void *)bridge_remote_cloud_app;
    bridge_remote_storage_vtable[23] = (void *)bridge_remote_set_cloud_app;
    bridge_remote_storage11_vtable[0] = (void *)bridge_remote_file_write;
    bridge_remote_storage11_vtable[1] = (void *)bridge_remote_file_read;
    bridge_remote_storage11_vtable[2] = (void *)bridge_remote_forget;
    bridge_remote_storage11_vtable[3] = (void *)bridge_remote_delete;
    bridge_remote_storage11_vtable[4] = (void *)bridge_remote_file_share;
    bridge_remote_storage11_vtable[5] = (void *)bridge_remote_set_sync;
    bridge_remote_storage11_vtable[6] = (void *)bridge_remote_stream_open;
    bridge_remote_storage11_vtable[7] = (void *)bridge_remote_stream_chunk;
    bridge_remote_storage11_vtable[8] = (void *)bridge_remote_stream_close;
    bridge_remote_storage11_vtable[9] = (void *)bridge_remote_stream_cancel;
    bridge_remote_storage11_vtable[10] = (void *)bridge_remote_exists;
    bridge_remote_storage11_vtable[11] = (void *)bridge_remote_persisted;
    bridge_remote_storage11_vtable[12] = (void *)bridge_remote_file_size;
    bridge_remote_storage11_vtable[13] = (void *)bridge_remote_timestamp;
    bridge_remote_storage11_vtable[14] = (void *)bridge_remote_sync_platforms;
    bridge_remote_storage11_vtable[15] = (void *)bridge_remote_file_count;
    bridge_remote_storage11_vtable[16] = (void *)bridge_remote_name_size;
    bridge_remote_storage11_vtable[17] = (void *)bridge_remote_quota32;
    bridge_remote_storage11_vtable[18] = (void *)bridge_remote_cloud_account;
    bridge_remote_storage11_vtable[19] = (void *)bridge_remote_cloud_app;
    bridge_remote_storage11_vtable[20] = (void *)bridge_remote_set_cloud_app;
    bridge_remote_storage2_vtable[0] = (void *)bridge_remote_file_write;
    bridge_remote_storage2_vtable[1] = (void *)bridge_remote_file_size;
    bridge_remote_storage2_vtable[2] = (void *)bridge_remote_file_read;
    bridge_remote_storage2_vtable[3] = (void *)bridge_remote_exists;
    bridge_remote_storage2_vtable[4] = (void *)bridge_remote_file_count;
    bridge_remote_storage2_vtable[5] = (void *)bridge_remote_name_size;
    bridge_remote_storage2_vtable[6] = (void *)bridge_remote_quota32;
    bridge_networking_vtable[0] = (void *)bridge_network_send;
    bridge_networking_vtable[1] = (void *)bridge_network_available;
    bridge_networking_vtable[2] = (void *)bridge_network_read;
    bridge_networking_vtable[3] = (void *)bridge_network_accept;
    bridge_networking_vtable[4] = (void *)bridge_network_close;
    bridge_networking_vtable[5] = (void *)bridge_network_close_channel;
    bridge_networking_vtable[6] = (void *)bridge_network_session_state;
    bridge_networking_vtable[7] = (void *)bridge_network_allow_relay;
    memcpy( bridge_networking3_vtable, bridge_networking_vtable,
            5 * sizeof(*bridge_networking3_vtable) );
    bridge_networking3_vtable[5] = (void *)bridge_network_session_state;
    bridge_game_server_stats_vtable[0] = (void *)bridge_game_server_stats_request;
    bridge_game_server_stats_vtable[1] = (void *)bridge_game_server_stats_get_int;
    bridge_game_server_stats_vtable[2] = (void *)bridge_game_server_stats_get_float;
    bridge_game_server_stats_vtable[3] = (void *)bridge_game_server_stats_get_achievement;
    bridge_game_server_stats_vtable[4] = (void *)bridge_game_server_stats_set_int;
    bridge_game_server_stats_vtable[5] = (void *)bridge_game_server_stats_set_float;
    bridge_game_server_stats_vtable[6] = (void *)bridge_game_server_stats_update_rate;
    bridge_game_server_stats_vtable[7] = (void *)bridge_game_server_stats_set_achievement;
    bridge_game_server_stats_vtable[8] = (void *)bridge_game_server_stats_clear_achievement;
    bridge_game_server_stats_vtable[9] = (void *)bridge_game_server_stats_store;
    bridge_user_stats_vtable[0] = (void *)bridge_user_stats_request_current;
    bridge_user_stats_vtable[1] = (void *)bridge_user_stats_get_int;
    bridge_user_stats_vtable[2] = (void *)bridge_user_stats_get_float;
    bridge_user_stats_vtable[3] = (void *)bridge_user_stats_set_int;
    bridge_user_stats_vtable[4] = (void *)bridge_user_stats_set_float;
    bridge_user_stats_vtable[5] = (void *)bridge_user_stats_update_rate;
    bridge_user_stats_vtable[6] = (void *)bridge_user_stats_get_achievement;
    bridge_user_stats_vtable[7] = (void *)bridge_user_stats_set_achievement;
    bridge_user_stats_vtable[8] = (void *)bridge_user_stats_clear_achievement;
    bridge_user_stats_vtable[9] = (void *)bridge_user_stats_get_unlock;
    bridge_user_stats_vtable[10] = (void *)bridge_user_stats_store;
    bridge_user_stats_vtable[11] = (void *)bridge_user_stats_icon;
    bridge_user_stats_vtable[12] = (void *)bridge_user_stats_display;
    bridge_user_stats_vtable[13] = (void *)bridge_user_stats_indicate;
    bridge_user_stats_vtable[14] = (void *)bridge_user_stats_num_achievements;
    bridge_user_stats_vtable[15] = (void *)bridge_user_stats_achievement_name;
    bridge_user_stats_vtable[16] = (void *)bridge_user_stats_request_user;
    bridge_user_stats_vtable[17] = (void *)bridge_user_stats_get_user_int;
    bridge_user_stats_vtable[18] = (void *)bridge_user_stats_get_user_float;
    bridge_user_stats_vtable[19] = (void *)bridge_user_stats_get_user_achievement;
    bridge_user_stats_vtable[20] = (void *)bridge_user_stats_get_user_unlock;
    bridge_user_stats_vtable[21] = (void *)bridge_user_stats_reset;
    bridge_user_stats_vtable[22] = (void *)bridge_user_stats_find_create;
    bridge_user_stats_vtable[23] = (void *)bridge_user_stats_find;
    bridge_user_stats_vtable[24] = (void *)bridge_user_stats_leaderboard_name;
    bridge_user_stats_vtable[25] = (void *)bridge_user_stats_entry_count;
    bridge_user_stats_vtable[26] = (void *)bridge_user_stats_sort;
    bridge_user_stats_vtable[27] = (void *)bridge_user_stats_display_type;
    bridge_user_stats_vtable[28] = (void *)bridge_user_stats_download;
    bridge_user_stats_vtable[29] = (void *)bridge_user_stats_download_users;
    bridge_user_stats_vtable[30] = (void *)bridge_user_stats_downloaded_entry;
    bridge_user_stats_vtable[31] = (void *)bridge_user_stats_upload;
    bridge_user_stats_vtable[32] = (void *)bridge_user_stats_attach;
    bridge_user_stats_vtable[33] = (void *)bridge_user_stats_current_players;
    bridge_user_stats_vtable[34] = (void *)bridge_user_stats_global_achievements;
    bridge_user_stats_vtable[35] = (void *)bridge_user_stats_most_achieved;
    bridge_user_stats_vtable[36] = (void *)bridge_user_stats_next_achieved;
    bridge_user_stats_vtable[37] = (void *)bridge_user_stats_achievement_percent;
    bridge_user_stats_vtable[38] = (void *)bridge_user_stats_request_global;
    bridge_user_stats_vtable[39] = (void *)bridge_user_stats_global_int64;
    bridge_user_stats_vtable[40] = (void *)bridge_user_stats_global_double;
    bridge_user_stats_vtable[41] = (void *)bridge_user_stats_history_int64;
    bridge_user_stats_vtable[42] = (void *)bridge_user_stats_history_double;
    bridge_user_stats_vtable[43] = (void *)bridge_user_stats_limits_int;
    bridge_user_stats_vtable[44] = (void *)bridge_user_stats_limits_float;
    memcpy( bridge_user_stats7_vtable, bridge_user_stats_vtable,
            14 * sizeof(*bridge_user_stats7_vtable) );
    bridge_user_stats7_vtable[14] = (void *)bridge_user_stats_request_user;
    bridge_user_stats7_vtable[15] = (void *)bridge_user_stats_get_user_int;
    bridge_user_stats7_vtable[16] = (void *)bridge_user_stats_get_user_float;
    bridge_user_stats7_vtable[17] = (void *)bridge_user_stats_get_user_achievement;
    bridge_user_stats7_vtable[18] = (void *)bridge_user_stats_get_user_unlock;
    bridge_user_stats7_vtable[19] = (void *)bridge_user_stats_reset;
    bridge_user_stats7_vtable[20] = (void *)bridge_user_stats_find_create;
    bridge_user_stats7_vtable[21] = (void *)bridge_user_stats_find;
    bridge_user_stats7_vtable[22] = (void *)bridge_user_stats_leaderboard_name;
    bridge_user_stats7_vtable[23] = (void *)bridge_user_stats_entry_count;
    bridge_user_stats7_vtable[24] = (void *)bridge_user_stats_sort;
    bridge_user_stats7_vtable[25] = (void *)bridge_user_stats_display_type;
    bridge_user_stats7_vtable[26] = (void *)bridge_user_stats_download;
    bridge_user_stats7_vtable[27] = (void *)bridge_user_stats_downloaded_entry;
    bridge_user_stats7_vtable[28] = (void *)bridge_user_stats_upload;
    bridge_user_stats7_vtable[29] = (void *)bridge_user_stats_current_players;
    memcpy( bridge_user_stats11_vtable, bridge_user_stats_vtable,
            sizeof(bridge_user_stats11_vtable) );
    bridge_user_stats11_vtable[30] = (void *)bridge_zero;
    for (size_t i = 43; i < ARRAY_SIZE(bridge_user_stats11_vtable); ++i)
        bridge_user_stats11_vtable[i] = (void *)bridge_zero;
    bridge_ugc14_vtable[0] = (void *)bridge_ugc_create_user_query;
    bridge_ugc14_vtable[4] = (void *)bridge_ugc_send_query;
    bridge_ugc14_vtable[5] = (void *)bridge_ugc_get_query_result;
    bridge_ugc14_vtable[15] = (void *)bridge_ugc_release_query;
    bridge_ugc14_vtable[21] = (void *)bridge_ugc_set_long_description;
    bridge_ugc14_vtable[22] = (void *)bridge_ugc_set_metadata;
    bridge_ugc14_vtable[62] = (void *)bridge_ugc_subscribed_count;
    bridge_ugc14_vtable[63] = (void *)bridge_ugc_get_subscribed_items;
    bridge_ugc14_vtable[64] = (void *)bridge_ugc_get_item_state;
    bridge_ugc14_vtable[65] = (void *)bridge_ugc_get_item_install_info;
    bridge_ugc14_vtable[66] = (void *)bridge_ugc_get_item_download_info;
    bridge_ugc14_vtable[67] = (void *)bridge_ugc_download_item;
    bridge_ugc15_vtable[0] = (void *)bridge_ugc_create_user_query;
    bridge_ugc15_vtable[4] = (void *)bridge_ugc_send_query;
    bridge_ugc15_vtable[5] = (void *)bridge_ugc_get_query_result;
    bridge_ugc15_vtable[18] = (void *)bridge_ugc_release_query;
    bridge_ugc15_vtable[24] = (void *)bridge_ugc_set_long_description;
    bridge_ugc15_vtable[25] = (void *)bridge_ugc_set_metadata;
    bridge_ugc15_vtable[65] = (void *)bridge_ugc_subscribed_count;
    bridge_ugc15_vtable[66] = (void *)bridge_ugc_get_subscribed_items;
    bridge_ugc15_vtable[67] = (void *)bridge_ugc_get_item_state;
    bridge_ugc15_vtable[68] = (void *)bridge_ugc_get_item_install_info;
    bridge_ugc15_vtable[69] = (void *)bridge_ugc_get_item_download_info;
    bridge_ugc15_vtable[70] = (void *)bridge_ugc_download_item;
    bridge_http_vtable[0] = (void *)bridge_http_create;
    bridge_http_vtable[1] = (void *)bridge_http_context;
    bridge_http_vtable[2] = (void *)bridge_http_network_timeout;
    bridge_http_vtable[3] = (void *)bridge_http_header;
    bridge_http_vtable[4] = (void *)bridge_http_parameter;
    bridge_http_vtable[5] = (void *)bridge_http_send;
    bridge_http_vtable[6] = (void *)bridge_http_send_stream;
    bridge_http_vtable[7] = (void *)bridge_http_defer;
    bridge_http_vtable[8] = (void *)bridge_http_prioritize;
    bridge_http_vtable[9] = (void *)bridge_http_header_size;
    bridge_http_vtable[10] = (void *)bridge_http_header_value;
    bridge_http_vtable[11] = (void *)bridge_http_body_size;
    bridge_http_vtable[12] = (void *)bridge_http_body;
    bridge_http_vtable[13] = (void *)bridge_http_stream_body;
    bridge_http_vtable[14] = (void *)bridge_http_release;
    bridge_http_vtable[15] = (void *)bridge_http_progress;
    bridge_http_vtable[16] = (void *)bridge_http_raw_post;
    bridge_http_vtable[17] = (void *)bridge_http_create_cookie;
    bridge_http_vtable[18] = (void *)bridge_http_release_cookie;
    bridge_http_vtable[19] = (void *)bridge_http_set_cookie;
    bridge_http_vtable[20] = (void *)bridge_http_cookie_container;
    bridge_http_vtable[21] = (void *)bridge_http_user_agent;
    bridge_http_vtable[22] = (void *)bridge_http_verified;
    bridge_http_vtable[23] = (void *)bridge_http_absolute_timeout;
    bridge_http_vtable[24] = (void *)bridge_http_timed_out;
    bridge_http1_vtable[0] = (void *)bridge_http_create;
    bridge_http1_vtable[1] = (void *)bridge_http_context;
    bridge_http1_vtable[2] = (void *)bridge_http_network_timeout;
    bridge_http1_vtable[3] = (void *)bridge_http_header;
    bridge_http1_vtable[4] = (void *)bridge_http_parameter;
    bridge_http1_vtable[5] = (void *)bridge_http_send;
    bridge_http1_vtable[6] = (void *)bridge_http_defer;
    bridge_http1_vtable[7] = (void *)bridge_http_prioritize;
    bridge_http1_vtable[8] = (void *)bridge_http_header_size;
    bridge_http1_vtable[9] = (void *)bridge_http_header_value;
    bridge_http1_vtable[10] = (void *)bridge_http_body_size;
    bridge_http1_vtable[11] = (void *)bridge_http_body;
    bridge_http1_vtable[12] = (void *)bridge_http_release;
    bridge_http1_vtable[13] = (void *)bridge_http_progress;
    bridge_http1_vtable[14] = (void *)bridge_http_raw_post;
    bridge_inventory_vtable[0] = (void *)bridge_inventory_status;
    bridge_inventory_vtable[1] = (void *)bridge_inventory_items;
    bridge_inventory_vtable[2] = (void *)bridge_inventory_item_property;
    bridge_inventory_vtable[3] = (void *)bridge_inventory_timestamp;
    bridge_inventory_vtable[4] = (void *)bridge_inventory_check_steamid;
    bridge_inventory_vtable[5] = (void *)bridge_inventory_destroy;
    bridge_inventory_vtable[6] = (void *)bridge_inventory_all;
    bridge_inventory_vtable[7] = (void *)bridge_inventory_by_id;
    bridge_inventory_vtable[8] = (void *)bridge_inventory_serialize;
    bridge_inventory_vtable[9] = (void *)bridge_inventory_deserialize;
    bridge_inventory_vtable[20] = (void *)bridge_inventory_load_definitions;
    bridge_inventory_vtable[21] = (void *)bridge_inventory_definition_ids;
    bridge_inventory_vtable[22] = (void *)bridge_inventory_definition_property;
    bridge_inventory_vtable[23] = (void *)bridge_inventory_request_eligible;
    bridge_inventory_vtable[24] = (void *)bridge_inventory_eligible_ids;
    bridge_inventory_vtable[26] = (void *)bridge_inventory_request_prices;
    bridge_inventory_vtable[27] = (void *)bridge_inventory_price_count;
    bridge_inventory_vtable[28] = (void *)bridge_inventory_prices;
    bridge_inventory_vtable[29] = (void *)bridge_inventory_price;
    bridge_inventory_vtable[37] = (void *)bridge_inventory_inspect;
    bridge_inventory2_vtable[0] = (void *)bridge_inventory_status;
    bridge_inventory2_vtable[1] = (void *)bridge_inventory_items;
    bridge_inventory2_vtable[2] = (void *)bridge_inventory_item_property;
    bridge_inventory2_vtable[3] = (void *)bridge_inventory_timestamp;
    bridge_inventory2_vtable[4] = (void *)bridge_inventory_check_steamid;
    bridge_inventory2_vtable[5] = (void *)bridge_inventory_destroy;
    bridge_inventory2_vtable[6] = (void *)bridge_inventory_all;
    bridge_inventory2_vtable[7] = (void *)bridge_inventory_by_id;
    bridge_inventory2_vtable[8] = (void *)bridge_inventory_serialize;
    bridge_inventory2_vtable[9] = (void *)bridge_inventory_deserialize;
    bridge_inventory2_vtable[20] = (void *)bridge_inventory_load_definitions;
    bridge_inventory2_vtable[21] = (void *)bridge_inventory_definition_ids;
    bridge_inventory2_vtable[22] = (void *)bridge_inventory_definition_property;
    bridge_inventory2_vtable[23] = (void *)bridge_inventory_request_eligible;
    bridge_inventory2_vtable[24] = (void *)bridge_inventory_eligible_ids;
    bridge_inventory2_vtable[26] = (void *)bridge_inventory_request_prices;
    bridge_inventory2_vtable[27] = (void *)bridge_inventory_price_count;
    bridge_inventory2_vtable[28] = (void *)bridge_inventory_prices_v2;
    bridge_inventory2_vtable[29] = (void *)bridge_inventory_price_v2;
    bridge_inventory1_vtable[0] = (void *)bridge_inventory_status;
    bridge_inventory1_vtable[1] = (void *)bridge_inventory_items;
    bridge_inventory1_vtable[2] = (void *)bridge_inventory_timestamp;
    bridge_inventory1_vtable[3] = (void *)bridge_inventory_check_steamid;
    bridge_inventory1_vtable[4] = (void *)bridge_inventory_destroy;
    bridge_inventory1_vtable[5] = (void *)bridge_inventory_all;
    bridge_inventory1_vtable[6] = (void *)bridge_inventory_by_id;
    bridge_inventory1_vtable[7] = (void *)bridge_inventory_serialize;
    bridge_inventory1_vtable[8] = (void *)bridge_inventory_deserialize;
    bridge_inventory1_vtable[19] = (void *)bridge_inventory_load_definitions;
    bridge_inventory1_vtable[20] = (void *)bridge_inventory_definition_ids;
    bridge_inventory1_vtable[21] = (void *)bridge_inventory_definition_property;
    bridge_inventory1_vtable[22] = (void *)bridge_inventory_request_eligible;
    bridge_inventory1_vtable[23] = (void *)bridge_inventory_eligible_ids;
    bridge_game_search_vtable[0] = (void *)bridge_game_search_add;
    bridge_game_search_vtable[1] = (void *)bridge_game_search_lobby;
    bridge_game_search_vtable[2] = (void *)bridge_game_search_solo;
    bridge_game_search_vtable[3] = (void *)bridge_game_search_accept;
    bridge_game_search_vtable[4] = (void *)bridge_game_search_decline;
    bridge_game_search_vtable[5] = (void *)bridge_game_search_connection;
    bridge_game_search_vtable[6] = (void *)bridge_game_search_end_search;
    bridge_game_search_vtable[7] = (void *)bridge_game_search_host_params;
    bridge_game_search_vtable[8] = (void *)bridge_game_search_set_connection;
    bridge_game_search_vtable[9] = (void *)bridge_game_search_request_players;
    bridge_game_search_vtable[10] = (void *)bridge_game_search_confirm;
    bridge_game_search_vtable[11] = (void *)bridge_game_search_cancel;
    bridge_game_search_vtable[12] = (void *)bridge_game_search_submit;
    bridge_game_search_vtable[13] = (void *)bridge_game_search_end_game;
    bridge_app_list_vtable[0] = (void *)bridge_app_list_count;
    bridge_app_list_vtable[1] = (void *)bridge_app_list_apps;
    bridge_app_list_vtable[2] = (void *)bridge_app_list_name;
    bridge_app_list_vtable[3] = (void *)bridge_app_list_install_dir;
    bridge_app_list_vtable[4] = (void *)bridge_app_list_build_id;
    bridge_video_vtable[0] = (void *)bridge_video_url;
    bridge_video_vtable[1] = (void *)bridge_video_broadcasting;
    bridge_video_vtable[2] = (void *)bridge_video_opf_settings;
    bridge_video_vtable[3] = (void *)bridge_video_opf_string;
    bridge_parental_vtable[0] = (void *)bridge_parental_lock_enabled;
    bridge_parental_vtable[1] = (void *)bridge_parental_lock_locked;
    bridge_parental_vtable[2] = (void *)bridge_parental_app_blocked;
    bridge_parental_vtable[3] = (void *)bridge_parental_app_in_list;
    bridge_parental_vtable[4] = (void *)bridge_parental_feature_blocked;
    bridge_parental_vtable[5] = (void *)bridge_parental_feature_in_list;
    bridge_network_messages_vtable[0] = (void *)bridge_network_messages_send;
    bridge_network_messages_vtable[1] = (void *)bridge_network_messages_receive;
    bridge_network_messages_vtable[2] = (void *)bridge_network_messages_accept;
    bridge_network_messages_vtable[3] = (void *)bridge_network_messages_close;
    bridge_network_messages_vtable[4] = (void *)bridge_network_messages_close_channel;
    bridge_network_messages_vtable[5] = (void *)bridge_network_messages_info;
    bridge_network_sockets_vtable[0] = (void *)bridge_network_sockets_listen_ip;
    bridge_network_sockets_vtable[1] = (void *)bridge_network_sockets_connect_ip;
    bridge_network_sockets_vtable[2] = (void *)bridge_network_sockets_listen_p2p;
    bridge_network_sockets_vtable[3] = (void *)bridge_network_sockets_connect_p2p;
    bridge_network_sockets_vtable[4] = (void *)bridge_network_sockets_accept;
    bridge_network_sockets_vtable[5] = (void *)bridge_network_sockets_close;
    bridge_network_sockets_vtable[6] = (void *)bridge_network_sockets_close_listen;
    bridge_network_sockets_vtable[7] = (void *)bridge_network_sockets_set_userdata;
    bridge_network_sockets_vtable[8] = (void *)bridge_network_sockets_get_userdata;
    bridge_network_sockets_vtable[9] = (void *)bridge_network_sockets_set_name;
    bridge_network_sockets_vtable[10] = (void *)bridge_network_sockets_get_name;
    bridge_network_sockets_vtable[11] = (void *)bridge_network_sockets_send;
    bridge_network_sockets_vtable[12] = (void *)bridge_network_sockets_send_many12;
    bridge_network_sockets_vtable[13] = (void *)bridge_network_sockets_flush;
    bridge_network_sockets_vtable[14] = (void *)bridge_network_sockets_receive;
    bridge_network_sockets_vtable[15] = (void *)bridge_network_sockets_info;
    bridge_network_sockets_vtable[16] = (void *)bridge_network_sockets_quick;
    bridge_network_sockets_vtable[17] = (void *)bridge_network_sockets_detailed;
    bridge_network_sockets_vtable[18] = (void *)bridge_network_sockets_listen_address;
    bridge_network_sockets_vtable[19] = (void *)bridge_network_sockets_pair;
    bridge_network_sockets_vtable[20] = (void *)bridge_network_sockets_lanes;
    bridge_network_sockets_vtable[21] = (void *)bridge_network_sockets_identity;
    bridge_network_sockets_vtable[22] = (void *)bridge_network_sockets_init_auth;
    bridge_network_sockets_vtable[23] = (void *)bridge_network_sockets_auth_status;
    bridge_network_sockets_vtable[24] = (void *)bridge_network_sockets_create_poll;
    bridge_network_sockets_vtable[25] = (void *)bridge_network_sockets_destroy_poll;
    bridge_network_sockets_vtable[26] = (void *)bridge_network_sockets_set_poll;
    bridge_network_sockets_vtable[27] = (void *)bridge_network_sockets_receive_poll;
    bridge_network_sockets_vtable[46] = (void *)bridge_network_sockets_create_fake;
    bridge_network_fake_udp_vtable[0] = (void *)bridge_network_fake_udp_destroy;
    bridge_network_fake_udp_vtable[1] = (void *)bridge_network_fake_udp_send;
    bridge_network_fake_udp_vtable[2] = (void *)bridge_network_fake_udp_receive;
    bridge_network_fake_udp_vtable[3] = (void *)bridge_network_fake_udp_cleanup;
    memcpy( bridge_network_sockets13_vtable, bridge_network_sockets_vtable,
            sizeof(bridge_network_sockets_vtable) );
    bridge_network_sockets13_vtable[12] =
        (void *)bridge_network_sockets_send_many13;
    bridge_network_utils3_vtable[0] = (void *)bridge_network_utils_allocate3;
    bridge_network_utils3_vtable[1] = (void *)bridge_network_utils_relay_status;
    bridge_network_utils3_vtable[2] = (void *)bridge_network_utils_local_ping;
    bridge_network_utils3_vtable[3] = (void *)bridge_network_utils_estimate_two;
    bridge_network_utils3_vtable[4] = (void *)bridge_network_utils_estimate_local;
    bridge_network_utils3_vtable[5] = (void *)bridge_network_utils_ping_to_string;
    bridge_network_utils3_vtable[6] = (void *)bridge_network_utils_parse_ping;
    bridge_network_utils3_vtable[7] = (void *)bridge_network_utils_check_ping;
    bridge_network_utils3_vtable[8] = (void *)bridge_network_utils_ping_dc;
    bridge_network_utils3_vtable[9] = (void *)bridge_network_utils_direct_ping;
    bridge_network_utils3_vtable[10] = (void *)bridge_network_utils_pop_count;
    bridge_network_utils3_vtable[11] = (void *)bridge_network_utils_pop_list;
    bridge_network_utils3_vtable[12] = (void *)bridge_network_utils_timestamp;
    bridge_network_utils3_vtable[13] = (void *)bridge_network_utils_debug;
    bridge_network_utils3_vtable[14] = (void *)bridge_network_utils_set_config;
    bridge_network_utils3_vtable[15] = (void *)bridge_network_utils_get_config;
    bridge_network_utils3_vtable[16] = (void *)bridge_network_utils_config_info3;
    bridge_network_utils3_vtable[17] = (void *)bridge_network_utils_first_config;
    bridge_network_utils3_vtable[18] = (void *)bridge_network_utils_ip_to_string;
    bridge_network_utils3_vtable[19] = (void *)bridge_network_utils_ip_parse;
    bridge_network_utils3_vtable[20] = (void *)bridge_network_utils_identity_to_string;
    bridge_network_utils3_vtable[21] = (void *)bridge_network_utils_identity_parse;
    bridge_network_utils4_vtable[0] = (void *)bridge_network_utils_allocate4;
    memcpy( bridge_network_utils4_vtable + 1,
            bridge_network_utils3_vtable + 1, 13 * sizeof(void *) );
    bridge_network_utils4_vtable[14] = (void *)bridge_network_utils_fake_ipv4;
    bridge_network_utils4_vtable[15] = (void *)bridge_network_utils_real_identity;
    bridge_network_utils4_vtable[16] = (void *)bridge_network_utils_set_config;
    bridge_network_utils4_vtable[17] = (void *)bridge_network_utils_get_config;
    bridge_network_utils4_vtable[18] = (void *)bridge_network_utils_config_info4;
    bridge_network_utils4_vtable[19] = (void *)bridge_network_utils_iterate;
    bridge_network_utils4_vtable[20] = (void *)bridge_network_utils_ip_to_string;
    bridge_network_utils4_vtable[21] = (void *)bridge_network_utils_ip_parse;
    bridge_network_utils4_vtable[22] = (void *)bridge_network_utils_ip_fake_type;
    bridge_network_utils4_vtable[23] = (void *)bridge_network_utils_identity_to_string;
    bridge_network_utils4_vtable[24] = (void *)bridge_network_utils_identity_parse;
    bridge_proxy_ready = true;
}

static void *bridge_CreateInterface( const char *name, int *return_code )
{
    bridge_trace( "CreateInterface %s", name ? name : "(null)" );
    if (return_code) *return_code = 0;
    if (name && !strcmp( name, "SteamClient008" )) return &bridge_client008;
    if (name && !strcmp( name, "SteamClient009" )) return &bridge_client009;
    if (name && !strcmp( name, "SteamClient013" )) return &bridge_client13;
    if (name && !strcmp( name, "SteamClient014" )) return &bridge_client14;
    if (name && !strcmp( name, "SteamClient015" )) return &bridge_client15;
    if (name && !strcmp( name, "SteamClient016" )) return &bridge_client16;
    if (name && !strcmp( name, "SteamClient017" )) return &bridge_client17;
    if (name && !strcmp( name, "SteamClient018" )) return &bridge_client18;
    if (name && !strcmp( name, "SteamClient019" )) return &bridge_client19;
    if (name && !strcmp( name, "SteamClient020" )) return &bridge_client;
    if (name && !strcmp( name, "SteamClient021" )) return &bridge_client21;
    if (name && !strcmp( name, "SteamClient023" )) return &bridge_client23;
    if (name && !strncmp( name, "SteamClient", 11 )) return &bridge_client;
    if (name && !strcmp( name, "STEAMAPPTICKET_INTERFACE_VERSION001" ))
        return &bridge_app_ticket;
    if (name && !strcmp( name, "SteamNetworkingMessages002" ))
        return &bridge_network_messages;
    if (name && !strcmp( name, "SteamNetworkingSockets012" ))
        return &bridge_network_sockets;
    if (name && !strcmp( name, "SteamNetworkingSockets013" ))
        return &bridge_network_sockets13;
    if (name && !strcmp( name, "SteamNetworkingUtils003" ))
        return &bridge_network_utils3;
    if (name && !strcmp( name, "SteamNetworkingUtils004" ))
        return &bridge_network_utils4;
    if (name && !strcmp( name, "STEAMHTMLSURFACE_INTERFACE_VERSION_005" ))
    {
        bridge_trace( "HTML CreateInterface disabled (A/B test)" );
        if (return_code) *return_code = 1;
        return NULL;
    }
    if (return_code) *return_code = 1;
    return NULL;
}
static bool bridge_rpc_connect( int *fd, const char **path )
{
    *path = getenv( "LSTEAM_BRIDGE_SOCKET" );
    if (!*path || !**path) *path = LSTEAM_BRIDGE_DEFAULT_SOCKET;
    if (strlen( *path ) >= sizeof(((sockaddr_un *)0)->sun_path)) return false;
    *fd = socket( AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0 );
    if (*fd < 0) return false;
    sockaddr_un address = {};
    address.sun_family = AF_UNIX;
    memcpy( address.sun_path, *path, strlen(*path) + 1 );
    if (connect( *fd, (sockaddr *)&address,
                 offsetof(sockaddr_un, sun_path) + strlen(*path) + 1 ) < 0)
    {
        close( *fd );
        return false;
    }
    return true;
}

static int8_t bridge_Steam_BGetCallback( int32_t, u_CallbackMsg_t *msg,
                                         int32_t *steam_call )
{
    static __thread uint8_t callback_data[LSTEAM_BRIDGE_MAX_CALLBACK];
    static uint32_t sequence = 10000;
    const char *path;
    int fd;
    if (msg && bridge_html_take_callback( msg, steam_call )) return 1;
    bridge_mm_pump();
    if (!msg || !bridge_rpc_connect( &fd, &path )) return 0;
    lsteam_bridge_request request = { LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        LSTEAM_BRIDGE_GET_CALLBACK,
        __atomic_add_fetch( &sequence, 1, __ATOMIC_RELAXED ) };
    lsteam_bridge_callback_response response = {};
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              recv( fd, &response, sizeof(response), 0 ) == sizeof(response);
    close( fd );
    ok = ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
         response.version == LSTEAM_BRIDGE_VERSION &&
         response.opcode == LSTEAM_BRIDGE_GET_CALLBACK &&
         response.sequence == request.sequence && !response.status &&
         response.data_size <= LSTEAM_BRIDGE_MAX_CALLBACK;
    if (!ok) return 0;
    if (response.data_size) memcpy( callback_data, response.data, response.data_size );
    msg->m_hSteamUser = response.steam_user;
    msg->m_iCallback = response.callback_id;
    msg->m_pubParam = callback_data;
    msg->m_cubParam = response.data_size;
    if (steam_call) *steam_call = response.steam_call;
    bridge_trace( "callback id=%d size=%u", response.callback_id,
                  response.data_size );
    return 1;
}

static int8_t bridge_Steam_GetAPICallResult( int32_t, uint64_t call,
                                             void *callback, int size, int id,
                                             int8_t *failed )
{
    static uint32_t sequence = 20000;
    const char *path;
    int fd;
    if (bridge_html_get_api_result( call, callback, size, id, failed )) return 1;
    if (!callback || size < 0 || (uint32_t)size > LSTEAM_BRIDGE_MAX_CALLBACK ||
        !bridge_rpc_connect( &fd, &path )) return 0;
    lsteam_bridge_request request = { LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        LSTEAM_BRIDGE_GET_API_CALL_RESULT,
        __atomic_add_fetch( &sequence, 1, __ATOMIC_RELAXED ) };
    lsteam_bridge_api_call_request api = { call, size, id };
    lsteam_bridge_api_call_response response = {};
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, &api, sizeof(api), MSG_NOSIGNAL ) == sizeof(api) &&
              recv( fd, &response, sizeof(response), 0 ) == sizeof(response);
    close( fd );
    ok = ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
         response.version == LSTEAM_BRIDGE_VERSION &&
         response.opcode == LSTEAM_BRIDGE_GET_API_CALL_RESULT &&
         response.sequence == request.sequence && !response.status &&
         response.data_size == (uint32_t)size;
    if (failed) *failed = response.failed;
    if (!ok) return 0;
    if (size) memcpy( callback, response.data, size );
    bridge_trace( "API result call=%llu id=%d size=%d failed=%d",
                  (unsigned long long)call, id, size, response.failed );
    return 1;
}

static int8_t bridge_Steam_FreeLastCallback( int32_t )
{
    static uint32_t sequence = 30000;
    const char *path;
    int fd;
    if (bridge_html_free_callback()) return 1;
    if (!bridge_rpc_connect( &fd, &path )) return 0;
    lsteam_bridge_request request = { LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        LSTEAM_BRIDGE_FREE_CALLBACK,
        __atomic_add_fetch( &sequence, 1, __ATOMIC_RELAXED ) };
    lsteam_bridge_response response = {};
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              recv( fd, &response, sizeof(response), 0 ) == sizeof(response);
    close( fd );
    return ok && response.magic == LSTEAM_BRIDGE_MAGIC &&
           response.version == LSTEAM_BRIDGE_VERSION && !response.status;
}
static void bridge_Steam_ReleaseThreadLocalMemory( int ) {}
static bool bridge_Steam_IsKnownInterface( const char * ) { return true; }
static void bridge_Steam_NotifyMissingInterface( int32_t, const char * ) {}
