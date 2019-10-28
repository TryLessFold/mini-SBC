#include <rx_op.h>

pjsip_method* rx_get_method(pjsip_rx_data *rdata)
{
    return &rdata->msg_info.msg->line.req.method;
}

short rx_get_port(pjsip_rx_data *rdata)
{
    return pj_ntohs(rdata->tp_info.transport->local_addr.ipv4.sin_port);
}

pjsip_status_code rx_get_status(pjsip_rx_data *rdata)
{
    return rdata->msg_info.msg->line.status.code;
}