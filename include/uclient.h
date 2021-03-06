#include <stdarg.h>
#include <pjlib.h>
#include "lvcode.h"
#include "$UPROTO$.h"

typedef struct $UPROTO$_client_s $UPROTO$_client_t;

struct $UPROTO$_client_s {
	pj_sock_t fd;
	char connect_str[30];
    void *connect_data;
    char *(*id_f)();
    char *(*passphrase_f)();
};

void $UPROTO$_client_open($UPROTO$_client_t *uclient, char *conn_str, char *(*id_f)(), char *(*pph_f)());
//void $UPROTO$_client_open_ex($UPROTO$_client_t *uclient, char *conn_str, char *(*id_f)(), char *(*pph_f)());
int $UPROTO$_client_send($UPROTO$_client_t *uclient, $UPROTO$_request_t *req);
//int $UPROTO$_client_send_ex($UPROTO$_client_t *uclient, $UPROTO$_request_t *req);
//int $UPROTO$_client_recv($UPROTO$_client_t *uclient, $UPROTO$_response_t *resp);
void $UPROTO$_client_close($UPROTO$_client_t *uclient);
