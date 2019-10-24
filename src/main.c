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
    PJ_LOG(3, (THIS_FILE, "RX"));
    pjsip_tx_data *tdata;
    pj_status_t status;
    pjsip_sip_uri *target;
    int num_host;

    status = pjsip_endpt_create_request_fwd(app.sip_endpt, rdata, NULL, NULL, 0, &tdata);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Can't create tdata", status);
    }
    pjsip_msg_find_remove_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    SBC_request_redirect(tdata, &num_host);

    pj_sockaddr dst_host =
        {
            .ipv4 = {
                PJ_AF_INET,
                pj_htons(app.hosts[num_host].port),
                pj_inet_addr(&app.hosts[num_host].host)}};
    status = pjsip_transport_send(app.trans_port[num_host],
                                  tdata,
                                  (pj_sockaddr_t *)&dst_host,
                                  sizeof(dst_host),
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
    PJ_LOG(3, (THIS_FILE, "RX"));
    pjsip_tx_data *tdata, *tedata;
    pjsip_response_addr res_addr;
    pjsip_via_hdr *hdr_via;
    pj_status_t status;

    /* Create response to be forwarded upstream (Via will be stripped here) */
    status = pjsip_endpt_create_tdata(app.sip_endpt, &tdata);
    if (status != PJ_SUCCESS)
    {
       app_perror(THIS_FILE, "Error creating response", status);
       return PJ_TRUE;
    }
    pjsip_tx_data_add_ref(tdata);

    tdata->msg = pjsip_msg_clone(app.pool, rdata->msg_info.msg);

    status = pjsip_endpt_create_response_fwd(app.sip_endpt, rdata, 0, &tedata);
    if (status != PJ_SUCCESS)
    {
       app_perror(THIS_FILE, "Error creating response", status);
       return PJ_TRUE;
    }

    /* Get topmost Via header */
    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    if (hdr_via == NULL)
    {
        /* Invalid response! Just drop it */
        pjsip_tx_data_dec_ref(tdata);
        return PJ_TRUE;
    }

    SBC_response_redirect(tdata, &res_addr);

    /* Forward response */
    pj_sockaddr dst_host =
        {
            .ipv4 = {
                PJ_AF_INET,
                pj_htons(res_addr.dst_host.addr.port),
                pj_inet_addr(&res_addr.dst_host.addr.host)}};
    status = pjsip_transport_send(res_addr.transport,
                                  tdata,
                                  (pj_sockaddr_t *)&dst_host,
                                  sizeof(dst_host),
                                  NULL,
                                  NULL);
    // status = pjsip_endpt_send_response(app.sip_endpt, &res_addr, tdata, 
	// 			       NULL, NULL);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error forwarding response", status);
        return PJ_TRUE;
    }
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
    addr.sin_port = pj_htons(app.port[0]);

    /* Create UDP transport on FIRST_PORT */
    status = pjsip_udp_transport_start(app.sip_endpt, &addr, NULL, 1, &app.trans_port[0]);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }

    /* Create UDP transport on SECOND_PORT */
    addr.sin_port = pj_htons(app.port[1]);
    status = pjsip_udp_transport_start(app.sip_endpt, &addr, NULL, 1, &app.trans_port[1]);
    if (status != PJ_SUCCESS)
    {
        PJ_LOG(3, (THIS_FILE,
                   "Error starting UDP transport (port in use?)"));
        return 1;
    }

    app.tpmgr = app.trans_port[0]->tpmgr;

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

    status = init_options(argc, argv);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = init_sip();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = init_SBC();
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
