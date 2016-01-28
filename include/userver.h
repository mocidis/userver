#include <pjlib.h>
#include "$UPROTO$.h"

typedef struct $UPROTO$_server_s $UPROTO$_server_t;

struct $UPROTO$_server_s {
    pj_sock_t fd;
    pj_pool_t *pool;
    pj_thread_t *master_thread;
    pj_mutex_t *mutex;
    char connect_str[30];
    volatile int is_end;
    int (*recv_f)(int fd, char *buff, int len, void *data, unsigned int *data_len);
    int (*send_f)(int fd, char *buff, int len, void *data, unsigned int data_len);
    void (*on_request_f)($UPROTO$_server_t *userver, $UPROTO$_request_t *request, char *caddr_str);
    void (*on_init_done_f)($UPROTO$_server_t *userver);
    void (*on_open_socket_f)($UPROTO$_server_t *userver);
    char *(*get_pph_f)(pj_str_t *id);
    void *user_data;

    int is_online;
};

void $UPROTO$_server_init($UPROTO$_server_t *userver, char *conn_str, pj_pool_t *pool);
void $UPROTO$_server_init_ex($UPROTO$_server_t *userver, char *conn_str, pj_pool_t *pool, char *(*get_pph_f)(pj_str_t *id));
void $UPROTO$_server_start($UPROTO$_server_t *userver);
void $UPROTO$_server_start_ex($UPROTO$_server_t *userver);
void $UPROTO$_server_join($UPROTO$_server_t *userver, char *multicast_ip);
void $UPROTO$_server_leave($UPROTO$_server_t *userver, char *multicast_ip);
void $UPROTO$_server_end($UPROTO$_server_t *userver);
