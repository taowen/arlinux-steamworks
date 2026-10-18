/* Bionic-side SteamMatchMakingServers002 proxy. */
struct bridge_mm_proxy_query
{
    uint32_t id;
    uint32_t type;
    void *handle;
    union
    {
        u_ISteamMatchmakingServerListResponse_106 *list;
        u_ISteamMatchmakingPingResponse *ping;
        u_ISteamMatchmakingPlayersResponse *players;
        u_ISteamMatchmakingRulesResponse *rules;
        void *opaque;
    } callback;
    gameserveritem_t_105 details;
};

static bridge_mm_proxy_query bridge_mm_queries[16];

static bridge_mm_proxy_query *bridge_mm_find( uint32_t id )
{
    if (!id) return NULL;
    for (size_t i = 0; i < ARRAY_SIZE(bridge_mm_queries); ++i)
        if (bridge_mm_queries[i].id == id) return &bridge_mm_queries[i];
    return NULL;
}

static bridge_mm_proxy_query *bridge_mm_free_slot(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(bridge_mm_queries); ++i)
        if (!bridge_mm_queries[i].id) return &bridge_mm_queries[i];
    return NULL;
}

static bool bridge_mm_rpc( uint16_t opcode, lsteam_bridge_mm_request *mm,
                           lsteam_bridge_mm_response *response )
{
    static uint32_t next_sequence = 40000;
    const char *path;
    int fd;
    if (!bridge_rpc_connect( &fd, &path )) return false;
    lsteam_bridge_request request = { LSTEAM_BRIDGE_MAGIC, LSTEAM_BRIDGE_VERSION,
        opcode, __atomic_add_fetch( &next_sequence, 1, __ATOMIC_RELAXED ) };
    memset( response, 0, sizeof(*response) );
    bool ok = send( fd, &request, sizeof(request), MSG_NOSIGNAL ) == sizeof(request) &&
              send( fd, mm, sizeof(*mm), MSG_NOSIGNAL ) == sizeof(*mm) &&
              recv( fd, response, sizeof(*response), 0 ) == sizeof(*response);
    close( fd );
    ok = ok && response->magic == LSTEAM_BRIDGE_MAGIC &&
         response->version == LSTEAM_BRIDGE_VERSION &&
         response->opcode == opcode && response->sequence == request.sequence &&
         !response->status && response->data_size <= sizeof(response->data);
    if (!ok) bridge_trace( "Matchmaking opcode=%u method=%u query=%u failed status=%d",
                           opcode, mm->method, mm->query_id, response->status );
    return ok;
}

static void bridge_mm_dispatch( const lsteam_bridge_mm_response *response )
{
    bridge_mm_proxy_query *query = bridge_mm_find( response->query_id );
    if (!query || !response->event_kind) return;
    bool retire = response->event_kind == 4 || response->event_kind == 5 ||
                  response->event_kind == 7 || response->event_kind == 8 ||
                  response->event_kind == 10 || response->event_kind == 11;
    if (!query->callback.opaque)
    {
        if (retire) memset( query, 0, sizeof(*query) );
        return;
    }
    switch (response->event_kind)
    {
    case 1:
        query->callback.list->ServerResponded( query->handle, response->event_index );
        break;
    case 2:
        query->callback.list->ServerFailedToRespond( query->handle, response->event_index );
        break;
    case 3:
        query->callback.list->RefreshComplete( query->handle, response->event_response );
        break;
    case 4:
    {
        gameserveritem_t_165 server = {};
        size_t size = response->data_size < sizeof(server) ?
                      response->data_size : sizeof(server);
        if (size) memcpy( &server, response->data, size );
        query->callback.ping->ServerResponded( &server );
        break;
    }
    case 5:
        query->callback.ping->ServerFailedToRespond();
        break;
    case 6:
    {
        float time_played = 0.0f;
        memcpy( &time_played, &response->event_response, sizeof(time_played) );
        char name[LSTEAM_BRIDGE_MM_DATA_SIZE];
        memcpy( name, response->data, sizeof(name) );
        name[sizeof(name) - 1] = 0;
        query->callback.players->AddPlayerToList( name, response->event_index,
                                                  time_played );
        break;
    }
    case 7:
        query->callback.players->PlayersFailedToRespond();
        break;
    case 8:
        query->callback.players->PlayersRefreshComplete();
        break;
    case 9:
    {
        constexpr size_t half = LSTEAM_BRIDGE_MM_DATA_SIZE / 2;
        char rule[half], value[half];
        memcpy( rule, response->data, half );
        memcpy( value, response->data + half, half );
        rule[half - 1] = value[half - 1] = 0;
        query->callback.rules->RulesResponded( rule, value );
        break;
    }
    case 10:
        query->callback.rules->RulesFailedToRespond();
        break;
    case 11:
        query->callback.rules->RulesRefreshComplete();
        break;
    }
    if (retire) memset( query, 0, sizeof(*query) );
}

