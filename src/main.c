#include <mini_SBC.h>

#define THIS_FILE "main.c"

static pjsip_endpoint *sip_endpt;
static pj_bool_t quit_flag;
static pjsip_dialog *dlg;

static int handler_events(void *arg)
{
    PJ_UNUSED_ARG(arg);

    while (!quit_flag)
    {
        pj_time_val timeout = {0, 500};
        pjsip_endpt_handle_events(sip_endpt, &timeout);
    }

    return 0;
}

static pj_status_t on_rx_request(pjsip_rx_data *rdata)
{

    PJ_LOG(3, (THIS_FILE, "req Addr: %d",
               rx_get_port(rdata)));
    
}

static pj_status_t on_rx_response(pjsip_rx_data *rdata)
{
    PJ_LOG(3, (THIS_FILE, "res Addr: %s",
               rdata->pkt_info.src_addr));
}

static void on_tsx_state(pjsip_transaction *tsx, pjsip_event *event)
{
    PJ_LOG(3, (THIS_FILE, "Role: %d, Method: %s, Type event: %d",
               tsx->role, tsx->method.name, event->type));
}

static pjsip_module app_module =
    {
        NULL, NULL,                     /* prev, next.*/
        {"app_module", 12},             /* Name.*/
        -1,                             /* Id, automatically*/
        PJSIP_MOD_PRIORITY_APPLICATION, /* Priority*/
        NULL,                           /* load()*/
        NULL,                           /* start()*/
        NULL,                           /* stop()*/
        NULL,                           /* unload()*/
        &on_rx_request,                           /* on_rx_request(), incoming message*/
        &on_rx_response,                           /* on_rx_response()*/
        NULL,                           /* on_tx_request.*/
        NULL,                           /* on_tx_response()*/
        &on_tsx_state                   /* on_tsx_state()*/
};

/*
 * main()
 *
 */
int main(int argc, char *argv[])
{
    pj_caching_pool cp;
    pj_thread_t *thread;
    pj_pool_t *pool;
    pj_status_t status;
    pjsip_tx_data *p_data;

    /* Must init PJLIB first: */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Then init PJLIB-UTIL: */
    status = pjlib_util_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Must create a pool factory before we can allocate any memory. */
    pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

    /* Create the endpoint: */
    status = pjsip_endpt_create(&cp.factory, "sipstateless",
                                &sip_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    pj_sockaddr_in addr;

    addr.sin_family = pj_AF_INET();
    addr.sin_addr.s_addr = 0;
    addr.sin_port = pj_htons(FIRST_PORT);

    status = pjsip_udp_transport_start(sip_endpt, &addr, NULL, 1, NULL);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }

    addr.sin_port = pj_htons(SECOND_PORT);

     status = pjsip_udp_transport_start(sip_endpt, &addr, NULL, 1, NULL);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }

    status = pjsip_tsx_layer_init_module(sip_endpt);
    pj_assert(status == PJ_SUCCESS);

    status = pjsip_ua_init_module(sip_endpt, NULL);
    pj_assert(status == PJ_SUCCESS);

    status = pjsip_endpt_register_module(sip_endpt, &app_module);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    pool = pjsip_endpt_create_pool(sip_endpt, "", 1000, 1000);

    status = pj_thread_create(pool, "", &handler_events, NULL, 0, 0, &thread);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    for (;;)
    {
        char line[10];

        fgets(line, sizeof(line), stdin);

        switch (line[0])
        {
        case 'q':
            break;
        }
    }
    quit_flag = 1;
    return 0;
}
