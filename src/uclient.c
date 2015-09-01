#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "uclient.h"
#include "ansi-utils.h"

#define UCLIENT_BUFSIZE 512
void uclient_open(uclient_t *uclient, char *server, int port) {
	int ret;
	// open socket
	uclient->socket = socket(AF_INET, SOCK_DGRAM, 0);
	EXIT_IF_TRUE(uclient->socket <= 0, "Error creating socket\n");

	strncpy(uclient->server, server, sizeof(uclient->server));	

	memset((void *)&uclient->server_addr, '\0', sizeof(struct sockaddr_in));
	uclient->server_addr.sin_family = AF_INET;
	uclient->server_addr.sin_port = htons(port);
	ret = inet_aton(uclient->server, &uclient->server_addr.sin_addr);

	EXIT_IF_TRUE(ret == 0, "Invalid server ip address\n");
}

int uclient_send(uclient_t *uclient,...) {
	int n, ret;
	va_list argp;
	char buff[UCLIENT_BUFSIZE];
	va_start(argp, uclient);
	
	n = uclient->build_request_f(buff, sizeof(buff), argp);

	va_end(argp);
	
	ret = sendto(uclient->socket, buff, n, 0, (struct sockaddr *)&uclient->server_addr, sizeof(struct sockaddr_in));
	
	return ret;
}

int uclient_recv(uclient_t *uclient, uproto_response_t *resp) {
	int n;
	int len;
	char buffer[UCLIENT_BUFSIZE];
	struct sockaddr_in from_addr;

	len = sizeof(from_addr);
	memset(&from_addr, '\0', len);
	n = recvfrom(uclient->socket, buffer, UCLIENT_BUFSIZE, 0, (struct sockaddr *)&from_addr, &len);
	if( n > 0) {
		uclient->parse_response_f(buffer, n, resp);
	}

	return n;
}

void ucient_close(uclient_t *uclient){
	close(uclient->socket);
}

