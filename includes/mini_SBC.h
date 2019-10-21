#ifndef __MINI_SBC__
#define __MINI_SBC__

#include <pjsip.h>
#include <pjsip.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjmedia.h>

#include <rx_get_port.h>
#include <px_SBC.h>

#define MAX_PORTS 16
#define DEFAULT_PORT 5060
#define UNDEFINED_PORT -1

typedef struct my_config
{
    pj_str_t local_host;
    unsigned short port[MAX_PORTS];
    
    pjsip_endpoint *sip_endpt;
    pjmedia_endpt *media_endpt;
    pjsip_transport *trans_port[2];
    pjmedia_transport *mtrans_port[2];
    pj_caching_pool cp;
    pj_pool_t *pool;
    pjsip_tpmgr *tpmgr;
   
    pj_thread_t *thread;
    
    pjsip_dialog *dlg[2];
    unsigned hosts_cnt;
    pjsip_host_port hosts[MAX_PORTS];
} my_config_t;

extern my_config_t app;

/* Output error */
void app_perror(const char *sender,
                       const char *title,
                       pj_status_t status);

pj_status_t init_options(int argc, char *argv[]);

pj_status_t init_SBC();

/* Determines if Uri is local fo this host */
pj_bool_t is_uri_local(const pjsip_sip_uri *uri);

/* Changes Request-Uri in tdata for redirect, headers for hide info
 * Fills num_host, which point to needed transport
 */
pj_status_t SBC_request_redirect(pjsip_tx_data *tdata, int *num_host);

/* Changes  in tdatafor redirect 
 * Fills num_host, which point to needed transport
 */
pj_status_t SBC_response_redirect(pjsip_tx_data *tdata, pjsip_response_addr *res_addr);

/* Changes Via header for hide local server */
pj_status_t via_hdr_hide_host(pjsip_via_hdr *via, pj_str_t host, int port);

/* Changes To header for redirect request/response */
pj_status_t to_hdr_redirect(pjsip_to_hdr *to, pj_str_t host, int port);

/* Changes From header for hide local server*/
pj_status_t from_hdr_hide_host(pjsip_from_hdr *from, pj_str_t host, int port);

/* Changes Contact header for hide local server*/
pj_status_t contact_hdr_hide_host(pjsip_contact_hdr *cont, pj_str_t host, int port);

#endif //__MINI_SBC__