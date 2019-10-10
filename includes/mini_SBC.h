#ifndef __MINI_SBC__
#define __MINI_SBC__

#include <pjsip.h>
#include <pjsip.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjmedia.h>

#include <rx_get_port.h>

#define FIRST_PORT 8000
#define FIRST_PORT_TEXT ":8000"
#define FIRST_AUDIO_PORT 8010
#define FIRST_AUDIO_PORT_TEXT ":8010"

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
} my_config_t;

extern my_config_t app;

#endif //__MINI_SBC__