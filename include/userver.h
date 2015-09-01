#ifndef __USERVER_H__
#define __USERVER_H__
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "uproto.h"
#include "proto.h"
typedef struct userver_s userver_t;

typedef void (*userver_callback_t)(userver_t *userver, char *buff, int len);

struct userver_s {
	int socket;
	pthread_t master_thread;
	int port;
	volatile int is_end;
	
	userver_callback_t on_sent;
	userver_callback_t on_received;

	void (*parse_request_f)(char *buffer, int len, uproto_request_t *request);
	void (*on_request)(userver_t *userver, uproto_request_t *request, uproto_response_t *response);
	int (*build_response_f)(char *buffer, int len, uproto_response_t *response);
};

void userver_init(userver_t *userver, int port);

void userver_set_sent_callback(userver_callback_t f);
void userver_set_received_callback(userver_callback_t f);

void userver_start(userver_t *userver);
void userver_end(userver_t *userver);

#endif
