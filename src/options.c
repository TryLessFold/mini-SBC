#include <mini_SBC.h>

#define THIS_FILE "options.c"

static void usage(void)
{
    puts("Options:\n"
         "\n"
         " -p, --set-port port<>host(:port)\tSet local listener port to destination host\n"
         " -r, --set-rtp-port\tSet rtp start port(default " DEFAULT_RTP_PORT_TEXT ")\n");
}

pj_status_t init_options(int argc, char *argv[])
{
    struct pj_getopt_option long_opt[] = {
        {"set-port", 1, 0, 'p'},
        {"set_rtp-port", 1, 0, 'r'},
        {"help", 0, 0, 'h'},
        {NULL, 0, 0, 0}};
    int c;
    int opt_ind = 0;

    pj_optind = 0;
    while ((c = pj_getopt_long(argc, argv, "p:hr:", long_opt, &opt_ind)) != -1)
    {
        switch (c)
        {
        case 'p':
        {
            int local_port = 0, port = 0;
            char addr[PJ_INET_ADDRSTRLEN] = {0};

            sscanf(pj_optarg, "%d#%16s:%d", &local_port, addr, &port);
            if ((local_port != 0) && (strlen(addr) != 0))
            {
                app.port[app.hosts_cnt] = local_port;
                pj_strdup2(app.pool, &app.hosts[app.hosts_cnt].host, addr);
                if (port == 0)
                    app.hosts[app.hosts_cnt].port = DEFAULT_PORT;
                else
                    app.hosts[app.hosts_cnt].port = port;
                PJ_LOG(3, (THIS_FILE, "Connection is set: %d - %s:%d",
                           app.port[app.hosts_cnt],
                           app.hosts[app.hosts_cnt].host.ptr,
                           app.hosts[app.hosts_cnt].port));
                app.hosts_cnt++;
            }
            break;
        }
        case 'r':
            if (app.media.rtp_port == 0)
                app.media.rtp_port = atoi(pj_optarg);
            else {
                puts("rtp_port is already defined");
                return -1;
            }
            break;
        case 'h':
            usage();
            return -1;

        default:
            puts("Unknown option. Run with --help for help.");
            return -1;
        }
    }

    return PJ_SUCCESS;
}