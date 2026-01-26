#ifndef MASTER_H
#define MASTER_H

#include <unistd.h>
#include <pthread.h>

#define MAIL_CONNECTION_QUEUE_CAPACITY 512

#define smtpsmsg_reject_unimplemented() dprintf(clientsocket, "421 Unimplemented\n");
#define smtpsmsg_reject_starttls() dprintf(clientsocket, "523 Use SMTPS instead\n");

typedef enum {
	ACCEPT_CONTENT,
	ACCEPT_UNBLOCK,
	ACCEPT_ERROR
} ACCEPT_STATE;

typedef enum {
	SMTP_CONNECTION,
	SMTPS_CONNECTION,
	STARTTLS_CONNECTION
} MailConnectionType;

typedef enum {
	MAIL_CONNECTION_OPENED
} MailConnectionState;

typedef struct {
	MailConnectionType type;
	MailConnectionState state;
	int clientsocket;
} MailConnection;

typedef struct {
	pthread_mutex_t lock;
	size_t count;
	size_t head;
	size_t tail;
	MailConnection data[MAIL_CONNECTION_QUEUE_CAPACITY];
} MailConnectionQueue;

int mailconnq_init(MailConnectionQueue *);
size_t mailconnq_count(MailConnectionQueue *);
int mailconnq_enqueue(MailConnectionQueue *,
				  MailConnection);
int mailconnq_dequeue(MailConnectionQueue *,
					  MailConnection *);

int get_accept_state(int);
int handle_connection(MailConnectionType,
					  int);
void process_mail_connection(void);
void * start_master_service(void *);

#endif //MASTER_H
