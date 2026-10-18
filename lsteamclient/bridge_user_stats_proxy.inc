/* Bionic-side SteamUserStats011/012 proxy. */
static bool bridge_user_stats_rpc(
    lsteam_bridge_game_server_request *request,
    lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_USER_STATS_CALL,
                             request, response );
}

static void bridge_user_stats_name(
    lsteam_bridge_game_server_request *request, const char *name )
{
    bridge_iface_copy_text( request->text, name );
}

static int8_t bridge_user_stats_request_current( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static int8_t bridge_user_stats_get_int(
    void *, const char *name, int32_t *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (value) memcpy( value, &response.arg0, sizeof(*value) );
    return 1;
}

static int8_t bridge_user_stats_get_float(
    void *, const char *name, float *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 2;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (value) memcpy( value, &response.arg0, sizeof(*value) );
    return 1;
}

static int8_t bridge_user_stats_set_int(
    void *, const char *name, int32_t value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 3;
    request.arg0 = (uint32_t)value;
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static int8_t bridge_user_stats_set_float(
    void *, const char *name, float value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    memcpy( &request.arg0, &value, sizeof(value) );
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static int8_t bridge_user_stats_update_rate(
    void *, const char *name, float count, double length )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 5;
    memcpy( &request.arg0, &count, sizeof(count) );
    request.data_size = sizeof(length);
    memcpy( request.data, &length, sizeof(length) );
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static int8_t bridge_user_stats_named_bool( uint32_t method,
                                             const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static int8_t bridge_user_stats_get_achievement(
    void *, const char *name, int8_t *achieved )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (achieved) *achieved = response.arg0 != 0;
    return 1;
}
static int8_t bridge_user_stats_set_achievement( void *, const char *name )
{ return bridge_user_stats_named_bool( 7, name ); }
static int8_t bridge_user_stats_clear_achievement( void *, const char *name )
{ return bridge_user_stats_named_bool( 8, name ); }

static int8_t bridge_user_stats_get_unlock(
    void *, const char *name, int8_t *achieved, uint32_t *unlock_time )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 9;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (achieved) *achieved = response.arg0 != 0;
    if (unlock_time) *unlock_time = response.arg1;
    return 1;
}

static int8_t bridge_user_stats_store( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 10;
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static int32_t bridge_user_stats_icon( void *, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static __thread char bridge_user_stats_text[LSTEAM_BRIDGE_MAX_TICKET];

static const char *bridge_user_stats_response_text(
    lsteam_bridge_game_server_response *response )
{
    if (!response->data_size ||
        response->data_size > sizeof(bridge_user_stats_text)) return "";
    memcpy( bridge_user_stats_text, response->data, response->data_size );
    bridge_user_stats_text[response->data_size - 1] = 0;
    return bridge_user_stats_text;
}

static const char *bridge_user_stats_display(
    void *, const char *name, const char *key )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 12;
    bridge_user_stats_name( &request, name );
    bridge_iface_copy_text( request.value, key );
    return bridge_user_stats_rpc( &request, &response ) ?
           bridge_user_stats_response_text( &response ) : "";
}

static int8_t bridge_user_stats_indicate(
    void *, const char *name, uint32_t current, uint32_t maximum )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 13;
    request.arg0 = current;
    request.arg1 = maximum;
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static uint32_t bridge_user_stats_num_achievements( void * )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 14;
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static const char *bridge_user_stats_achievement_name( void *, uint32_t index )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 15;
    request.arg0 = index;
    return bridge_user_stats_rpc( &request, &response ) ?
           bridge_user_stats_response_text( &response ) : "";
}

static uint64_t bridge_user_stats_request_user( void *, CSteamID user )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 16;
    request.steam_id = bridge_iface_id_value( user );
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_user_stats_get_user_value(
    uint32_t method, CSteamID user, const char *name, void *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = bridge_iface_id_value( user );
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (value) memcpy( value, &response.arg0, sizeof(response.arg0) );
    return 1;
}

static int8_t bridge_user_stats_get_user_int(
    void *, CSteamID user, const char *name, int32_t *value )
{
    uint32_t bits = 0;
    int8_t ok = bridge_user_stats_get_user_value( 17, user, name, &bits );
    if (ok && value) memcpy( value, &bits, sizeof(*value) );
    return ok;
}
static int8_t bridge_user_stats_get_user_float(
    void *, CSteamID user, const char *name, float *value )
{
    uint32_t bits = 0;
    int8_t ok = bridge_user_stats_get_user_value( 18, user, name, &bits );
    if (ok && value) memcpy( value, &bits, sizeof(*value) );
    return ok;
}
static int8_t bridge_user_stats_get_user_achievement(
    void *, CSteamID user, const char *name, int8_t *achieved )
{
    uint32_t value = 0;
    int8_t ok = bridge_user_stats_get_user_value( 19, user, name, &value );
    if (ok && achieved) *achieved = value != 0;
    return ok;
}

static int8_t bridge_user_stats_get_user_unlock(
    void *, CSteamID user, const char *name, int8_t *achieved,
    uint32_t *unlock_time )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 20;
    request.steam_id = bridge_iface_id_value( user );
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (achieved) *achieved = response.arg0 != 0;
    if (unlock_time) *unlock_time = response.arg1;
    return 1;
}

static int8_t bridge_user_stats_reset( void *, int8_t achievements )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 21;
    request.arg0 = achievements != 0;
    return bridge_user_stats_rpc( &request, &response ) && response.result;
}

