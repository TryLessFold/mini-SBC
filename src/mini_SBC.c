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
    pjsip_to_hdr *hdr_to;
    pjsip_from_hdr *hdr_from;
    pjsip_via_hdr *hdr_via;

    target = (pjsip_sip_uri *)tdata->msg->line.req.uri;

    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_VIA, NULL);
    hdr_to = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_TO, NULL);
    to_hdr_redirect(hdr_to, pj_str(FIRST_ADDR_FROMTO));
    hdr_from = (pjsip_fromto_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_FROM, NULL);
    from_hdr_hide_host(hdr_from, pj_str(FIRST_ADDR_FROMTO));
    //    hroute = (pjsip_route_hdr *)pjsip_msg_find_hdr(tdata->msg, PJSIP_H_ROUTE, NULL);
    if (target->port == app.ports[0])
    {
        char *dest_host = SECOND_ADDR_FROMTO;
        target->host = pj_str(dest_host);
        target->port = 0;
    }
    else if (target->port == app.ports[1])
    {
        char *dest_host = FIRST_ADDR_FROMTO;
        target->host = pj_str(dest_host);
        target->port = 0;
    }
    else
    {
        return 1;
    }
}

pj_status_t via_hdr_hide_host(pjsip_via_hdr *via, pj_str_t host, int port)
{
    pj_memcpy((void *)&via->sent_by.host, (void *)&host,
              sizeof(via->sent_by.host));
    via->sent_by.port = port;
    via->recvd_param.ptr = "";
    via->recvd_param.slen = 0;
}

pj_status_t to_hdr_redirect(pjsip_to_hdr *to, pj_str_t host)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *) pjsip_uri_get_uri(to->uri);
    uri->host = pj_str(host.ptr);
}

pj_status_t from_hdr_hide_host(pjsip_from_hdr *from, pj_str_t host)
{
    pjsip_sip_uri *uri = (pjsip_sip_uri *) pjsip_uri_get_uri(from->uri);
    uri->host = pj_str(host.ptr);
}

static void usage(void)
{
    puts("Options:\n"
         "\n"
         " -s, --set-listener port<>host(:port)\tSet local listener port to destination host\n");
}

static pj_status_t init_options(int argc, char *argv[])
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
                printf(pj_optarg);
                sscanf(pj_optarg, "%d<>%16s:%d", &local_port, &addr, &port);
                printf("Port is set to");
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