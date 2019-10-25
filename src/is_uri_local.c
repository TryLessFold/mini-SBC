#include <mini_SBC.h>

#define THIS_FILE "is_uri_local.c"

pj_bool_t is_uri_local(const pjsip_sip_uri *uri)
{
    unsigned i;
    for (i = 0; i < app.hosts_cnt; ++i)
    {
        if ((uri->port == app.hosts[i].port) &&
            pj_stricmp(&uri->host, &app.hosts[i].host) == 0)
        {
            /* Match */
            return PJ_TRUE;
        }
    }

    /* Doesn't match */
    return PJ_FALSE;
}