static uint64_t bridge_user_stats_find_create(
    void *, const char *name, uint32_t sort, uint32_t display )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 22;
    request.arg0 = sort;
    request.arg1 = display;
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}
static uint64_t bridge_user_stats_find( void *, const char *name )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 23;
    bridge_user_stats_name( &request, name );
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_user_stats_leaderboard_result(
    uint32_t method, uint64_t leaderboard )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.steam_id = leaderboard;
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}
static const char *bridge_user_stats_leaderboard_name(
    void *, uint64_t leaderboard )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 24;
    request.steam_id = leaderboard;
    return bridge_user_stats_rpc( &request, &response ) ?
           bridge_user_stats_response_text( &response ) : "";
}
static int32_t bridge_user_stats_entry_count( void *, uint64_t leaderboard )
{ return bridge_user_stats_leaderboard_result( 25, leaderboard ); }
static uint32_t bridge_user_stats_sort( void *, uint64_t leaderboard )
{ return bridge_user_stats_leaderboard_result( 26, leaderboard ); }
static uint32_t bridge_user_stats_display_type( void *, uint64_t leaderboard )
{ return bridge_user_stats_leaderboard_result( 27, leaderboard ); }

static uint64_t bridge_user_stats_download(
    void *, uint64_t leaderboard, uint32_t data_request,
    int32_t start, int32_t end )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 28;
    request.steam_id = leaderboard;
    request.arg0 = data_request;
    request.arg1 = (uint32_t)start;
    request.arg2 = (uint32_t)end;
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_user_stats_download_users(
    void *, uint64_t leaderboard, CSteamID *users, int32_t count )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (count < 0 || count > 100 || (count && !users)) return 0;
    request.method = 29;
    request.steam_id = leaderboard;
    request.arg0 = count;
    request.data_size = count * sizeof(uint64_t);
    for (int32_t i = 0; i < count; ++i) {
        uint64_t value = bridge_iface_id_value( users[i] );
        memcpy( request.data + i * sizeof(value), &value, sizeof(value) );
    }
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_user_stats_downloaded_entry(
    void *, uint64_t entries, int32_t index,
    u_LeaderboardEntry_t_123 *entry, int32_t *details,
    int32_t details_max )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (details_max < 0 || details_max > 64 ||
        (details_max && !details)) return 0;
    request.method = 30;
    request.steam_id = entries;
    request.arg0 = (uint32_t)index;
    request.arg1 = details_max;
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result ||
        response.data_size < 12 ||
        (response.data_size - 12) % sizeof(int32_t)) return 0;
    int32_t count = (response.data_size - 12) / sizeof(int32_t);
    if (count > details_max) return 0;
    if (entry) {
        entry->m_steamIDUser = bridge_iface_make_id( response.steam_id );
        memcpy( &entry->m_nGlobalRank, &response.arg0,
                sizeof(entry->m_nGlobalRank) );
        memcpy( &entry->m_nScore, &response.arg1,
                sizeof(entry->m_nScore) );
        memcpy( &entry->m_cDetails, response.data,
                sizeof(entry->m_cDetails) );
        memcpy( &entry->m_hUGC, response.data + 4,
                sizeof(entry->m_hUGC) );
    }
    if (count) memcpy( details, response.data + 12,
                       count * sizeof(int32_t) );
    return 1;
}

