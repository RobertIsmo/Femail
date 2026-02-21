#ifndef MASTER_H
#define MASTER_H

#include <unistd.h>
#include <pthread.h>
#include "femail.h"

#define CONNECTION_QUEUE_CAPACITY 512

typedef enum {
	ACCEPT_CONTENT,
	ACCEPT_UNBLOCK,
	ACCEPT_ERROR
} ACCEPT_STATE;

typedef struct {
	pthread_mutex_t lock;
	size_t count;
	size_t head;
	size_t tail;
	Connection data[CONNECTION_QUEUE_CAPACITY];
} ConnectionQueue;

void connection_init  (Connection [static 1]);
void reset_connection (Connection [static 1]);
void connection_deinit(Connection [static 1]);

int conn_queue_init    (ConnectionQueue [static 1]);
size_t conn_queue_count(ConnectionQueue [static 1]);
int conn_queue_enqueue (ConnectionQueue [static 1],
						Connection);
int conn_queue_dequeue (ConnectionQueue [static 1],
						Connection      [static 1]);

int get_accept_state(int);
int handle_connection(ConnectionType,
					  int,
					  SSL_STRATEGY);
void process_connection(void);

#endif //MASTER_H
