#ifndef __USERVER_CLIENT_H__
#define __USERVER_CLIENT_H__

#include <stdarg.h>
#include "$UPROTO$.h"

typedef struct $UPROTO$_client_s $UPROTO$_client_t;

struct $UPROTO$_client_s {
	int fd;
	char *connect_str;
    void *connect_data;
	//struct sockaddr_in server_addr;
	void (*parse_response_f)(char *buff, int len, $UPROTO$_response_t *resp);
	int (*build_request_f)(char *buff, int len, va_list argp);
};

void $UPROTO$_client_open($UPROTO$_client_t *uclient, char *conn_str);
int $UPROTO$_client_send($UPROTO$_client_t *uclient,...);
int $UPROTO$_client_recv($UPROTO$_client_t *uclient, $UPROTO$_response_t *resp);
void $UPROTO$_client_close($UPROTO$_client_t *uclient);

#endif