static uint64_t bridge_user_stats_upload(
    void *, uint64_t leaderboard, uint32_t method, int32_t score,
    const int32_t *details, int32_t count )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if (count < 0 || count > 64 || (count && !details)) return 0;
    request.method = 31;
    request.steam_id = leaderboard;
    request.arg0 = method;
    request.arg1 = (uint32_t)score;
    request.arg2 = count;
    request.data_size = count * sizeof(int32_t);
    if (count) memcpy( request.data, details, request.data_size );
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_user_stats_attach(
    void *, uint64_t leaderboard, uint64_t ugc )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 32;
    request.steam_id = leaderboard;
    request.data_size = sizeof(ugc);
    memcpy( request.data, &ugc, sizeof(ugc) );
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_user_stats_simple_async( uint32_t method )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}
static uint64_t bridge_user_stats_current_players( void * )
{ return bridge_user_stats_simple_async( 33 ); }
static uint64_t bridge_user_stats_global_achievements( void * )
{ return bridge_user_stats_simple_async( 34 ); }

static int32_t bridge_user_stats_achievement_info(
    uint32_t method, int32_t previous, char *name, uint32_t name_size,
    float *percent, int8_t *achieved )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = (uint32_t)previous;
    if (!bridge_user_stats_rpc( &request, &response )) return -1;
    if (name && name_size)
        snprintf( name, name_size, "%s",
                  bridge_user_stats_response_text( &response ) );
    if (percent) memcpy( percent, &response.arg0, sizeof(*percent) );
    if (achieved) *achieved = response.arg1 != 0;
    return (int32_t)response.result;
}
static int32_t bridge_user_stats_most_achieved(
    void *, char *name, uint32_t size, float *percent, int8_t *achieved )
{
    return bridge_user_stats_achievement_info(
        35, 0, name, size, percent, achieved );
}
static int32_t bridge_user_stats_next_achieved(
    void *, int32_t previous, char *name, uint32_t size,
    float *percent, int8_t *achieved )
{
    return bridge_user_stats_achievement_info(
        36, previous, name, size, percent, achieved );
}

static int8_t bridge_user_stats_achievement_percent(
    void *, const char *name, float *percent )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 37;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (percent) memcpy( percent, &response.arg0, sizeof(*percent) );
    return 1;
}

static uint64_t bridge_user_stats_request_global( void *, int32_t days )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 38;
    request.arg0 = (uint32_t)days;
    return bridge_user_stats_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_user_stats_global_value(
    uint32_t method, const char *name, void *value )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result ||
        response.data_size != sizeof(uint64_t)) return 0;
    if (value) memcpy( value, response.data, sizeof(uint64_t) );
    return 1;
}
static int8_t bridge_user_stats_global_int64(
    void *, const char *name, int64_t *value )
{ return bridge_user_stats_global_value( 39, name, value ); }
static int8_t bridge_user_stats_global_double(
    void *, const char *name, double *value )
{ return bridge_user_stats_global_value( 40, name, value ); }

static int32_t bridge_user_stats_global_history(
    uint32_t method, const char *name, void *data, uint32_t bytes )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    if ((bytes && !data) || bytes % sizeof(uint64_t)) return 0;
    if (bytes > LSTEAM_BRIDGE_MAX_TICKET) bytes = LSTEAM_BRIDGE_MAX_TICKET;
    request.method = method;
    request.arg0 = bytes;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) ||
        response.data_size > bytes) return 0;
    if (response.data_size) memcpy( data, response.data, response.data_size );
    return (int32_t)response.result;
}
static int32_t bridge_user_stats_history_int64(
    void *, const char *name, int64_t *data, uint32_t bytes )
{ return bridge_user_stats_global_history( 41, name, data, bytes ); }
static int32_t bridge_user_stats_history_double(
    void *, const char *name, double *data, uint32_t bytes )
{ return bridge_user_stats_global_history( 42, name, data, bytes ); }

static int8_t bridge_user_stats_limits_int(
    void *, const char *name, int32_t *minimum, int32_t *maximum )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 43;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (minimum) memcpy( minimum, &response.arg0, sizeof(*minimum) );
    if (maximum) memcpy( maximum, &response.arg1, sizeof(*maximum) );
    return 1;
}
static int8_t bridge_user_stats_limits_float(
    void *, const char *name, float *minimum, float *maximum )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 44;
    bridge_user_stats_name( &request, name );
    if (!bridge_user_stats_rpc( &request, &response ) || !response.result)
        return 0;
    if (minimum) memcpy( minimum, &response.arg0, sizeof(*minimum) );
    if (maximum) memcpy( maximum, &response.arg1, sizeof(*maximum) );
    return 1;
}
