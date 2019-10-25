#include <mini_SBC.h>

#define THIS_FILE "create_transport.c"

pj_status_t SBC_create_transport(pjmedia_transport **trans_port, 
                                 pj_uint16_t *rtp_port)
{
    if ((rtp_port == NULL)||(*rtp_port == 0))
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