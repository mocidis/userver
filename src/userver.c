#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>

#include "json.h"
#include "$UPROTO$-server.h"
#include "ansi-utils.h"

void $UPROTO$_server_init($UPROTO$_server_t *userver, char *conn_str) {
    int ret;
    userver->connect_str = conn_str;
    ret = pthread_mutex_init(&userver->mutex, NULL);
    EXIT_IF_TRUE(ret != 0, "Error creating mutex\n");
}

static void open_udp_socket($UPROTO$_server_t *userver, char *addr, int port) {
    struct sockaddr_in saddr;
    int ret;
    
    // create udp socket here
    userver->fd = socket(AF_INET, SOCK_DGRAM, 0);
    EXIT_IF_TRUE(userver->fd <= 0, "Cannot create socket\n");
    
    // bind the socket
    memset(&saddr, '\0', sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    ret = inet_aton(addr, &saddr.sin_addr);
    EXIT_IF_TRUE(ret==0, "Invalid ip address\n");

    ret = bind(userver->fd, (struct sockaddr *)&saddr, sizeof(saddr));
    EXIT_IF_TRUE(ret < 0, "Cannot bind socket to port\n");
}

static void open_tty($UPROTO$_server_t *userver, char *path) {
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
}

static int udp_recvfrom(int fd, char *buff, int len, void *data, unsigned int *data_len) {
    return recvfrom(fd, buff, len, 0, (struct sockaddr *)data, data_len);
}

static int tty_recvfrom(int fd, char *buff, int len, void *data, unsigned int *data_len) {
    return read(fd, buff, len);
}

static int udp_sendto(int fd, char *buff, int len, void *data, unsigned int data_len) {
    return sendto(fd, buff, len, 0, (struct sockaddr *)data, data_len);
}

static int tty_sendto(int fd, char *buff, int len, void *data, unsigned int data_len) {
    return write(fd, buff, len);
}

#define USERVER_BUFSIZE 512
void *$UPROTO$_server_proc(void *param) {
    int ret;
    unsigned int len;
    
    int port;
    char *first, *second, *third;

    struct sockaddr_in caddr;
    struct timeval timeout;

    fd_set read_fds;

	char buffer[USERVER_BUFSIZE];
	$UPROTO$_request_t request;

    $UPROTO$_server_t *userver = ($UPROTO$_server_t *)param;

    first = userver->connect_str;
    
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
    timeout.tv_sec = 0;
    timeout.tv_usec = 100 * 1000;
	userver->is_end = 0;
	while( !userver->is_end ) {
		FD_ZERO(&read_fds);
		FD_SET(userver->fd, &read_fds);

        pthread_mutex_lock(&userver->mutex);
		ret = select(userver->fd + 1, &read_fds, NULL, NULL, &timeout); 
        pthread_mutex_unlock(&userver->mutex);

		EXIT_IF_TRUE(ret < 0, "Error on server socket\n");

		if( FD_ISSET(userver->fd, &read_fds) ) {
			len = sizeof(caddr);
			memset(&caddr, '\0', len);

            pthread_mutex_lock(&userver->mutex);
			ret = userver->recv_f(userver->fd, buffer, USERVER_BUFSIZE, (void *)&caddr, &len);
            pthread_mutex_unlock(&userver->mutex);

			if( ret > 0 ) {
				buffer[ret] = '\0';
				printf("Received from client: %s\n", buffer);
				parse_request(buffer, ret, &request);
                userver->on_request_f(userver, &request);
			}
		}
		// if userver->fd is ready to write. When write finish, call userver->on_sent();
		// else --> time out
	}
	return NULL;
}

void $UPROTO$_server_start($UPROTO$_server_t *userver) {
    pthread_create(&userver->master_thread, NULL, $UPROTO$_server_proc, userver);
}

void $UPROTO$_server_join($UPROTO$_server_t *userver, char *multicast_ip) {
    struct ip_mreq mreq;
    int ret;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    pthread_mutex_lock(&userver->mutex);
    ret = setsockopt(userver->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,sizeof(mreq));
    EXIT_IF_TRUE(ret < 0, "Error in joining multicast group\n");
    pthread_mutex_unlock(&userver->mutex);
}

void $UPROTO$_server_leave($UPROTO$_server_t *userver, char *multicast_ip) {
    struct ip_mreq mreq;
    int ret;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    pthread_mutex_lock(&userver->mutex);
    ret = setsockopt(userver->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,sizeof(mreq));
    EXIT_IF_TRUE(ret < 0, "Error in joining multicast group\n");
    pthread_mutex_unlock(&userver->mutex);
}

void $UPROTO$_server_end($UPROTO$_server_t *userver) {
    userver->is_end = 1;
    pthread_join(userver->master_thread, NULL);
}
