#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "femail.h"
#include "master.h"

ConnectionQueue connqueue = {0};

void connection_init(Connection conn[static 1]) {
	time(&conn->timeinitialized);
	time(&conn->timerefreshed);
	conn->live		  = true;
	// these two will be set else where
	// conn->type			= type;
	// conn->clientsocket	= clientsocket;
	switch (conn->sslstrat) {
	case IMPLICIT_TLS:
		conn->sslconn = SSL_new(sslctx);
		SSL_set_fd(conn->sslconn,
				   conn->clientsocket);
		break;
	case OPPORTUNISTIC_TLS:
		// TODO: implement
		break;
	case NO_SSL:
		break;
	default:
		log_emerg("Unexpected SSL strategy. aborting...");
		abort();
	}
	switch (conn->type) {
	case SMTP_CONNECTION:
	case SMTPS_CONNECTION:
	case STARTTLS_CONNECTION:
		conn->state	= MAIL_CONNECTION_OPENED;
		break;
	case HTTP_CONNECTION:
	case HTTPS_CONNECTION:
		conn->state	= HTTP_CONNECTION_OPENED;
		break;
	default:
		log_emerg("Unexpected connection type. aborting...");
		abort();
	}
	conn->alloccount		= 0;
	conn->messagebuffersize = 0;
	conn->messagebufferend  = 0;
	conn->messagebuffer		= NULL;
}

void reset_connection(Connection conn[static 1]) {
	conn->live		= true;
	if (conn->state == MAIL_CONNECTION_GREETED) {		
		conn->state = MAIL_CONNECTION_GREETED;
	} else {
		conn->state	= MAIL_CONNECTION_EXPECT_ANY;
	}
	conn->alloccount		= 0;
	conn->messagebuffersize = 0;
	conn->messagebufferend  = 0;
	free(conn->messagebuffer);
	conn->messagebuffer		= NULL;
}

void connection_deinit(Connection conn[static 1]) {
	conn->live = false;
	SSL_free(conn->sslconn);
	free(conn->messagebuffer);
	close(conn->clientsocket);
}

