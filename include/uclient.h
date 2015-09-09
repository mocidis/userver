#include <stdarg.h>
#include "$UPROTO$.h"

typedef struct $UPROTO$_client_s $UPROTO$_client_t;

struct $UPROTO$_client_s {
	int fd;
	char *connect_str;
    void *connect_data;
};

void $UPROTO$_client_open($UPROTO$_client_t *uclient, char *conn_str);
int $UPROTO$_client_send($UPROTO$_client_t *uclient, $UPROTO$_request_t *req);
//int $UPROTO$_client_recv($UPROTO$_client_t *uclient, $UPROTO$_response_t *resp);
void $UPROTO$_client_close($UPROTO$_client_t *uclient);
