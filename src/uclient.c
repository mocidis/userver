#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#if 0
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#endif

#include "pjlib.h"

#include "json.h"
#include "$UPROTO$-client.h"
#include "my-pjlib-utils.h"
#include "ansi-utils.h"

#define UCLIENT_BUFSIZE 512

static void open_udp_socket($UPROTO$_client_t *uclient, char *server, int port) {
    pj_sockaddr_in *saddr;
    pj_str_t s;
    // open socket
    CHECK(__FILE__, pj_sock_socket(PJ_AF_INET, PJ_SOCK_DGRAM, 0, &uclient->fd));

    uclient->connect_data = malloc(sizeof(pj_sockaddr_in));
    saddr = (pj_sockaddr_in *)uclient->connect_data;
    pj_bzero((void *)saddr, sizeof(pj_sockaddr_in));
    saddr->sin_family = PJ_AF_INET;
    saddr->sin_port = pj_htons(port);
    saddr->sin_addr = pj_inet_addr(pj_cstr(&s, server));
}

static void open_tty($UPROTO$_client_t *uclient, char *path) {
    (void) uclient;
    (void) path;
}

void $UPROTO$_client_open($UPROTO$_client_t *uclient, char *conn_str) {
    char *first, *second, *third;
    int len, port;

    memset(uclient->connect_str, 0, sizeof(uclient->connect_str));
    strncpy(uclient->connect_str, conn_str, strlen(conn_str));
    len = strlen(uclient->connect_str);
    uclient->connect_str[len] = '\0';

    first = conn_str;
    second = strchr(first, ':');
    EXIT_IF_TRUE(second == NULL, "Invalid connection string\n");
    *second = '\0';
    second++;
    if( 0 == strcmp(first, "udp") ) {
        third = strchr(second, ':');
        EXIT_IF_TRUE(third == NULL, "Invalid connection string for udp socket\n");
        *third = '\0';
        third++;
        port =  atoi(third);
        open_udp_socket(uclient, second, port);
    }
    else if( 0 == strcmp(first, "tty") ) {
        open_tty(uclient, second);
    }

}

int $UPROTO$_client_send($UPROTO$_client_t *uclient, $UPROTO$_request_t *request) {
	int ret;
    long nbytes;
	char buff[UCLIENT_BUFSIZE];
	
	$UPROTO$_build_request(buff, sizeof(buff), request);
	
    nbytes = strlen(buff);
	ret = pj_sock_sendto(uclient->fd, buff, &nbytes, 0, (const pj_sockaddr_t *)uclient->connect_data, sizeof(pj_sockaddr_in));

    if(ret != 0) {
        PERROR_IF_TRUE(1, "Error in sending data\n");
        return -1;
    }

	return nbytes;
}
/*
int $UPROTO$_client_recv($UPROTO$_client_t *uclient, $UPROTO$_response_t *resp) {
    int n;
    unsigned int len;
    char buffer[UCLIENT_BUFSIZE];
    struct sockaddr_in from_addr;

    len = sizeof(from_addr);
    memset(&from_addr, '\0', len);
    n = recvfrom(uclient->fd, buffer, UCLIENT_BUFSIZE, 0, (struct sockaddr *)&from_addr, &len);
    if( n > 0) {
        uclient->parse_response_f(buffer, n, resp);
    }

    return n;
}
*/
void $UPROTO$_client_close($UPROTO$_client_t *uclient){
    pj_sock_close(uclient->fd);
    free(uclient->connect_data);
}

