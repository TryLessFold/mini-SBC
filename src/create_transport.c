#include <mini_SBC.h>

#define THIS_FILE "create_transport.c"

pj_status_t SBC_create_transport(pjmedia_transport **trans_port,
                                 pj_uint16_t *rtp_port)
{
    if ((rtp_port == NULL) || (*rtp_port == 0))
        return -1;

    /* Repeat binding media socket to next port when fails to bind
     * to current port number.
	 */
    int retry;
    int status = -1;

    for (retry = 0; retry < 100; ++retry, *rtp_port += 2)
    {
        status = pjmedia_transport_udp_create2(app.media.endpt,
                                               "siprtp",
                                               &app.local_addr,
                                               *rtp_port, 0,
                                               trans_port);
        if (status == PJ_SUCCESS)
        {
            PJ_LOG(3, (THIS_FILE, "Media transport has been created on port %d", *rtp_port));
            *rtp_port += 2;
            return PJ_SUCCESS;
        }
    }
    return -1;
}

static inline pj_status_t inc(short num_to, short num_from, pj_uint16_t *rtp_port)
{
    pj_status_t status;
    if ((!app.media.trans[num_to].port) && (!app.media.trans[num_from].port))
    {
        status = SBC_create_transport(&app.media.trans[num_to].port, rtp_port);
        if (status != PJ_SUCCESS)
        {
            app_perror(THIS_FILE, "Media transport couldn't be created (port in use?)", status);
            return -1;
        }
        app.media.trans[num_to].used = 1;
        status = SBC_create_transport(&app.media.trans[num_from].port, rtp_port);
        if (status != PJ_SUCCESS)
        {
            app_perror(THIS_FILE, "Media transport couldn't be created (port in use?)", status);
            status = pjmedia_transport_close(app.media.trans[num_to].port);
            if (status == PJ_SUCCESS)
            {
                PJ_LOG(3, (THIS_FILE, "Media transport(%d) has been closed successfully", num_to));
                app.media.trans[num_to].port = 0;
                app.media.trans[num_to].used = 0;
            }
            else
            {
                app_perror(THIS_FILE, "Media transport(%d) hasn't been closed", num_to);
            }
            return -1;
        }
        app.media.trans[num_from].used = 1;
    }
    else
    {
        app.media.trans[num_to].used++;
        app.media.trans[num_from].used++;
    }
    return PJ_SUCCESS;
}

static inline pj_status_t dec(short num_to, short num_from)
{
    pj_status_t status;
    if ((app.media.trans[num_to].port != NULL) && (app.media.trans[num_to].used == 1))
    {
        status = pjmedia_transport_close(app.media.trans[num_to].port);
        if (status == PJ_SUCCESS)
        {
            PJ_LOG(3, (THIS_FILE, "Media transport(%d) has been closed successfully", num_to));
            app.media.trans[num_to].port = NULL;
            app.media.trans[num_to].used = 0;
        }
        else
        {
            app_perror(THIS_FILE, "Media transport(%d) hasn't been closed", num_to);
        }
    }
    else
    {
        app.media.trans[num_to].used--;
    }
    if ((app.media.trans[num_from].port != NULL) && (app.media.trans[num_from].used == 1))
    {
        status = pjmedia_transport_close(app.media.trans[num_from].port);
        if (status == PJ_SUCCESS)
        {
            PJ_LOG(3, (THIS_FILE, "Media transport(%d) has been closed successfully", num_from));
            app.media.trans[num_from].port = NULL;
            app.media.trans[num_from].used = 0;
        }
        else
        {
            app_perror(THIS_FILE, "Media transport(%d) hasn't been closed", num_to);
        }
    }
    else
    {
        app.media.trans[num_from].used--;
    }
    return PJ_SUCCESS;
}

pj_status_t SBC_calculate_transports(pjsip_rx_data *rdata,
                                     short num_to,
                                     short num_from)
{
    pj_status_t status;
    pj_uint16_t rtp_port;

    pjsip_via_hdr *hdr_via;

    hdr_via = (pjsip_via_hdr *)pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_VIA, NULL);
    if ((!pj_strcmp(&hdr_via->sent_by.host, &app.local_addr)) && (hdr_via->sent_by.port == app.port[num_from]))
    {
        if (rx_get_status(rdata) >= PJSIP_SC_BAD_REQUEST)
        {
            dec(num_to, num_from);
        }
        return PJ_SUCCESS;
    }

    /* Create and destroy media transport */
    rtp_port = (pj_uint16_t)(app.media.rtp_port & 0xFFFE);
    switch (rx_get_method(rdata)->id)
    {
    case PJSIP_INVITE_METHOD:
        status = inc(num_to, num_from, &rtp_port);
        if (status != PJ_SUCCESS)
            return -1;
        break;
    case PJSIP_CANCEL_METHOD:
    case PJSIP_BYE_METHOD:
        status = dec(num_to, num_from);
        if (status != PJ_SUCCESS)
            return -1;
        break;
    default:
        break;
    }
    return PJ_SUCCESS;
}