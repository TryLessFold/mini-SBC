#ifndef __RX_GET_PORT__
#define __RX_GET_PORT__

#include <mini_SBC.h>

/* Reverses and returns port from pjsip_rx_data structure
 */
short rx_get_port(pjsip_rx_data *rdata);

#endif //__RX_GET_PORT__