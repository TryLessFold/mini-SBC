#include <mini_SBC.h>

#define THIS_FILE "main.c"

my_config_t app = {0};
pj_bool_t quit_flag = 0;

static pj_bool_t on_rx_request(pjsip_rx_data *rdata);
static pj_bool_t on_rx_response(pjsip_rx_data *rdata);
static void on_rx_rtp(void *user_data, void *pkt, pj_ssize_t size);
static void on_rx_rtcp(void *user_data, void *pkt, pj_ssize_t size);

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
        NULL                            /* on_tsx_state()*/
};

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
    PJ_LOG(3, (THIS_FILE, "RXReq"));
    pjsip_tx_data *tdata;
    pj_status_t status;
    pjsip_sip_uri *target;
    pjsip_host_port local_host;
    pj_sockaddr from_addr;
    short num_to;
    short num_from;

    /* Create tdata */
    status = pjsip_endpt_create_request_fwd(app.sip_endpt, rdata, NULL, NULL, 0, &tdata);
    if (status != PJ_SUCCESS)
    {
        pjsip_endpt_respond_stateless(app.sip_endpt, rdata,
                                      PJSIP_SC_INTERNAL_SERVER_ERROR, NULL,
                                      NULL, NULL);
        app_perror(THIS_FILE, "Can't create tdata", status);
    }
    pjsip_msg_find_remove_hdr(tdata->msg, PJSIP_H_VIA, NULL);

    if (rx_get_port(rdata) == app.port[0])
    {
        num_from = 0;
        num_to = 1;
    }
    else if (rx_get_port(rdata) == app.port[1])
    {
        num_from = 1;
        num_to = 0;
    }

    local_host.host = app.local_addr;
    local_host.port = app.port[num_to];

    /* Creates transport for specific port */
    status = SBC_calculate_transports(rdata, num_to, num_from);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error calculating transports", status);
    }

    /* Change headers for hide dest host and request request */
    SBC_request_redirect(tdata, app.hosts[num_to], local_host);

    if (rx_get_method(rdata)->id == PJSIP_INVITE_METHOD)
    {
        SBC_tx_redirect_sdp(tdata, num_to, &from_addr);
        status = pjmedia_transport_attach(app.media.trans[num_from].port, &app.media.trans[num_to].id,
                                          &from_addr,
                                          NULL,
                                          sizeof(pj_sockaddr),
                                          &on_rx_rtp,
                                          &on_rx_rtcp);
        pjmedia_transport_media_start(app.media.trans[num_from].port, 0, 0, 0, 0);
    }

    /* Forward request */
    pj_sockaddr dst_host =
        {
            .ipv4 = {
                PJ_AF_INET,
                pj_htons(app.hosts[num_to].port),
                pj_inet_addr(&app.hosts[num_to].host)}};
    status = pjsip_transport_send(app.trans_port[num_to],
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
    pjsip_tx_data_dec_ref(tdata);
}

static pj_bool_t on_rx_response(pjsip_rx_data *rdata)
{
    PJ_LOG(3, (THIS_FILE, "RXRes"));
    pjsip_tx_data *tdata;
    pjsip_response_addr res_addr;
    pjsip_via_hdr *hdr_via;
    pj_status_t status;
    pjsip_host_port local_host;
    pj_sockaddr from_addr;
    short num_to;
    short num_from;

    /* Create txdata and clone msg from rx to tx */
    status = pjsip_endpt_create_tdata(app.sip_endpt, &tdata);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error creating response", status);
        return PJ_TRUE;
    }
    pjsip_tx_data_add_ref(tdata);

    tdata->msg = pjsip_msg_clone(app.pool, rdata->msg_info.msg);

    /* Get topmost Via header */
    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    if (hdr_via == NULL)
    {
        /* Invalid response! Just drop it */
        pjsip_tx_data_dec_ref(tdata);
        return PJ_TRUE;
    }

    /* Change headers for hide dest host and redirect response */
    if (rx_get_port(rdata) == app.port[0])
    {
        num_to = 1;
        num_from = 0;
    }
    else if (rx_get_port(rdata) == app.port[1])
    {
        num_to = 0;
        num_from = 1;
    }

    status = SBC_calculate_transports(rdata, num_to, num_from);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error calculating transports", status);
        return PJ_TRUE;
    }

    local_host.host = app.local_addr;
    local_host.port = app.port[num_to];

    pj_bzero(&res_addr, sizeof(res_addr));
    SBC_response_redirect(tdata, app.hosts[num_to], local_host, &res_addr);

    if (rx_get_status(rdata) == PJSIP_SC_OK)
    {
        SBC_tx_redirect_sdp(tdata, num_to, &from_addr);
        if (tdata->msg->body != NULL)
        {
            status = pjmedia_transport_attach(app.media.trans[num_from].port, &app.media.trans[num_to].id,
                                              &from_addr,
                                              NULL,
                                              sizeof(pj_sockaddr),
                                              &on_rx_rtp,
                                              &on_rx_rtcp);
            pjmedia_transport_media_start(app.media.trans[num_from].port, 0, 0, 0, 0);
        }
    }
    /* Forward response */
    pj_sockaddr dst_host =
        {
            .ipv4 = {
                PJ_AF_INET,
                pj_htons(res_addr.dst_host.addr.port),
                pj_inet_addr(&res_addr.dst_host.addr.host)}};
    status = pjsip_transport_send(app.trans_port[num_to],
                                  tdata,
                                  (pj_sockaddr_t *)&dst_host,
                                  sizeof(dst_host),
                                  NULL,
                                  NULL);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error forwarding response", status);
        return PJ_TRUE;
    }
    pjsip_tx_data_dec_ref(tdata);
}

