/* Read-only/query core for SteamUGC014 and SteamUGC015. */
static uint32_t bridge_ugc_version( void *iface )
{
    return iface == &bridge_ugc14 ? 14 : 15;
}

static bool bridge_ugc_rpc( lsteam_bridge_game_server_request *request,
                            lsteam_bridge_game_server_response *response )
{
    return bridge_iface_rpc( LSTEAM_BRIDGE_UGC_CALL, request, response );
}

static uint64_t bridge_ugc_create_user_query(
    void *iface, uint32_t account, uint32_t list_type, uint32_t matching,
    uint32_t sort, uint32_t creator, uint32_t consumer, uint32_t page )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 0;
    request.arg0 = account;
    request.arg1 = list_type;
    request.arg2 = matching;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = sort;
    request.data_size = 12;
    memcpy( request.data, &creator, 4 );
    memcpy( request.data + 4, &consumer, 4 );
    memcpy( request.data + 8, &page, 4 );
    return bridge_ugc_rpc( &request, &response ) ? response.result : 0;
}

static uint64_t bridge_ugc_send_query( void *iface, uint64_t query )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 1;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = query;
    return bridge_ugc_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_ugc_set_bool( void *iface, uint32_t method,
                                   uint64_t query, int8_t enabled )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = method;
    request.arg0 = enabled != 0;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = query;
    return bridge_ugc_rpc( &request, &response ) && response.result;
}
static int8_t bridge_ugc_set_long_description(
    void *iface, uint64_t query, int8_t enabled )
{ return bridge_ugc_set_bool( iface, 2, query, enabled ); }
static int8_t bridge_ugc_set_metadata(
    void *iface, uint64_t query, int8_t enabled )
{ return bridge_ugc_set_bool( iface, 3, query, enabled ); }

static uint32_t bridge_ugc_subscribed_count( void *iface )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 4;
    request.arg3 = bridge_ugc_version( iface );
    return bridge_ugc_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_ugc_get_query_result(
    void *iface, uint64_t query, uint32_t index,
    u_SteamUGCDetails_t_128x *details )
{
    if (!details) return 0;
    lsteam_bridge_ugc_details wire = {};
    uint32_t offset = 0;
    int8_t result = 0;

    while (offset < sizeof(wire))
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 5;
        request.arg0 = index;
        request.arg1 = offset;
        request.arg3 = bridge_ugc_version( iface );
        request.steam_id = query;
        if (!bridge_ugc_rpc( &request, &response ) ||
            response.arg0 != sizeof(wire) || !response.data_size ||
            response.data_size > sizeof(wire) - offset)
            return 0;
        memcpy( (uint8_t *)&wire + offset, response.data,
                response.data_size );
        offset += response.data_size;
        result = response.result != 0;
    }

    memset( details, 0, sizeof(*details) );
    details->m_nPublishedFileId = wire.published_file_id;
    details->m_eResult = wire.result;
    details->m_eFileType = wire.file_type;
    details->m_nCreatorAppID = wire.creator_app_id;
    details->m_nConsumerAppID = wire.consumer_app_id;
    memcpy( details->m_rgchTitle.data(), wire.title, sizeof(wire.title) );
    memcpy( details->m_rgchDescription.data(), wire.description,
            sizeof(wire.description) );
    details->m_ulSteamIDOwner = wire.owner_steam_id;
    details->m_rtimeCreated = wire.time_created;
    details->m_rtimeUpdated = wire.time_updated;
    details->m_rtimeAddedToUserList = wire.time_added_to_user_list;
    details->m_eVisibility = wire.visibility;
    details->m_bBanned = wire.banned;
    details->m_bAcceptedForUse = wire.accepted_for_use;
    details->m_bTagsTruncated = wire.tags_truncated;
    memcpy( details->m_rgchTags.data(), wire.tags, sizeof(wire.tags) );
    details->m_hFile = wire.file_handle;
    details->m_hPreviewFile = wire.preview_file_handle;
    memcpy( details->m_pchFileName.data(), wire.file_name,
            sizeof(wire.file_name) );
    details->m_nFileSize = wire.file_size;
    details->m_nPreviewFileSize = wire.preview_file_size;
    memcpy( details->m_rgchURL.data(), wire.url, sizeof(wire.url) );
    details->m_unVotesUp = wire.votes_up;
    details->m_unVotesDown = wire.votes_down;
    details->m_flScore = wire.score;
    details->m_unNumChildren = wire.child_count;
    return result;
}

static int8_t bridge_ugc_release_query( void *iface, uint64_t query )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 6;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = query;
    return bridge_ugc_rpc( &request, &response ) && response.result;
}

static uint32_t bridge_ugc_get_subscribed_items(
    void *iface, uint64_t *items, uint32_t maximum )
{
    if (maximum && !items) return 0;
    uint32_t copied = 0, total = 0;
    do
    {
        lsteam_bridge_game_server_request request = {};
        lsteam_bridge_game_server_response response = {};
        request.method = 7;
        request.arg0 = maximum;
        request.arg1 = copied;
        request.arg3 = bridge_ugc_version( iface );
        if (!bridge_ugc_rpc( &request, &response )) return copied;
        total = response.arg0;
        if (total > maximum || response.data_size % sizeof(uint64_t) ||
            response.data_size > (total - copied) * sizeof(uint64_t))
            return copied;
        uint32_t count = response.data_size / sizeof(uint64_t);
        if (count) memcpy( items + copied, response.data, response.data_size );
        copied += count;
        if (!count) break;
    } while (copied < total);
    return total;
}

static uint32_t bridge_ugc_get_item_state( void *iface, uint64_t item )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 8;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = item;
    return bridge_ugc_rpc( &request, &response ) ? response.result : 0;
}

static int8_t bridge_ugc_get_item_install_info(
    void *iface, uint64_t item, uint64_t *size_on_disk, char *folder,
    uint32_t folder_size, uint32_t *timestamp )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 9;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = item;
    if (!bridge_ugc_rpc( &request, &response ) ||
        response.data_size < sizeof(uint64_t))
        return 0;
    if (size_on_disk) memcpy( size_on_disk, response.data, sizeof(uint64_t) );
    if (timestamp) *timestamp = response.arg0;
    if (folder && folder_size)
    {
        uint32_t available = response.data_size - sizeof(uint64_t);
        uint32_t copy = available < folder_size - 1 ? available : folder_size - 1;
        memcpy( folder, response.data + sizeof(uint64_t), copy );
        folder[copy] = 0;
    }
    return response.result != 0;
}

static int8_t bridge_ugc_get_item_download_info(
    void *iface, uint64_t item, uint64_t *downloaded, uint64_t *total )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 10;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = item;
    if (!bridge_ugc_rpc( &request, &response ) || response.data_size != 16)
        return 0;
    if (downloaded) memcpy( downloaded, response.data, 8 );
    if (total) memcpy( total, response.data + 8, 8 );
    return response.result != 0;
}

static int8_t bridge_ugc_download_item(
    void *iface, uint64_t item, int8_t high_priority )
{
    lsteam_bridge_game_server_request request = {};
    lsteam_bridge_game_server_response response = {};
    request.method = 11;
    request.arg0 = high_priority != 0;
    request.arg3 = bridge_ugc_version( iface );
    request.steam_id = item;
    return bridge_ugc_rpc( &request, &response ) && response.result;
}
