#ifndef __USERVER_H__
#define __USERVER_H__
#include <pthread.h>

#include "uproto.h"
#include "proto.h"
typedef struct userver_s userver_t;

struct userver_s {
	int fd;
	pthread_t master_thread;
    char *connect_str;
	volatile int is_end;
	
    int (*recv_f)(int fd, char *buff, int len, void *data, unsigned int *data_len);
    int (*send_f)(int fd, char *buff, int len, void *data, unsigned int data_len);
	void (*parse_request_f)(char *buffer, int len, uproto_request_t *request);
	void (*on_request)(userver_t *userver, uproto_request_t *request, uproto_response_t *response);
	int (*build_response_f)(char *buffer, int len, uproto_response_t *response);
};

void userver_init(userver_t *userver, char *conn_str);

void userver_start(userver_t *userver);
void userver_end(userver_t *userver);

#endif
