#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include "master.h"
#include "femail.h"
#include "comm.h"

typedef enum {
	ACCEPT_CONTENT,
	ACCEPT_UNBLOCK,
	ACCEPT_ERROR
} ACCEPT_STATE;

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

int handle_message(int acceptresult) {
	switch(get_accept_state(acceptresult)) {
	case ACCEPT_UNBLOCK: return 0;
	case ACCEPT_ERROR: return 1;
	case ACCEPT_CONTENT:
		int clientsocket = acceptresult;
		log_err("unimplemented");
		close(clientsocket);
		return 1;
	default:
		log_emerg("Unexpected accept status. aborting...");
		abort();
	}
}

int handle_submission(int acceptresult) {
	switch(get_accept_state(acceptresult)) {
	case ACCEPT_UNBLOCK: return 0;
	case ACCEPT_ERROR: return 1;
	case ACCEPT_CONTENT:
		int clientsocket = acceptresult;

		log_warn("SMTPS is unimplemented.");
		dprintf(clientsocket, "421 Unimplemented\n");
		
		close(clientsocket);
		return 0;
	default:
		log_emerg("Unexpected accept status. aborting...");
		abort();
	}
}

int handle_reject_starttls(int acceptresult) {
	switch(get_accept_state(acceptresult)) {
	case ACCEPT_UNBLOCK: return 0;
	case ACCEPT_ERROR: return 1;
	case ACCEPT_CONTENT:
		int clientsocket = acceptresult;

		dprintf(clientsocket, "523 Use SMTPS instead\n");
		
		close(clientsocket);
		return 0;
	default:
		log_emerg("Unexpected accept status. aborting...");
		abort();
	}
}

void * start_master_service(void *) {
	log_info("Starting master service...");

	while(!is_stopped()) {
		if (handle_message(accept(smtpctx.socket4,
								  NULL,
								  NULL)) != 0) {
			log_err("SMTP: Failed to handle connection on IPv4.");
		}

		if (handle_message(accept(smtpctx.socket6,
								  NULL,
								  NULL)) != 0) {
			log_err("SMTP: Failed to handle connection on IPv6.");
		}

		if (handle_submission(accept(smtpsctx.socket4,
									 NULL,
									 NULL)) != 0) {
			log_err("SMTPS: Failed to handle connection on IPv4.");
		}

		if (handle_submission(accept(smtpsctx.socket6,
									 NULL,
									 NULL)) != 0) {
			log_err("SMTPS: Failed to handle connection on IPv6.");
		}

		if (handle_reject_starttls(accept(starttlsctx.socket4,
								 NULL,
								 NULL)) != 0) {
			log_err("StartTLS: Failed to handle connection on IPv4.");
		}

		if (handle_reject_starttls(accept(starttlsctx.socket6,
								 NULL, 
								 NULL)) != 0) {
			log_err("StartTLS: Failed to handle connection on IPv6.");
		}
	}

	log_info("Master service stopped successfully.");

	return NULL;
}
