#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "json.h"
#include "$UPROTO$-client.h"
#include "ansi-utils.h"

#define UCLIENT_BUFSIZE 512

static void open_udp_socket($UPROTO$_client_t *uclient, char *server, int port) {
    int ret;
    struct sockaddr_in *saddr;
    // open socket
    uclient->fd = socket(AF_INET, SOCK_DGRAM, 0);
    EXIT_IF_TRUE(uclient->fd <= 0, "Error creating socket\n");

    uclient->connect_data = malloc(sizeof(struct sockaddr_in));
    saddr = (struct sockaddr_in *)uclient->connect_data;
    memset((void *)saddr, '\0', sizeof(struct sockaddr_in));
    saddr->sin_family = AF_INET;
    saddr->sin_port = htons(port);
    ret = inet_aton(server, &saddr->sin_addr);

    EXIT_IF_TRUE(ret == 0, "Invalid server ip address\n");
}

static void open_tty($UPROTO$_client_t *uclient, char *path) {
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
	char buff[UCLIENT_BUFSIZE];
	
	$UPROTO$_build_request(buff, sizeof(buff), request);
	
	ret = sendto(uclient->fd, buff, strlen(buff), 0, (struct sockaddr *)uclient->connect_data, sizeof(struct sockaddr_in));
	
	return ret;
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
    close(uclient->fd);
    free(uclient->connect_data);
}

