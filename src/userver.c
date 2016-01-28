#if 0
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#endif

#include <stdarg.h>

#if 0
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

#include "json.h"
#include "$UPROTO$-server.h"
#include "lvcode.h"
#include "ansi-utils.h"
#include "my-pjlib-utils.h"
static void $UPROTO$_server_init_int($UPROTO$_server_t *userver, char *conn_str, pj_pool_t *pool) {
    userver->pool = pool;
    ansi_copy_str(userver->connect_str, conn_str);
    userver->is_online = 1;
    CHECK(__FILE__, pj_mutex_create_simple(pool, "", &userver->mutex));
    if (userver->on_init_done_f != NULL){
        userver->on_init_done_f(userver);
	}
}

void $UPROTO$_server_init($UPROTO$_server_t *userver, char *conn_str, pj_pool_t *pool) {
    userver->get_pph_f = NULL;
    $UPROTO$_server_init_int(userver, conn_str, pool);
}

void $UPROTO$_server_init_ex($UPROTO$_server_t *userver, char *conn_str, pj_pool_t *pool, char *(*get_pph_f)(pj_str_t *id)) {
    userver->get_pph_f = get_pph_f;
    $UPROTO$_server_init_int(userver, conn_str, pool);
}

static void open_udp_socket($UPROTO$_server_t *userver, char *addr, int port) {
    pj_sockaddr_in saddr;
    pj_str_t s;
    int optval = 1;
    SHOW_LOG(3, __FILE__":open_udp_socket: %s:%d\n", addr, port);   
    // create udp socket here
    CHECK(__FILE__, pj_sock_socket(PJ_AF_INET, PJ_SOCK_DGRAM, 0, &userver->fd));

#ifdef __ICS_INTEL__
    // Allow socket reuse
    //CHECK(__FILE__, pj_sock_setsockopt(userver->fd, PJ_SOL_SOCKET, 512, &optval, sizeof(optval)));
    CHECK(__FILE__, pj_sock_setsockopt(userver->fd, PJ_SOL_SOCKET, PJ_SO_REUSEADDR, &optval, sizeof(optval)));
#endif

    // bind the socket
    pj_bzero(&saddr, sizeof(saddr));
    saddr.sin_family = PJ_AF_INET;
    saddr.sin_port = pj_htons(port);
    saddr.sin_addr = pj_inet_addr(pj_cstr(&s,addr));

    /*pj_status_t status = pj_sock_bind(userver->fd, &saddr, sizeof(saddr));
    if( status != 0 ) {
        SHOW_LOG(1, __FILE__":open_udp_socket error(%d)\n", status);
        exit(-1);
    }*/
    CHECK(__FILE__, pj_sock_bind(userver->fd, &saddr, sizeof(saddr)));
}

static void open_tty($UPROTO$_server_t *userver, char *path) {
    (void)userver;
    (void)path;
#if 0
    struct termios options;
    
    userver->fd = open(path, O_RDWR | O_NOCTTY);
    EXIT_IF_TRUE(userver->fd < 0, "Cannot open port");
    fcntl(userver->fd, F_SETFL, 0);

    tcgetattr(userver->fd, &options);

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CREAD |CLOCAL);
    options.c_cflag &= ~CSIZE; /* Mask the character size bits */
    options.c_cflag |= CS8;    /* Select 8 data bits */
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    //options->c_lflag |= (ICANON | ECHO | ECHOE); // Canonical mode

    options.c_lflag &= ~(ICANON | ECHO); // RAW mode
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 2; // measured in 0.1 second

    tcsetattr(userver->fd, TCSANOW, &options);
#endif
}

static int udp_recvfrom(int fd, char *buff, int len, void *data, unsigned int *data_len) {
    int ret;
    long nbytes = len;
    ret = pj_sock_recvfrom(fd, buff, &nbytes, 0, (pj_sockaddr_t *)data, (int *)data_len);
    if( ret != 0 ) {
        PERROR_IF_TRUE(1, "Error receiving data\n");
        return -1;
    }
    return nbytes;
}

static int tty_recvfrom(int fd, char *buff, int len, void *data, unsigned int *data_len) {
    (void)fd, (void) buff, (void) len, (void)data, (void)data_len;
    return -1;
    //return read(fd, buff, len);
}

static int udp_sendto(int fd, char *buff, int len, void *data, unsigned int data_len) {
    int ret = 0;
    long nbytes = len;
    ret = pj_sock_sendto(fd, buff, &nbytes, 0, (pj_sockaddr_t *)data, data_len);
    if( ret != 0 ) {
        PERROR_IF_TRUE(1, "Error in sending data\n");
        return -1;
    }
    return nbytes;
}

