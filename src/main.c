#include <mini_SBC.h>

#define THIS_FILE "main.c"

my_config_t app;
pj_bool_t quit_flag;

static int handler_events(void *arg)
{
    PJ_UNUSED_ARG(arg);

    while (!quit_flag)
    {
        pj_time_val timeout = {0, 500};
        pjsip_endpt_handle_events(app.sip_endpt, &timeout);
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

static pj_status_t init_SBC()
{
    pj_status_t status;
    /* Must init PJLIB first: */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Then init PJLIB-UTIL: */
    status = pjlib_util_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Must create a pool factory before we can allocate any memory. */
    pj_caching_pool_init(&app.cp, &pj_pool_factory_default_policy, 0);

    /* Create the endpoint: */
    status = pjsip_endpt_create(&app.cp.factory, "sipstateless",
                                &app.sip_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    pj_sockaddr_in addr;

    addr.sin_family = pj_AF_INET();
    addr.sin_addr.s_addr = 0;
    addr.sin_port = pj_htons(FIRST_PORT);

    status = pjsip_udp_transport_start(app.sip_endpt, &addr, NULL, 1, &app.trans_port[0]);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }

    addr.sin_port = pj_htons(SECOND_PORT);

     status = pjsip_udp_transport_start(app.sip_endpt, &addr, NULL, 1, &app.trans_port[1]);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }

    status = pjsip_tsx_layer_init_module(app.sip_endpt);
    pj_assert(status == PJ_SUCCESS);

    status = pjsip_ua_init_module(app.sip_endpt, NULL);
    pj_assert(status == PJ_SUCCESS);

    status = pjsip_endpt_register_module(app.sip_endpt, &app_module);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    app.pool = pjsip_endpt_create_pool(app.sip_endpt, "", 1000, 1000);

    status = pj_thread_create(app.pool, "", &handler_events, NULL, 0, 0, &app.thread);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
}

/*
 * main()
 *
 */
int main(int argc, char *argv[])
{
    pj_status_t status;
    status = init_SBC();
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
