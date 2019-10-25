#include <rx_op.h>

pjsip_method* rx_get_method(pjsip_rx_data *rdata)
{
    return &rdata->msg_info.msg->line.req.method;
}