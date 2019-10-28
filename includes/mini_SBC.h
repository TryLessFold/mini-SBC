#ifndef __MINI_SBC__
#define __MINI_SBC__

#include <pjsip.h>
#include <pjsip.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjmedia.h>

#include <rx_op.h>
#include <px_SBC.h>

#define TRANSPORT_TYPE PJSIP_TRANSPORT_UDP
#define MAX_PORTS 16
#define DEFAULT_PORT 5060
#define DEFAULT_PORT_TEXT "5060"
#define DEFAULT_RTP_PORT 10000
#define DEFAULT_RTP_PORT_TEXT "10000"

#define UNDEFINED_PORT -1

typedef struct my_config
{
    pj_str_t local_addr;
    unsigned short port[MAX_PORTS];

    pjsip_endpoint *sip_endpt;
    pjsip_transport *trans_port[MAX_PORTS];
    pj_caching_pool cp;
    pj_pool_t *pool;
    pjsip_tpmgr *tpmgr;

    pj_thread_t *thread;

    pjsip_dialog *dlg[2];
    unsigned hosts_cnt;
    pjsip_host_port hosts[MAX_PORTS];
    
    struct
    {
        pjmedia_endpt *endpt;
        struct{
            pjmedia_transport *port;
            int used;
        } trans[MAX_PORTS];
        int rtp_port;

    } media;
} my_config_t;

extern my_config_t app;

/* Output error */
void app_perror(const char *sender,
                const char *title,
                pj_status_t status);

pj_status_t init_options(int argc,
                         char *argv[]);

pj_status_t init_SBC();

/* Determines if Uri is local fo this host */
pj_bool_t is_uri_local(const pjsip_sip_uri *uri);

/* Changes Request-Uri in tdata for redirect, headers for hide info
 */
pj_status_t SBC_request_redirect(pjsip_tx_data *tdata,
                                 pjsip_host_port dest_host,
                                 pjsip_host_port local_host);

/* Changes  in tdatafor redirect 
 * Fills dst_host field in res_addr
 */
pj_status_t SBC_response_redirect(pjsip_tx_data *tdata,
                                  pjsip_host_port dest_host,
                                  pjsip_host_port local_host,
                                  pjsip_response_addr *res_addr);

/* Changes Via header for hide local server */
pj_status_t SBC_via_hdr_hide_host(pjsip_via_hdr *via,
                              pj_str_t host,
                              int port);

/* Changes To header for redirect request/response */
pj_status_t SBC_to_hdr_redirect(pjsip_to_hdr *to,
                            pj_str_t host,
                            int port);

/* Changes From header for hide local server*/
pj_status_t SBC_from_hdr_hide_host(pjsip_from_hdr *from,
                               pj_str_t host,
                               int port);

/* Changes Contact header for hide local server*/
pj_status_t SBC_contact_hdr_hide_host(pjsip_contact_hdr *cont,
                                  pj_str_t host,
                                  int port);

/* Looks for a free port */
pj_status_t SBC_create_transport(pjmedia_transport **trans_port,
                                 pj_uint16_t *rtp_port);

/* Handler for RTP transports: creates, deletes
 * transports, inc or dec transport counter.
 */
pj_status_t SBC_calculate_transports(pjsip_rx_data *rdata,
                                 short num_to,
                                 short num_from);

pj_status_t SBC_tx_redirect_sdp(pjsip_tx_data *tdata,
                                int tp_num);

#endif //__MINI_SBC__