static int tty_sendto(int fd, char *buff, int len, void *data, unsigned int data_len) {
    (void)fd, (void) buff, (void) len, (void)data, (void)data_len;
    return -1;
    //return write(fd, buff, *len);
}

#define USERVER_BUFSIZE 512
int $UPROTO$_server_proc(void *param) {
    int ret;
    unsigned int len;
    
    int port;
    char *first, *second, *third;
    char cnt_str[30];

    pj_sockaddr_in caddr;
    char *caddr_str;
    pj_time_val timeout;

    pj_fd_set_t read_fds;

	char buffer[USERVER_BUFSIZE];
	$UPROTO$_request_t request;

    $UPROTO$_server_t *userver = ($UPROTO$_server_t *)param;
	ansi_copy_str(cnt_str, userver->connect_str);

    first = cnt_str;
    
    second = strchr(first, ':');
    EXIT_IF_TRUE(second == NULL, "Wrong connection string format\n");
    *second = '\0';
    second++;

    if(0 == strcmp(first, "udp")) {
        third = strchr(second, ':');
        EXIT_IF_TRUE(third == NULL, "Wrong connection string format\n");
        *third = '\0';
        third++;
        port = atoi(third);
        open_udp_socket(userver, second, port);
        if (userver->on_open_socket_f != NULL)
            userver->on_open_socket_f(userver);

        userver->recv_f = &udp_recvfrom;
        userver->send_f = &udp_sendto;
    }
    else if( 0 == strcmp(first, "tty") ) {
        open_tty(userver, second);
        userver->recv_f = &tty_recvfrom;
        userver->send_f = &tty_sendto;
    }
    else {
        EXIT_IF_TRUE(1, "Unsuported protocol\n");
    }

    // thread loop
    timeout.sec = 0;
    timeout.msec = 100;
    userver->is_end = 0;


    while( !userver->is_end ) {

        while (!userver->is_online) {
            SHOW_LOG(3, "Server is currently offline...\n");
            //usleep(8*1000*1000);
            pj_thread_sleep(1000);
        }

        PJ_FD_ZERO(&read_fds);
        PJ_FD_SET(userver->fd, &read_fds);

        pj_mutex_lock(userver->mutex);
        ret = pj_sock_select(userver->fd + 1, &read_fds, NULL, NULL, &timeout); 
        pj_mutex_unlock(userver->mutex);

        EXIT_IF_TRUE(ret < 0, "Error on server socket\n");

        if( PJ_FD_ISSET(userver->fd, &read_fds) ) {
            len = sizeof(caddr);
            pj_bzero(&caddr, len);

            pj_mutex_lock(userver->mutex);
            ret = userver->recv_f(userver->fd, buffer, USERVER_BUFSIZE, (void *)&caddr, &len);
            pj_mutex_unlock(userver->mutex);
            caddr_str = pj_inet_ntoa(caddr.sin_addr);

            if( ret > 0 ) {
                buffer[ret] = '\0';
                SHOW_LOG(5, "Received from client: %s\n", buffer);
                $UPROTO$_parse_request(buffer, ret, &request);
                userver->on_request_f(userver, &request, caddr_str);
            }
        }
        //usleep(100*1000);
        pj_thread_sleep(100);
        // if userver->fd is ready to write. When write finish, call userver->on_sent();
        // else --> time out
    }
	return 0;
}
int $UPROTO$_secure_server_proc(void *param) {
    int ret;
    unsigned int len;
    
    int port;
    char *first, *second, *third;
    char cnt_str[30];

    pj_sockaddr_in caddr;
    char *caddr_str;
    pj_time_val timeout;

    pj_fd_set_t read_fds;

	char buffer[USERVER_BUFSIZE];

	$UPROTO$_request_t request;

    // For lvc parsing
    lvc_t lvc;
    int len1;
    char *val;
    uint32_t *ts;
    char sts[32];
    char plain[USERVER_BUFSIZE];
    char *pph;
    char otp[32];
    pj_str_t pjstr;
    // End for lvc parsing

    $UPROTO$_server_t *userver = ($UPROTO$_server_t *)param;
	ansi_copy_str(cnt_str, userver->connect_str);

    first = cnt_str;
    
    second = strchr(first, ':');
    EXIT_IF_TRUE(second == NULL, "Wrong connection string format\n");
    *second = '\0';
    second++;

    if(0 == strcmp(first, "udp")) {
        third = strchr(second, ':');
        EXIT_IF_TRUE(third == NULL, "Wrong connection string format\n");
        *third = '\0';
        third++;
        port = atoi(third);
        open_udp_socket(userver, second, port);
        if (userver->on_open_socket_f != NULL)
            userver->on_open_socket_f(userver);

        userver->recv_f = &udp_recvfrom;
        userver->send_f = &udp_sendto;
    }
    else if( 0 == strcmp(first, "tty") ) {
        open_tty(userver, second);
        userver->recv_f = &tty_recvfrom;
        userver->send_f = &tty_sendto;
    }
    else {
        EXIT_IF_TRUE(1, "Unsuported protocol\n");
    }

    // thread loop
    timeout.sec = 0;
    timeout.msec = 100;
    userver->is_end = 0;


    while( !userver->is_end ) {

        while (!userver->is_online) {
            SHOW_LOG(3, "Server is currently offline...\n");
            pj_thread_sleep(1000);
        }

        PJ_FD_ZERO(&read_fds);
        PJ_FD_SET(userver->fd, &read_fds);

        pj_mutex_lock(userver->mutex);
        ret = pj_sock_select(userver->fd + 1, &read_fds, NULL, NULL, &timeout); 
        pj_mutex_unlock(userver->mutex);

        EXIT_IF_TRUE(ret < 0, "Error on server socket\n");

        if( PJ_FD_ISSET(userver->fd, &read_fds) ) {
            len = sizeof(caddr);
            pj_bzero(&caddr, len);

            pj_mutex_lock(userver->mutex);
            ret = userver->recv_f(userver->fd, buffer, USERVER_BUFSIZE, (void *)&caddr, &len);
            pj_mutex_unlock(userver->mutex);
            caddr_str = pj_inet_ntoa(caddr.sin_addr);

            if( ret > 0 ) {
                buffer[ret] = '\0';

                lvc_init(&lvc, buffer, ret);

                lvc_unpack(&lvc, &len, &val);
                pj_strset(&pjstr, val, len);
                pph = userver->get_pph_f(&pjstr);
                if( pph != NULL ) {

                    lvc_unpack(&lvc, &len, &val);
                    ts = (uint32_t *)val;
                    ts2str(*ts, sts);

                    lvc_unpack(&lvc, &len, &val);
                    
                    generate_otp(otp, pph, sts);
                    do_decrypt(val, len, plain, &len1, otp);
                    if( pj_ansi_strncmp(sts, plain, len1) == 0 ) {
                        lvc_unpack(&lvc, &len, &val);
                        do_decrypt(val, len, plain, &len1, otp);
                        plain[len1] = '\0';

                        $UPROTO$_parse_request(plain, len1, &request);
                        userver->on_request_f(userver, &request, caddr_str);
                    }
                }
                //else {
                //}
            }
        }
        pj_thread_sleep(100);
    }
	return 0;
}

