#ifndef __RX_OP__
#define __RX_OP__

#include <mini_SBC.h>

/* Reverses and returns port from pjsip_rx_data structure
 */
short rx_get_port(pjsip_rx_data *rdata);

pjsip_method *rx_get_method(pjsip_rx_data *rdata);

#endif //__RX_GET_PORT__