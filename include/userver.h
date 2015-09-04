#ifndef __USERVER_H__
#define __USERVER_H__
#include <pthread.h>

#include "$UPROTO$.h"
typedef struct $UPROTO$_server_s $UPROTO$_server_t;

struct $UPROTO$_server_s {
    int fd;
    pthread_t master_thread;
    char *connect_str;
    volatile int is_end;
    
    int (*recv_f)(int fd, char *buff, int len, void *data, unsigned int *data_len);
    int (*send_f)(int fd, char *buff, int len, void *data, unsigned int data_len);
    void (*parse_request_f)(char *buffer, int len, $UPROTO$_request_t *request);
    void (*on_request)($UPROTO$_server_t *userver, $UPROTO$_request_t *request, $UPROTO$_response_t *response);
    int (*build_response_f)(char *buffer, int len, $UPROTO$_response_t *response);
};

void $UPROTO$_server_init($UPROTO$_server_t *userver, char *conn_str);

void $UPROTO$_server_start($UPROTO$_server_t *userver);
void $UPROTO$_server_end($UPROTO$_server_t *userver);

#endif
