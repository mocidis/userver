#ifndef __USERVER_CLIENT_H__
#define __USERVER_CLIENT_H__

#include <stdarg.h>
#include "uproto.h"
#include "proto.h"

typedef struct uclient_s uclient_t;

struct uclient_s {
	int fd;
	char *connect_str;
    void *connect_data;
	//struct sockaddr_in server_addr;
	void (*parse_response_f)(char *buff, int len, uproto_response_t *resp);
	int (*build_request_f)(char *buff, int len, va_list argp);
};

void uclient_open(uclient_t *uclient, char *conn_str);
int uclient_send(uclient_t *uclient,...);
int uclient_recv(uclient_t *uclient, uproto_response_t *resp);
void ucient_close(uclient_t *uclient);

#endif