int conn_queue_init(ConnectionQueue queue[static 1]) {
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
size_t conn_queue_count(ConnectionQueue queue[static 1]) {
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
int conn_queue_enqueue(ConnectionQueue queue[static 1],
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
	time(&conn.timerefreshed);
	queue->data[queue->head] = conn;
	queue->head = (queue->head + 1) % CONNECTION_QUEUE_CAPACITY;
	queue->count++;
	if (pthread_mutex_unlock(&queue->lock) != 0) {
		log_alert("Mutex unlock failure, system may begin failing.");
		return 1;
	}
	return 0;
}

int conn_queue_dequeue(ConnectionQueue queue[static 1],
					  Connection conn[static 1]) {
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
					  int clientsocket,
					  SSL_STRATEGY strat) {
	switch (get_accept_state(clientsocket)) {
	case ACCEPT_UNBLOCK: return 0;
	case ACCEPT_ERROR: return 1;
	case ACCEPT_CONTENT:
		log_debug("Received a connection");
		Connection conn;
		conn.type = type;
		conn.clientsocket = clientsocket;
		conn.sslstrat = strat;
		connection_init(&conn);
		if (conn_queue_enqueue(&connqueue,
							  conn) != 0) {
			log_err("Failed to enqueue a connection. rejecting...");
			connection_deinit(&conn);
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
	
	ConnectionHandlerResult result = CONNECTION_ERROR;
	switch (conn.type) {
	case SMTP_CONNECTION:
		result = smtp_handler(&conn);
		break;
	case SMTPS_CONNECTION:
		result = smtps_handler(&conn);
		break;
	case STARTTLS_CONNECTION:
		result = starttls_handler(&conn);
		break;
	case HTTP_CONNECTION:
		result = http_handler(&conn);
		break;
	case HTTPS_CONNECTION:
		result = https_handler(&conn);
		break;
	default:
		log_emerg("Unexpected mail connection type. aborting...");
		abort();
	}

	switch(result) {
	case CONNECTION_CONTINUE:
		if (conn_queue_enqueue(&connqueue,
							   conn) != 0) {
			log_err("Failed to renqueue a connection. dropping...");
			log_debug("Closing...");
			connection_deinit(&conn);
		}
		break;
	case CONNECTION_RESET:
		reset_connection(&conn);
		if (conn_queue_enqueue(&connqueue,
							   conn) != 0) {
			log_err("Failed to renqueue a reset connection. dropping...");
			log_debug("Closing...");
			connection_deinit(&conn);
		}
		break;
	case CONNECTION_DONE:
		log_debug("Connection finished. Closing...");
		connection_deinit(&conn);
		break;
	case CONNECTION_ERROR:
		log_err("Received an error while handling a connection.");
		log_debug("Closing...");
		connection_deinit(&conn);
		break;
	default:
		log_emerg("Unexpected connection handler result. aborting...");
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
		if (smtpctx.active) {
			SSL_STRATEGY strat;
			if (smtpctx.starttls) {
				strat = OPPORTUNISTIC_TLS;
			} else {
				strat = NO_SSL;
			}
			if (handle_connection(SMTP_CONNECTION,
								  accept(smtpctx.socket4,
										 NULL,
										 NULL),
								  strat) != 0) {
				log_err("SMTP: Failed to handle connection on IPv4.");
			}

			if (handle_connection(SMTP_CONNECTION,
								  accept(smtpctx.socket6,
										 NULL,
										 NULL),
								  strat) != 0) {
				log_err("SMTP: Failed to handle connection on IPv6.");
			}
		}

		if (smtpsctx.active) {
			if (handle_connection(SMTPS_CONNECTION,
								  accept(smtpsctx.socket4,
										 NULL,
										 NULL),
								  IMPLICIT_TLS) != 0) {
				log_err("SMTPS: Failed to handle connection on IPv4.");
			}

			if (handle_connection(SMTPS_CONNECTION,
								  accept(smtpsctx.socket6,
										 NULL,
										 NULL),
								  IMPLICIT_TLS) != 0) {
				log_err("SMTPS: Failed to handle connection on IPv6.");
			}
		}

		if (starttlsctx.active) {
			if (handle_connection(STARTTLS_CONNECTION,
								  accept(starttlsctx.socket4,
										 NULL,
										 NULL),
								  OPPORTUNISTIC_TLS) != 0) {
				log_err("StartTLS: Failed to handle connection on IPv4.");
			}

			if (handle_connection(STARTTLS_CONNECTION,
								  accept(starttlsctx.socket6,
										 NULL, 
										 NULL),
								  OPPORTUNISTIC_TLS) != 0) {
				log_err("StartTLS: Failed to handle connection on IPv6.");
			}
		}

		if (httpctx.active) {
			if (handle_connection(HTTP_CONNECTION,
								  accept(httpctx.socket4,
										 NULL,
										 NULL),
								  NO_SSL) != 0) {
				log_err("HTTP: Failed to handle connection on IPv4.");
			}

			if (handle_connection(HTTP_CONNECTION,
								  accept(httpctx.socket6,
										 NULL,
										 NULL),
								  NO_SSL) != 0) {
				log_err("HTTP: Failed to handle connection on IPv6.");
			}
		}

		if (httpsctx.active) {
			if (handle_connection(HTTPS_CONNECTION,
								  accept(httpsctx.socket4,
										 NULL,
										 NULL),
								  IMPLICIT_TLS) != 0) {
				log_err("HTTPS: Failed to handle connection on IPv4.");
			}

			if (handle_connection(HTTPS_CONNECTION,
								  accept(httpsctx.socket6,
										 NULL,
										 NULL),
								  IMPLICIT_TLS) != 0) {
				log_err("HTTPS: Failed to handle connection on IPv6.");
			}
		}

		process_connection();
	}

	log_info("Master service stopped successfully.");

	return NULL;
}
