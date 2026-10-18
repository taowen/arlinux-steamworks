struct bridge_html_call_completed {
    uint64_t call;
    int32_t callback_id;
    uint32_t callback_size;
};

#pragma pack(push, 4)
struct bridge_html_needs_paint {
    uint32_t browser;
    const char *bgra;
    uint32_t width;
    uint32_t height;
    uint32_t update_x;
    uint32_t update_y;
    uint32_t update_width;
    uint32_t update_height;
    uint32_t scroll_x;
    uint32_t scroll_y;
    float page_scale;
    uint32_t page_serial;
};
#pragma pack(pop)

static pthread_mutex_t bridge_html_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t bridge_html_call;
static uint32_t bridge_html_browser;
static uint32_t bridge_html_next_browser = 1;
static bool bridge_html_callback_pending;
static bool bridge_html_callback_outstanding;
static bool bridge_html_paint_pending;
static const char bridge_html_white_pixel[] = "\xff\xff\xff\xff";

static void bridge_html_destructor( void * ) {}

static int8_t bridge_html_init( void * )
{
    /* The headless surface is experimental.  Returning failure lets games
     * fall back to their native VGUI instead of waiting on an incomplete
     * embedded browser implementation. */
    bridge_trace( "HTML Init headless=0 (A/B test)" );
    return 0;
}

static int8_t bridge_html_shutdown( void * )
{
    return 1;
}

static uint64_t bridge_html_create( void *, const char *user_agent,
                                    const char *user_css )
{
    (void)user_agent;
    (void)user_css;
    pthread_mutex_lock( &bridge_html_lock );
    bridge_html_browser = bridge_html_next_browser++;
    if (!bridge_html_next_browser) bridge_html_next_browser = 1;
    bridge_html_call = 0x4c53485400000000ull | bridge_html_browser;
    bridge_html_callback_pending = true;
    bridge_html_callback_outstanding = false;
    uint64_t call = bridge_html_call;
    uint32_t browser = bridge_html_browser;
    pthread_mutex_unlock( &bridge_html_lock );
    bridge_trace( "HTML CreateBrowser headless call=%llu browser=%u",
                  (unsigned long long)call, browser );
    return call;
}

static void bridge_html_remove( void *, uint32_t browser )
{
    pthread_mutex_lock( &bridge_html_lock );
    if (bridge_html_browser == browser) {
        bridge_html_call = 0;
        bridge_html_browser = 0;
        bridge_html_callback_pending = false;
        bridge_html_paint_pending = false;
    }
    pthread_mutex_unlock( &bridge_html_lock );
    bridge_trace( "HTML RemoveBrowser handle=%u", browser );
}

static void bridge_html_set_size( void *, uint32_t browser,
                                  uint32_t width, uint32_t height )
{
    pthread_mutex_lock( &bridge_html_lock );
    /* A synthetic NeedsPaint with a texture smaller than the requested
     * surface can leave Source's HTML panel spinning forever. Until the
     * headless browser can provide a correctly sized backing store, leave
     * the surface unpainted instead of advertising an invalid frame. */
    if (bridge_html_browser == browser) bridge_html_paint_pending = false;
    pthread_mutex_unlock( &bridge_html_lock );
    bridge_trace( "HTML SetSize handle=%u requested=%ux%u paint=disabled",
                  browser, width, height );
}

static int8_t bridge_html_take_callback( u_CallbackMsg_t *msg,
                                         int32_t *steam_call )
{
    static __thread bridge_html_call_completed completed;
    static __thread bridge_html_needs_paint paint;
    pthread_mutex_lock( &bridge_html_lock );
    if (!bridge_html_callback_pending && !bridge_html_paint_pending) {
        pthread_mutex_unlock( &bridge_html_lock );
        return 0;
    }
    if (bridge_html_paint_pending && !bridge_html_callback_pending) {
        memset( &paint, 0, sizeof(paint) );
        paint.browser = bridge_html_browser;
        paint.bgra = bridge_html_white_pixel;
        paint.width = 1;
        paint.height = 1;
        paint.update_width = 1;
        paint.update_height = 1;
        paint.page_scale = 1.0f;
        paint.page_serial = 1;
        bridge_html_paint_pending = false;
        bridge_html_callback_outstanding = true;
        pthread_mutex_unlock( &bridge_html_lock );
        msg->m_hSteamUser = bridge_status.steam_user;
        msg->m_iCallback = 4502;
        msg->m_pubParam = (uint8_t *)&paint;
        msg->m_cubParam = sizeof(paint);
        if (steam_call) *steam_call = 0;
        bridge_trace( "HTML callback NeedsPaint browser=%u 1x1",
                      paint.browser );
        return 1;
    }
    completed.call = bridge_html_call;
    completed.callback_id = 4501;
    completed.callback_size = sizeof(uint32_t);
    bridge_html_callback_pending = false;
    bridge_html_callback_outstanding = true;
    pthread_mutex_unlock( &bridge_html_lock );
    msg->m_hSteamUser = bridge_status.steam_user;
    msg->m_iCallback = 703;
    msg->m_pubParam = (uint8_t *)&completed;
    msg->m_cubParam = sizeof(completed);
    if (steam_call) *steam_call = 0;
    bridge_trace( "HTML callback BrowserReady call=%llu",
                  (unsigned long long)completed.call );
    return 1;
}

static int8_t bridge_html_get_api_result( uint64_t call, void *callback,
                                          int size, int id, int8_t *failed )
{
    pthread_mutex_lock( &bridge_html_lock );
    bool match = call == bridge_html_call && id == 4501 &&
                 size == (int)sizeof(uint32_t) && callback;
    uint32_t browser = bridge_html_browser;
    pthread_mutex_unlock( &bridge_html_lock );
    if (!match) return 0;
    memcpy( callback, &browser, sizeof(browser) );
    if (failed) *failed = 0;
    bridge_trace( "HTML API result BrowserReady browser=%u", browser );
    return 1;
}

static int8_t bridge_html_free_callback(void)
{
    pthread_mutex_lock( &bridge_html_lock );
    bool outstanding = bridge_html_callback_outstanding;
    bridge_html_callback_outstanding = false;
    pthread_mutex_unlock( &bridge_html_lock );
    return outstanding;
}
