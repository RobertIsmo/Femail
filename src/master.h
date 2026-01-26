#ifndef MASTER_H
#define MASTER_H

#include <unistd.h>
#include <pthread.h>
#include "femail.h"

#define CONNECTION_QUEUE_CAPACITY 512

#define smtpsmsg_reject_unimplemented() dprintf(clientsocket, "421 Unimplemented\n");
#define smtpsmsg_reject_starttls() dprintf(clientsocket, "523 Use SMTPS instead\n");

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

int conn_queue_init(ConnectionQueue *);
size_t conn_queue_count(ConnectionQueue *);
int conn_queue_enqueue(ConnectionQueue *,
				  Connection);
int conn_queue_dequeue(ConnectionQueue *,
					   Connection *);

int get_accept_state(int);
int handle_connection(ConnectionType,
					  int);
void process_connection(void);

#endif //MASTER_H
