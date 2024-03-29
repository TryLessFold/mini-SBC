#include <mini_SBC.h>

#define THIS_FILE "mini_SBC.c"

void app_perror(const char *sender,
                const char *title,
                pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));
    PJ_LOG(1, (sender, "%s: %s [code=%d]", title, errmsg, status));
}

pj_status_t SBC_request_redirect(pjsip_tx_data *tdata,
                                 pjsip_host_port dest_host,
                                 pjsip_host_port local_host)
{
    pjsip_sip_uri *target;
    pjsip_to_hdr *hdr_to;
    pjsip_from_hdr *hdr_from;
    pjsip_via_hdr *hdr_via;
    pjsip_contact_hdr *hdr_cont;
    char addr[PJ_INET_ADDRSTRLEN];
    pj_str_t addr_str;

    target = (pjsip_sip_uri *)tdata->msg->line.req.uri;

    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    hdr_to = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_TO, NULL);
    hdr_from = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_FROM, NULL);
    hdr_cont = (pjsip_contact_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_CONTACT, NULL);

    SBC_to_hdr_redirect(hdr_to, dest_host.host, dest_host.port);
    SBC_from_hdr_hide_host(hdr_from, local_host.host, local_host.port);
    SBC_via_hdr_hide_host(hdr_via, local_host.host, local_host.port);
    if (hdr_cont != NULL)
        SBC_contact_hdr_hide_host(hdr_cont, local_host.host, local_host.port);
    pj_strdup(app.pool, &target->host, &dest_host.host);
    target->port = dest_host.port;
}

pj_status_t SBC_response_redirect(pjsip_tx_data *tdata,
                                  pjsip_host_port dest_host,
                                  pjsip_host_port local_host,
                                  pjsip_response_addr *res_addr)
{
    pjsip_to_hdr *hdr_to;
    pjsip_from_hdr *hdr_from;
    pjsip_via_hdr *hdr_via;
    pjsip_contact_hdr *hdr_cont;

    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    hdr_to = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_TO, NULL);
    hdr_from = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_FROM, NULL);
    hdr_cont = (pjsip_contact_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_CONTACT, NULL);

    SBC_to_hdr_redirect(hdr_to, local_host.host, local_host.port);
    res_addr->dst_host.type = TRANSPORT_TYPE;
    res_addr->dst_host.flag = pjsip_transport_get_flag_from_type(TRANSPORT_TYPE);
    SBC_from_hdr_hide_host(hdr_from, dest_host.host, dest_host.port);
    SBC_via_hdr_hide_host(hdr_via, dest_host.host, dest_host.port);
    hdr_via->recvd_param = dest_host.host;
    hdr_via->rport_param = dest_host.port;
    if (hdr_cont != NULL)
        SBC_contact_hdr_hide_host(hdr_cont, local_host.host, local_host.port);
    res_addr->dst_host.addr.host = dest_host.host;
    res_addr->dst_host.addr.port = dest_host.port;
}

pj_status_t SBC_via_hdr_hide_host(pjsip_via_hdr *via, pj_str_t host, int port)
{
    via->sent_by.host.ptr = host.ptr;
    via->sent_by.host.slen = host.slen;
    via->sent_by.port = port;
    via->recvd_param.ptr = "";
    via->recvd_param.slen = 0;
    via->rport_param = 0;
}

pj_status_t SBC_to_hdr_redirect(pjsip_to_hdr *to,
                                pj_str_t host,
                                int port)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *)pjsip_uri_get_uri(to->uri);
    uri->host = pj_str(host.ptr);
}

pj_status_t SBC_from_hdr_hide_host(pjsip_from_hdr *from,
                                   pj_str_t host,
                                   int port)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *)pjsip_uri_get_uri(from->uri);
    uri->host.ptr = host.ptr;
    uri->host.slen = host.slen;
}

pj_status_t SBC_contact_hdr_hide_host(pjsip_contact_hdr *cont,
                                      pj_str_t host,
                                      int port)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *)pjsip_uri_get_uri(cont->uri);
    uri->host.ptr = host.ptr;
    uri->host.slen = host.slen;
    uri->port = port;
}

pj_status_t init_SBC()
{
    pj_status_t status;
    pj_sockaddr my_addr;
    if (app.hosts_cnt < 2)
    {
        app_perror(THIS_FILE, "Needs more ports (minimum 2)", -1);
        return -1;
    }
    if (app.media.rtp_port == 0)
    {
        app.media.rtp_port = DEFAULT_RTP_PORT;
        PJ_LOG(3, (THIS_FILE, "Rtp start port has been set as default"));
    }
    char addr[PJ_INET_ADDRSTRLEN];
    pj_sockaddr tmp;
    status = pj_getdefaultipinterface(pj_AF_INET(), &tmp);
    if (status != PJ_SUCCESS)
        return -1;
    status = pj_inet_ntop(pj_AF_INET(), &tmp.ipv4.sin_addr,
                          addr, sizeof(addr));
    if (status != PJ_SUCCESS)
        return -1;
    pj_strdup2(app.pool, &app.local_addr, addr);
    return PJ_SUCCESS;
}

pj_status_t SBC_tx_redirect_sdp(pjsip_tx_data *tdata,
                                short tp_num,
                                pj_sockaddr *from_addr)
{
    pj_status_t status;
    pjmedia_sdp_session *tmp_sdp, *sdp;
    pj_time_val tv;
    pjmedia_transport_info tpinfo;
    pjsip_msg_body *body;
    char buf[1024];
    pj_str_t str_buf;
    int length;

    if (tdata->msg->body != NULL)
    {
        from_addr->ipv4.sin_family = PJ_AF_INET;
        pjmedia_sdp_parse(app.pool, (char *)tdata->msg->body->data, tdata->msg->body->len, &tmp_sdp);
        sdp = pjmedia_sdp_session_clone(app.pool, tmp_sdp);
        pjmedia_transport_info_init(&tpinfo);
        pjmedia_transport_get_info(app.media.trans[tp_num].port, &tpinfo);

        from_addr->ipv4.sin_addr = pj_inet_addr(&sdp->origin.addr);
        pj_strcpy(&sdp->origin.addr, &app.local_addr);
        pj_strcpy(&sdp->conn->addr, &app.local_addr);
        pj_gettimeofday(&tv);
        sdp->origin.version = sdp->origin.id = tv.sec + 2208988800UL;

        sdp->media_count = 1;
        sdp->media[0]->desc.media = pj_str("audio");
        from_addr->ipv4.sin_port = sdp->media[0]->desc.port;
        app.media.trans[tp_num].dest_rtp_port = sdp->media[0]->desc.port;
        sdp->media[0]->desc.port = pj_ntohs(tpinfo.sock_info.rtp_addr_name.ipv4.sin_port);
        sdp->media[0]->desc.port_count = 1;
        sdp->media[0]->desc.transport = pj_str("RTP/AVP");

        length = pjmedia_sdp_print(sdp, buf, sizeof(buf));
        str_buf.ptr = buf;
        str_buf.slen = length;
        body = pjsip_msg_body_create(app.pool, &tdata->msg->body->content_type.type, &tdata->msg->body->content_type.subtype,
                                     &str_buf);
        pjsip_msg_body_copy(app.pool, tdata->msg->body, body);
    }
    return PJ_SUCCESS;
}