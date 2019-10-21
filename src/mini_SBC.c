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

pj_bool_t is_uri_local(const pjsip_sip_uri *uri)
{
    unsigned i;
    for (i = 0; i < app.hosts_cnt; ++i)
    {
        if ((uri->port == app.hosts[i].port) &&
            pj_stricmp(&uri->host, &app.hosts[i].host) == 0)
        {
            /* Match */
            return PJ_TRUE;
        }
    }

    /* Doesn't match */
    return PJ_FALSE;
}

pj_status_t SBC_request_redirect(pjsip_tx_data *tdata, int *num_host)
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
    if (target->port == app.port[0])
    {
        char *dest_host = app.hosts[1].host.ptr;
        to_hdr_redirect(hdr_to, app.hosts[1].host, app.hosts[1].port);
        from_hdr_hide_host(hdr_from, app.local_host, app.port[1]);
        via_hdr_hide_host(hdr_via, app.local_host, app.port[1]);
        contact_hdr_hide_host(hdr_cont, app.local_host, app.port[1]);
        target->host = pj_str(dest_host);
        target->port = 0;
        *num_host = 1;
    }
    else if (target->port == app.port[1])
    {
        char *dest_host = app.hosts[0].host.ptr;
        to_hdr_redirect(hdr_to, app.hosts[0].host, app.hosts[1].port);
        from_hdr_hide_host(hdr_from, app.local_host, app.port[0]);
        via_hdr_hide_host(hdr_via, app.local_host, app.port[0]);
        contact_hdr_hide_host(hdr_cont, app.local_host, app.port[0]);
        target->host = pj_str(dest_host);
        target->port = 0;
        *num_host = 0;
    }
    else
    {
        return 1;
    }
}

pj_status_t SBC_response_redirect(pjsip_tx_data *tdata, pjsip_response_addr *res_addr)
{
    pjsip_to_hdr *hdr_to;
    pjsip_from_hdr *hdr_from;
    pjsip_via_hdr *hdr_via;
    pjsip_contact_hdr *hdr_cont;

    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    hdr_to = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_TO, NULL);
    hdr_from = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_FROM, NULL);
    hdr_cont = (pjsip_contact_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_CONTACT, NULL);
    to_hdr_redirect(hdr_to, app.local_host, app.port[1]);
    if (hdr_via->rport_param == app.port[0])
    {
        from_hdr_hide_host(hdr_from, app.hosts[1].host, app.hosts[1].port);
        via_hdr_hide_host(hdr_via, app.hosts[1].host, app.hosts[1].port);
        if (hdr_cont!=NULL)
            contact_hdr_hide_host(hdr_cont, app.hosts[1].host, app.hosts[1].port);
    }
    else if (hdr_via->rport_param == app.port[1])
    {
        from_hdr_hide_host(hdr_from, app.hosts[0].host, app.hosts[0].port);
        via_hdr_hide_host(hdr_via, app.hosts[0].host, app.hosts[0].port);
        if (hdr_cont!=NULL)
            contact_hdr_hide_host(hdr_cont, app.hosts[0].host, app.hosts[0].port);
    }
    else
    {
        return 1;
    }
}

pj_status_t via_hdr_hide_host(pjsip_via_hdr *via, pj_str_t host, int port)
{
    via->sent_by.host.ptr = host.ptr;
    via->sent_by.host.slen = host.slen;
    via->sent_by.port = port;
    via->recvd_param.ptr = "";
    via->recvd_param.slen = 0;
    via->rport_param = 0;
}

pj_status_t to_hdr_redirect(pjsip_to_hdr *to, pj_str_t host, int port)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *) pjsip_uri_get_uri(to->uri);
    uri->host = pj_str(host.ptr);
}

pj_status_t from_hdr_hide_host(pjsip_from_hdr *from, pj_str_t host, int port)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *) pjsip_uri_get_uri(from->uri);
    uri->host.ptr = host.ptr;
    uri->host.slen = host.slen;
}

pj_status_t contact_hdr_hide_host(pjsip_contact_hdr *cont, pj_str_t host, int port)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *) pjsip_uri_get_uri(cont->uri);
    uri->host.ptr = host.ptr;
    uri->host.slen = host.slen;
    uri->port = port;
}

static void usage(void)
{
    puts("Options:\n"
         "\n"
         " -s, --set-listener port<>host(:port)\tSet local listener port to destination host\n");
}

pj_status_t init_options(int argc, char *argv[])
{
    struct pj_getopt_option long_opt[] = {
        {"set-port", 1, 0, 'p'},
        {"help", 0, 0, 'h'},
        {NULL, 0, 0, 0}};
    int c;
    int opt_ind;

    pj_optind = 0;
    while ((c = pj_getopt_long(argc, argv, "p:h", long_opt, &opt_ind)) != -1)
    {
        switch (c)
        {
        case 'p':
            {
                int local_port, port;
                char addr[PJ_INET_ADDRSTRLEN];
                
                sscanf(pj_optarg, "%d#%16s:%d", &local_port, addr, &port);
                if ((local_port != 0)&&(strlen(addr)!=0))
                {
                    app.port[app.hosts_cnt] = local_port;
                    pj_strdup2(app.pool, &app.hosts[app.hosts_cnt].host, addr);
                    if (port == 0)
                        app.hosts[app.hosts_cnt].port = DEFAULT_PORT;
                    else
                        app.hosts[app.hosts_cnt].port = port;
                    PJ_LOG(3, (THIS_FILE, "Connection is set: %d - %s:%d", 
                                        app.port[app.hosts_cnt],
                                        app.hosts[app.hosts_cnt].host.ptr,
                                        app.hosts[app.hosts_cnt].port));
                    app.hosts_cnt++;
                }
                break;
            }
        case 'h':
            usage();
            return -1;

        default:
            puts("Unknown option. Run with --help for help.");
            return -1;
        }
    }

    return PJ_SUCCESS;
}

pj_status_t init_SBC()
{
    pj_sockaddr my_addr;
    if (app.hosts_cnt < 2)
    {
        app_perror(THIS_FILE, "Needs more ports (minimum 2)", -1);
        return -1;
    }

    char addr[PJ_INET_ADDRSTRLEN];
    pj_sockaddr tmp;
    pj_getdefaultipinterface(pj_AF_INET(), &tmp);
    pj_inet_ntop(pj_AF_INET(), &tmp.ipv4.sin_addr,
                 addr, sizeof(addr));
    pj_strdup2(app.pool, &app.local_host, addr);
    return PJ_SUCCESS;
}