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

static void $UPROTO$_client_open_int($UPROTO$_client_t *uclient, char *conn_str) {
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

void $UPROTO$_client_open($UPROTO$_client_t *uclient, char *conn_str) {
    uclient->id_f = NULL;
    uclient->passphrase_f = NULL;
    $UPROTO$_client_open_int(uclient, conn_str);
}

void $UPROTO$_client_open_ex($UPROTO$_client_t *uclient, char *conn_str, char *(*id_f)(), char *(*pph_f)()) {
    uclient->id_f = id_f;
    uclient->passphrase_f = pph_f;
    $UPROTO$_client_open_int(uclient, conn_str);
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

int $UPROTO$_client_send_ex($UPROTO$_client_t *uclient, $UPROTO$_request_t *request) {
	int ret;
    long nbytes;
	char buff[UCLIENT_BUFSIZE];
    char cipher[UCLIENT_BUFSIZE];
    char message[UCLIENT_BUFSIZE];
    uint32_t timestamp;
    char sts[32];
    char *passphrase = uclient->passphrase_f();
    char otp[100];
    char *id = uclient->id_f();
    char challenge[32];
    int len = 32;
    int len1 = 32;
	
    lvc_t lvc;
    
    timestamp = get_ts();
    len1 = ts2str(timestamp, sts);
    
    generate_otp(otp, passphrase, sts);

    do_encrypt(challenge, &len, sts, len1, otp);

    lvc_init(&lvc, message, UCLIENT_BUFSIZE);

    fprintf(stdout, "lvc_pack id:%s\n", id);
    lvc_pack( &lvc, strlen(id), id );
    fprintf(stdout, "lvc_pack ts:%u\n", timestamp);
    lvc_pack( &lvc, sizeof(uint32_t), (char *)&timestamp );
    fprintf(stdout, "lvc_pack challenge:%d\n", len);
    lvc_pack( &lvc, len, challenge );

	$UPROTO$_build_request(buff, sizeof(buff), request);
    nbytes = strlen(buff);

    fprintf(stdout, "Message to send:%.*s\n", nbytes, buff);
    
    len = sizeof(cipher);
    do_encrypt(cipher, &len, buff, nbytes, otp);

    lvc_pack( &lvc, len, cipher );
    lvc_pack_finish(&lvc);
    
    nbytes = lvc.len;

	ret = pj_sock_sendto(uclient->fd, lvc.data, &nbytes, 0, (const pj_sockaddr_t *)uclient->connect_data, sizeof(pj_sockaddr_in));
	//ret = pj_sock_sendto(uclient->fd, buff, &nbytes, 0, (const pj_sockaddr_t *)uclient->connect_data, sizeof(pj_sockaddr_in));

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

