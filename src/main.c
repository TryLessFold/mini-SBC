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

static pj_bool_t on_rx_request(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
    pj_status_t status;
    pjsip_sip_uri *target;
    status = pjsip_endpt_create_request_fwd(app.sip_endpt, rdata, NULL, NULL, 0, &tdata);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Can't create tdata", status);
    }
    SBC_redirect(tdata);
    // status = pjsip_endpt_send_request_stateless(app.sip_endpt, tdata,
    //                                             NULL, NULL);
    pj_str_t tmp = pj_str(FIRST_ADDR_FROMTO);
    pj_sockaddr lala = {
        .ipv4 = {
            PJ_AF_INET,
            pj_htons(5060),
            pj_inet_addr(&tmp)}};
    status = pjsip_transport_send(app.trans_port[1],
                                  tdata,
                                  (pj_sockaddr_t *)&lala,
                                  sizeof(lala),
                                  NULL,
                                  NULL);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error forwarding request", status);
        return PJ_TRUE;
    }
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

static void rx_callback(pjsip_endpoint *sip_endpt, pj_status_t status, pjsip_rx_data *rdata)
{
    PJ_LOG(3, (THIS_FILE, "rx_callback Addr: %d",
               rx_get_port(rdata)));
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
        &on_rx_request,                 /* on_rx_request(), incoming message*/
        &on_rx_response,                /* on_rx_response()*/
        NULL,                           /* on_tx_request.*/
        NULL,                           /* on_tx_response()*/
        &on_tsx_state                   /* on_tsx_state()*/
};

static pj_status_t init_pj()
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

    /* Create pool for misc. */
    app.pool = pj_pool_create(&app.cp.factory, "misc", 1000, 1000, NULL);

    /* Create the sip endpoint: */
    status = pjsip_endpt_create(&app.cp.factory, "sipstateless", &app.sip_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Create media endpoint */
    status = pjmedia_endpt_create(&app.cp.factory, pjsip_endpt_get_ioqueue(app.sip_endpt),
                                  0, &app.media_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
}

static pj_status_t init_sip()
{
    pj_status_t status;
    pj_sockaddr_in addr;

    addr.sin_family = pj_AF_INET();
    addr.sin_addr.s_addr = 0;
    addr.sin_port = pj_htons(FIRST_PORT);

    /* Create UDP transport on FIRST_PORT */
    status = pjsip_udp_transport_start(app.sip_endpt, &addr, NULL, 1, &app.trans_port[0]);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }
    app.ports[0] = FIRST_PORT;

    /* Create UDP transport on SECOND_PORT */
    addr.sin_port = pj_htons(SECOND_PORT);
    status = pjsip_udp_transport_start(app.sip_endpt, &addr, NULL, 1, &app.trans_port[1]);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }
    app.ports[1] = SECOND_PORT;

    app.tpmgr = app.trans_port[0]->tpmgr;

    status = pjsip_tsx_layer_init_module(app.sip_endpt);
    pj_assert(status == PJ_SUCCESS);

    status = pjsip_ua_init_module(app.sip_endpt, NULL);
    pj_assert(status == PJ_SUCCESS);

    status = pjsip_endpt_register_module(app.sip_endpt, &app_module);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = pj_thread_create(app.pool, "", &handler_events, NULL, 
        0, 0, &app.thread);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
}

static pj_status_t init_media()
{
    pj_status_t status;
}

/*
 * main()
 *
 */
int main(int argc, char *argv[])
{
    pj_status_t status;
    status = init_pj();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
   
    status = init_sip();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
   
    // status = init_pj();
    // PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

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
