#include <mini_SBC.h>

void app_perror(const char *sender,
                const char *title,
                pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));
    PJ_LOG(1, (sender, "%s: %s [code=%d]", title, errmsg, status));
}

pj_bool_t is_uri_local(const pjsip_sip_uri *uri)
{
    unsigned i;
    for (i = 0; i < app.networks_cnt; ++i)
    {
        if ((uri->port == app.networks[i].port) &&
            pj_stricmp(&uri->host, &app.networks[i].host) == 0)
        {
            /* Match */
            return PJ_TRUE;
        }
    }

    /* Doesn't match */
    return PJ_FALSE;
}

sbc_status_t SBC_redirect(pjsip_tx_data *tdata)
{
    pjsip_sip_uri *target;
    pjsip_route_hdr *hroute;

    target = (pjsip_sip_uri *)tdata->msg->line.req.uri;
    hroute = (pjsip_route_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_ROUTE, NULL);
    pjsip_msg_find_remove_hdr(tdata->msg,
                             PJSIP_H_VIA,
                             NULL);
    if (target->port == app.ports[0])
    {
        tdata->tp_info.transport = app.trans_port[1];
        char *dest_host = SECOND_ADDR_FROMTO;
        target->host = pj_str(dest_host);
        target->port = 0;
    }
    else if (target->port == app.ports[1])
    {
        tdata->tp_info.transport = app.trans_port[0];
        char *dest_host = FIRST_ADDR_FROMTO;
        target->host = pj_str(dest_host);
        target->port = 0;
    }
    else
    {
        return 1;
    }
}