static void on_rx_rtp(void *user_data, void *pkt, pj_ssize_t size)
{
    pj_status_t status;
    int *num = (int *)user_data;
    pjmedia_transport_info tp_info;
    pjmedia_transport_info_init(&tp_info);
    pjmedia_transport_get_info(app.media.trans[*num].port, &tp_info);
    PJ_LOG(3, (THIS_FILE, "RX_RTP Transport %d %d", *num, pj_ntohs(tp_info.sock_info.rtp_addr_name.ipv4.sin_port)));
    status = pjmedia_transport_send_rtp(app.media.trans[*num].port,
                                        pkt, size);
    if (status != PJ_SUCCESS)
        app_perror(THIS_FILE, "Error forwarding RTP", status);
}

static void on_rx_rtcp(void *user_data, void *pkt, pj_ssize_t size)
{
    pj_status_t status;
    int *num = (int *)user_data;
    pjmedia_transport_info tp_info;
    pjmedia_transport_info_init(&tp_info);
    pjmedia_transport_get_info(app.media.trans[*num].port, &tp_info);
    PJ_LOG(3, (THIS_FILE, "RX_RTCP Transport %d %d", *num, pj_ntohs(tp_info.sock_info.rtp_addr_name.ipv4.sin_port)));
    status = pjmedia_transport_send_rtcp(app.media.trans[*num].port,
                                         pkt, size);
    if (status != PJ_SUCCESS)
        app_perror(THIS_FILE, "Error forwarding RTCP", status);
}

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
                                  0, &app.media.endpt);
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
    pj_uint16_t rtp_port;
    int i, count;

    pjmedia_codec_g711_init(app.media.endpt);

    /* RTP port counter */
    // rtp_port = (pj_uint16_t)(app.media.rtp_port & 0xFFFE);

    // for (i = 0; i < app.hosts_cnt; i++)
    // {
    //     status = SBC_create_transport(app.media.trans[num_to].port[i], &rtp_port);
    //     if (status != PJ_SUCCESS)
    //     {
    //         app_perror(THIS_FILE, "Media transport couldn't be created (port in use?)", status);
    //         return -1;
    //     }
    // }
    return PJ_SUCCESS;
}

/*
 * main()
 *
 */
int main(int argc, char *argv[])
{
    pj_status_t status;

    /* Must be initialized first */
    status = init_pj();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Must be initializ
    ed second */
    status = init_options(argc, argv);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = init_sip();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = init_SBC();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = init_media();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    for (;;)
    {
        char line[10];

        fgets(line, sizeof(line), stdin);

        switch (line[0])
        {
        case 'q':
            goto return_exit;
        }
    }

return_exit:
    pj_thread_destroy(app.thread);

    if (app.pool)
    {
        pj_pool_release(app.pool);
        app.pool = NULL;
        pj_caching_pool_destroy(&app.cp);
    }

    pj_shutdown();
    quit_flag = 1;
    return 0;
}