static void *bridge_mm_request( uint32_t method, uint32_t appid,
                                MatchMakingKeyValuePair_t **filters,
                                uint32_t filter_count,
                                u_ISteamMatchmakingServerListResponse_106 *callback )
{
    bridge_mm_proxy_query *query = bridge_mm_free_slot();
    if (!query || filter_count > LSTEAM_BRIDGE_MAX_MM_FILTERS) return NULL;
    lsteam_bridge_mm_request mm = {};
    lsteam_bridge_mm_response response = {};
    mm.method = method;
    mm.app_id = appid;
    mm.filter_count = filter_count;
    for (uint32_t i = 0; i < filter_count; ++i)
    {
        if (!filters || !filters[i]) continue;
        snprintf( mm.filters[i].key, sizeof(mm.filters[i].key), "%.*s",
                  (int)sizeof(mm.filters[i].key) - 1, filters[i]->m_szKey );
        snprintf( mm.filters[i].value, sizeof(mm.filters[i].value), "%.*s",
                  (int)sizeof(mm.filters[i].value) - 1, filters[i]->m_szValue );
    }
    if (!bridge_mm_rpc( LSTEAM_BRIDGE_MATCHMAKING_REQUEST, &mm, &response ) ||
        !response.query_id) return NULL;
    memset( query, 0, sizeof(*query) );
    query->id = response.query_id;
    query->type = 1;
    query->handle = (void *)(uintptr_t)response.query_id;
    query->callback.list = callback;
    bridge_trace( "Matchmaking request method=%u appid=%u query=%u count=%d refreshing=%u",
                  method, appid, query->id, response.server_count,
                  response.refreshing );
    bridge_mm_dispatch( &response );
    return query->handle;
}

static void *bridge_mm_request_internet( void *, uint32_t appid,
                                         MatchMakingKeyValuePair_t **filters,
                                         uint32_t count,
                                         u_ISteamMatchmakingServerListResponse_106 *callback )
{
    return bridge_mm_request( 0, appid, filters, count, callback );
}

static void *bridge_mm_request_lan( void *, uint32_t appid,
                                    u_ISteamMatchmakingServerListResponse_106 *callback )
{
    return bridge_mm_request( 1, appid, NULL, 0, callback );
}

#define BRIDGE_MM_REQUEST(name, method) \
    static void *name( void *, uint32_t appid, MatchMakingKeyValuePair_t **filters, \
                       uint32_t count, u_ISteamMatchmakingServerListResponse_106 *callback ) \
    { return bridge_mm_request( method, appid, filters, count, callback ); }
BRIDGE_MM_REQUEST(bridge_mm_request_friends, 2)
BRIDGE_MM_REQUEST(bridge_mm_request_favorites, 3)
BRIDGE_MM_REQUEST(bridge_mm_request_history, 4)
BRIDGE_MM_REQUEST(bridge_mm_request_spectator, 5)
#undef BRIDGE_MM_REQUEST

static bool bridge_mm_call( uint32_t method, void *handle, int32_t argument,
                            lsteam_bridge_mm_response *response )
{
    lsteam_bridge_mm_request mm = {};
    mm.method = method;
    mm.query_id = (uint32_t)(uintptr_t)handle;
    mm.argument = argument;
    if (!bridge_mm_rpc( LSTEAM_BRIDGE_MATCHMAKING_CALL, &mm, response )) return false;
    bridge_mm_dispatch( response );
    return true;
}

static void bridge_mm_release( void *, void *handle )
{
    lsteam_bridge_mm_response response = {};
    bridge_mm_proxy_query *query = bridge_mm_find( (uint32_t)(uintptr_t)handle );
    if (!query) return;
    bridge_mm_call( 6, handle, 0, &response );
    memset( query, 0, sizeof(*query) );
}

static gameserveritem_t_105 *bridge_mm_details( void *, void *handle, int32_t server )
{
    static_assert( sizeof(gameserveritem_t_105) == LSTEAM_BRIDGE_MM_ITEM_SIZE,
                   "SteamMatchMakingServers002 item ABI changed" );
    lsteam_bridge_mm_response response = {};
    bridge_mm_proxy_query *query = bridge_mm_find( (uint32_t)(uintptr_t)handle );
    if (!query || !bridge_mm_call( 7, handle, server, &response ) ||
        response.data_size != sizeof(query->details)) return NULL;
    memcpy( &query->details, response.data, sizeof(query->details) );
    return &query->details;
}

