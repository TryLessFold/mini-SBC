#ifndef __MINI_SBC__
#define __MINI_SBC__

#include <pjsip.h>
#include <pjsip.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjmedia.h>

#include <rx_get_port.h>
#include <px_SBC.h>

#define FIRST_ADDR_FROMTO "192.168.23.134"
#define FIRST_PORT 8000
#define FIRST_PORT_TEXT ":8000"
#define FIRST_AUDIO_PORT 8010
#define FIRST_AUDIO_PORT_TEXT ":8010"

#define SECOND_ADDR_FROMTO "192.168.23.134"
#define SECOND_PORT 9000
#define SECOND_PORT_TEXT ":9000"
#define SECOND_AUDIO_PORT 9010
#define SECOND_AUDIO_PORT_TEXT ":9010"

typedef struct my_config
{
    pjsip_endpoint *sip_endpt;
    pjmedia_endpt *media_endpt;
    pjsip_transport *trans_port[2];
    pjmedia_transport *mtrans_port[2];
    pjsip_dialog *dlg[2];
    pj_caching_pool cp;
    pj_thread_t *thread;
    pj_pool_t *pool;
    pjsip_tpmgr *tpmgr;
    unsigned networks_cnt;
    pjsip_host_port networks[16];
    unsigned short ports[2];
} my_config_t;

typedef int sbc_status_t;

/* Output error */
void app_perror(const char *sender,
                       const char *title,
                       pj_status_t status);

/* Determines if Uri is local fo this host */
pj_bool_t is_uri_local(const pjsip_sip_uri *uri);

pj_status_t SBC_redirect(pjsip_tx_data *tdata);

extern my_config_t app;

#endif //__MINI_SBC__