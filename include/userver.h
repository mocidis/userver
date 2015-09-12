#include <pthread.h>
#include "$UPROTO$.h"

typedef struct $UPROTO$_server_s $UPROTO$_server_t;

struct $UPROTO$_server_s {
    int fd;
    pthread_t master_thread;
    pthread_mutex_t mutex;
    char *connect_str;
    volatile int is_end;
    int (*recv_f)(int fd, char *buff, int len, void *data, unsigned int *data_len);
    int (*send_f)(int fd, char *buff, int len, void *data, unsigned int data_len);
    void (*on_request_f)($UPROTO$_server_t *userver, $UPROTO$_request_t *request);
    void (*on_init_done_f)($UPROTO$_server_t *userver);
    void (*on_open_socket_f)($UPROTO$_server_t *userver);
    void *user_data;
};

void $UPROTO$_server_init($UPROTO$_server_t *userver, char *conn_str);
void $UPROTO$_server_start($UPROTO$_server_t *userver);
void $UPROTO$_server_join($UPROTO$_server_t *userver, char *multicast_ip);
void $UPROTO$_server_leave($UPROTO$_server_t *userver, char *multicast_ip);
void $UPROTO$_server_end($UPROTO$_server_t *userver);
