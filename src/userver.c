#include "userver.h"
#include "proto.h"
#include "ansi-utils.h"
#include <sys/select.h>

static void on_sent(userver_t *userver, char *buff, int len) {
	// ...
}
static void on_received(userver_t *userver, char *buff, int len) {
	// ...
}
/*
static void parse_request(char *buff, int len, void *request) {
}

static void build_response(char *buff, int len, va_list argp) {
}
*/
void userver_init(userver_t *userver, int port) {
	userver->port = port;
	//userver->parse_request_f = &parse_request;
	//userver->build_response_f = &build_response;
}

#define USERVER_BUFSIZE 512

void *userver_proc(void *param) {
	int ret, len;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
	struct timeval timeout;

	fd_set read_fds, write_fds;

	char buffer[USERVER_BUFSIZE];
	uproto_request_t request;
	uproto_response_t response;

	userver_t *userver = (userver_t *)param;

	// create udp socket here
	userver->socket = socket(AF_INET, SOCK_DGRAM, 0);
	EXIT_IF_TRUE(userver->socket <= 0, "Cannot create socket\n");
	
	// bind the socket to port userver->port
	memset(&saddr, '\0', sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(userver->port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(userver->socket, (struct sockaddr *)&saddr, sizeof(saddr));
	EXIT_IF_TRUE(ret < 0, "Cannot bind socket to port\n");

	// thread loop
	userver->is_end = 0;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100 * 1000;
	while( !userver->is_end ) {
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		FD_SET(userver->socket, &read_fds);
		FD_SET(userver->socket, &write_fds);
		ret = select(userver->socket + 1, &read_fds, &write_fds, NULL, &timeout); 
		EXIT_IF_TRUE(ret < 0, "Error on server socket\n");

		if( FD_ISSET(userver->socket, &read_fds) ) {
			// read goes here
			len = sizeof(caddr);
			memset(&caddr, '\0', len);
			ret = recvfrom(userver->socket, buffer, USERVER_BUFSIZE, 0, (struct sockaddr *)&caddr, &len);
			if( ret > 0 ) {
				buffer[ret] = '\0';
				printf("Received from client: %s\n", buffer);
				userver->parse_request_f(buffer, ret, &request);
			}
			// processing request here
			userver->on_request(userver, &request, &response);
			
			len = userver->build_response_f(buffer, sizeof(buffer), &response);

			sendto(userver->socket, buffer, len, 0, (struct sockaddr *)&caddr, sizeof(struct sockaddr_in));
		}
		// if userver->socket is ready to write. When write finish, call userver->on_sent();
	#if 0	
		if( FD_ISSET(userver->socket, &write_fds) ) {
			
		}
	#endif
		// else --> time out
	}
	return NULL;
}

void userver_start(userver_t *userver) {
	pthread_create(&userver->master_thread, NULL, userver_proc, userver);
}

void userver_end(userver_t *userver) {
	userver->is_end = 1;
	pthread_join(userver->master_thread, NULL);
}
