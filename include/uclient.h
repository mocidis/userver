#ifndef __USERVER_CLIENT_H__
#define __USERVER_CLIENT_H__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "uproto.h"
#include "proto.h"

typedef struct uclient_s uclient_t;

struct uclient_s {
	int socket;
	char server[50];
	struct sockaddr_in server_addr;
	void (*parse_response_f)(char *buff, int len, uproto_response_t *resp);
	int (*build_request_f)(char *buff, int len, va_list argp);
};

void uclient_open(uclient_t *uclient, char *server, int port);
int uclient_send(uclient_t *uclient,...);
int uclient_recv(uclient_t *uclient, uproto_response_t *resp);
void ucient_close(uclient_t *uclient);

#endif
