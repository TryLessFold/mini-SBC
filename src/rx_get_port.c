#include <rx_op.h>

short rx_get_port(pjsip_rx_data *rdata)
{
    return pj_ntohs(rdata->tp_info.transport->local_addr.ipv4.sin_port);
}