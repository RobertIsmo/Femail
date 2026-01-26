#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "master.h"
#include "femail.h"

ConnectionQueue connqueue = {0};

int conn_queue_init(ConnectionQueue * queue) {
	pthread_mutex_init(&queue->lock,
					   NULL);
	queue->count = 0;
	queue->head = 0;
	queue->tail = 0;
	memset(queue->data,
		   0,
		   CONNECTION_QUEUE_CAPACITY * sizeof(Connection));
	return 0;
}
size_t conn_queue_count(ConnectionQueue * queue) {
	if (pthread_mutex_lock(&queue->lock) != 0) {
		log_alert("Mutex lock failure, system may begin failing.");
		return 0;
	}
	size_t result = queue->count;
	if (pthread_mutex_unlock(&queue->lock) != 0) {
		log_alert("Mutex unlock failure, system may begin failing.");
		return 0;
	}
	return result;
}
int conn_queue_enqueue(ConnectionQueue * queue,
					  Connection conn) {
	if (pthread_mutex_lock(&queue->lock) != 0) {
		log_alert("Mutex lock failure, system may begin failing.");
		return 1;
	}
	if (queue->count == CONNECTION_QUEUE_CAPACITY) {
		log_err("Connection queue filled, rejecting a connection.");
		if (pthread_mutex_unlock(&queue->lock) != 0) {
			log_alert("Mutex unlock failure, system may begin failing.");
			return 1;
		}
		return 1;
	}
	queue->data[queue->head] = conn;
	queue->head = (queue->head + 1) % CONNECTION_QUEUE_CAPACITY;
	queue->count++;
	if (pthread_mutex_unlock(&queue->lock) != 0) {
		log_alert("Mutex unlock failure, system may begin failing.");
		return 1;
	}
	return 0;
}

int conn_queue_dequeue(ConnectionQueue * queue,
					  Connection * conn) {
	if (pthread_mutex_lock(&queue->lock) != 0) {
		log_alert("Mutex lock failure, system may begin failing.");
		return 1;
	}
	if (queue->count == 0) {
		if (pthread_mutex_unlock(&queue->lock) != 0) {
			log_alert("Mutex unlock failure, system may begin failing.");
			return 1;
		}
		return 1;
	}
	*conn = queue->data[queue->tail];
	queue->tail = (queue->tail + 1) % CONNECTION_QUEUE_CAPACITY;
	queue->count--;
	if (pthread_mutex_unlock(&queue->lock) != 0) {
		log_alert("Mutex unlock failure, system may begin failing.");
		return 1;
	}
	return 0;
}

int get_accept_state(int acceptresult) {
	if (acceptresult == -1) {
		if (errno == EWOULDBLOCK) {
			return ACCEPT_UNBLOCK;
		} else {
			return ACCEPT_ERROR;
		}
	} else {
		return ACCEPT_CONTENT;
	}	
}

int handle_connection(ConnectionType type,
					  int clientsocket) {
	switch (get_accept_state(clientsocket)) {
	case ACCEPT_UNBLOCK: return 0;
	case ACCEPT_ERROR: return 1;
	case ACCEPT_CONTENT:
		log_debug("Received a connection");
		Connection mailconn = {
			.type = type,
			.clientsocket = clientsocket,
			.state = MAIL_CONNECTION_OPENED
		};
		if (conn_queue_enqueue(&connqueue,
							  mailconn) != 0) {
			log_err("Failed to enqueue a connection. rejecting...");
			close(clientsocket);
		}
		return 0;
	default:
		log_emerg("Unexpected accept status. aborting...");
		abort();
	}
}

void process_connection(void) {
	Connection conn = {0};
	if (conn_queue_dequeue(&connqueue,
						  &conn) != 0) {
		return;
	}

	log_debug("Processing a connection.");
	switch (conn.type) {
	case SMTP_CONNECTION:
		dprintf(conn.clientsocket, "421 Unimplemented\n");
		close(conn.clientsocket);
		break;
	case SMTPS_CONNECTION:
		dprintf(conn.clientsocket, "421 Submission Unimplemented\n");
		close(conn.clientsocket);
		break;
	case STARTTLS_CONNECTION:
		dprintf(conn.clientsocket, "523 Use SMTPS instead\n");
		close(conn.clientsocket);
		break;
	default:
		log_emerg("Unexpected mail connection type. aborting...");
		abort();
	}
}

void * start_master_service(void *) {
	log_info("Starting master service...");

	if(conn_queue_init(&connqueue) != 0) {
		log_err("Couldn't initialize the mail connection queue.");
		return NULL;
	}
	
	while(!is_stopped()) {
		if (handle_connection(SMTP_CONNECTION,
							  accept(smtpctx.socket4,
									 NULL,
									 NULL)) != 0) {
			log_err("SMTP: Failed to handle connection on IPv4.");
		}

		if (handle_connection(SMTP_CONNECTION,
							  accept(smtpctx.socket6,
									 NULL,
									 NULL)) != 0) {
			log_err("SMTP: Failed to handle connection on IPv6.");
		}

		if (handle_connection(SMTPS_CONNECTION,
							  accept(smtpsctx.socket4,
									 NULL,
									 NULL)) != 0) {
			log_err("SMTPS: Failed to handle connection on IPv4.");
		}

		if (handle_connection(SMTPS_CONNECTION,
							  accept(smtpsctx.socket6,
									 NULL,
									 NULL)) != 0) {
			log_err("SMTPS: Failed to handle connection on IPv6.");
		}

		if (handle_connection(STARTTLS_CONNECTION,
							  accept(starttlsctx.socket4,
									 NULL,
									 NULL)) != 0) {
			log_err("StartTLS: Failed to handle connection on IPv4.");
		}

		if (handle_connection(STARTTLS_CONNECTION,
							  accept(starttlsctx.socket6,
									 NULL, 
									 NULL)) != 0) {
			log_err("StartTLS: Failed to handle connection on IPv6.");
		}

		process_connection();
	}

	log_info("Master service stopped successfully.");

	return NULL;
}