static void bridge_mm_cancel( void *, void *handle )
{
    lsteam_bridge_mm_response response = {};
    if (!bridge_mm_find( (uint32_t)(uintptr_t)handle )) return;
    bridge_mm_call( 8, handle, 0, &response );
}

static void bridge_mm_refresh( void *, void *handle )
{
    lsteam_bridge_mm_response response = {};
    if (!bridge_mm_find( (uint32_t)(uintptr_t)handle )) return;
    bridge_mm_call( 9, handle, 0, &response );
}

static int8_t bridge_mm_is_refreshing( void *, void *handle )
{
    lsteam_bridge_mm_response response = {};
    if (!bridge_mm_find( (uint32_t)(uintptr_t)handle )) return 0;
    return bridge_mm_call( 10, handle, 0, &response ) && response.refreshing;
}

static int32_t bridge_mm_count( void *, void *handle )
{
    lsteam_bridge_mm_response response = {};
    if (!bridge_mm_find( (uint32_t)(uintptr_t)handle )) return 0;
    return bridge_mm_call( 11, handle, 0, &response ) ? response.server_count : 0;
}

static void bridge_mm_refresh_server( void *, void *handle, int32_t server )
{
    lsteam_bridge_mm_response response = {};
    if (!bridge_mm_find( (uint32_t)(uintptr_t)handle )) return;
    bridge_mm_call( 12, handle, server, &response );
}

static int32_t bridge_mm_direct_request( uint32_t method, uint32_t ip,
                                         uint16_t port, void *callback )
{
    bridge_mm_proxy_query *query = bridge_mm_free_slot();
    if (!query) return -1;
    lsteam_bridge_mm_request mm = {};
    lsteam_bridge_mm_response response = {};
    mm.method = method;
    mm.app_id = ip;
    mm.query_id = port;
    if (!bridge_mm_rpc( LSTEAM_BRIDGE_MATCHMAKING_REQUEST, &mm, &response ) ||
        !response.query_id) return -1;
    memset( query, 0, sizeof(*query) );
    query->id = response.query_id;
    query->type = method;
    query->handle = (void *)(uintptr_t)response.query_id;
    query->callback.opaque = callback;
    bridge_trace( "Matchmaking direct method=%u ip=%u port=%u query=%u",
                  method, ip, port, query->id );
    bridge_mm_dispatch( &response );
    return (int32_t)response.query_id;
}

static int32_t bridge_mm_ping( void *, uint32_t ip, uint16_t port,
                               u_ISteamMatchmakingPingResponse *callback )
{
    return bridge_mm_direct_request( 13, ip, port, callback );
}

static int32_t bridge_mm_players( void *, uint32_t ip, uint16_t port,
                                  u_ISteamMatchmakingPlayersResponse *callback )
{
    return bridge_mm_direct_request( 14, ip, port, callback );
}

static int32_t bridge_mm_rules( void *, uint32_t ip, uint16_t port,
                                u_ISteamMatchmakingRulesResponse *callback )
{
    return bridge_mm_direct_request( 15, ip, port, callback );
}

static void bridge_mm_cancel_server( void *, int32_t handle )
{
    uint32_t id = (uint32_t)handle;
    bridge_mm_proxy_query *query = bridge_mm_find( id );
    lsteam_bridge_mm_response response = {};
    bridge_mm_call( 16, (void *)(uintptr_t)id, 0, &response );
    if (query) memset( query, 0, sizeof(*query) );
}

static void bridge_mm_pump(void)
{
    uint32_t ids[16];
    size_t count = 0;
    for (size_t i = 0; i < ARRAY_SIZE(bridge_mm_queries); ++i)
        if (bridge_mm_queries[i].id) ids[count++] = bridge_mm_queries[i].id;
    for (size_t i = 0; i < count; ++i)
    {
        bridge_mm_proxy_query *query = bridge_mm_find( ids[i] );
        if (!query) continue;
        lsteam_bridge_mm_request mm = {};
        lsteam_bridge_mm_response response = {};
        mm.method = 17;
        mm.query_id = ids[i];
        if (bridge_mm_rpc( LSTEAM_BRIDGE_MATCHMAKING_CALL, &mm, &response ))
            bridge_mm_dispatch( &response );
    }
}