void $UPROTO$_server_start($UPROTO$_server_t *userver) {
    pj_thread_create(userver->pool, NULL, $UPROTO$_server_proc, userver, PJ_THREAD_DEFAULT_STACK_SIZE, 0, &userver->master_thread);
}

void $UPROTO$_server_start_ex($UPROTO$_server_t *userver) {
    pj_thread_create(userver->pool, NULL, $UPROTO$_secure_server_proc, userver, PJ_THREAD_DEFAULT_STACK_SIZE, 0, &userver->master_thread);
}

void $UPROTO$_server_join($UPROTO$_server_t *userver, char *multicast_ip) {
    pj_ip_mreq mreq;
    pj_str_t s, s1;
    pj_status_t ret;

    pj_bzero(&mreq, sizeof(pj_ip_mreq));
	mreq.imr_multiaddr = pj_inet_addr(pj_cstr(&s, multicast_ip));
	mreq.imr_interface = pj_inet_addr(pj_cstr(&s1, "0.0.0.0"));

    //mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    //mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    pj_mutex_lock(userver->mutex);
    ret = pj_sock_setsockopt(userver->fd, PJ_SOL_IP, PJ_IP_ADD_MEMBERSHIP, &mreq,sizeof(mreq));
    PERROR_IF_TRUE(ret != 0, "Error in joining mcast group");
    pj_mutex_unlock(userver->mutex);
}

void $UPROTO$_server_leave($UPROTO$_server_t *userver, char *multicast_ip) {
    pj_ip_mreq mreq;
    pj_str_t s;
    pj_status_t ret;
    mreq.imr_multiaddr = pj_inet_addr(pj_cstr(&s, multicast_ip));
    mreq.imr_interface.s_addr = pj_htonl(PJ_INADDR_ANY);

    pj_mutex_lock(userver->mutex);
    ret = pj_sock_setsockopt(userver->fd, PJ_SOL_IP, PJ_IP_DROP_MEMBERSHIP, &mreq,sizeof(mreq));
    PERROR_IF_TRUE(ret != 0, "Error in leaving mcast group");
    pj_mutex_unlock(userver->mutex);
}

void $UPROTO$_server_end($UPROTO$_server_t *userver) {
    userver->is_end = 1;
    CHECK(__FILE__, pj_thread_join(userver->master_thread));
}
