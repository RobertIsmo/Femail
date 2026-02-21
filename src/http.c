#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "femail.h"
#include "http.h"

ConnectionHandlerResult http_handler(Connection conn[static 1]) {
	if (!conn->live) {
		log_debug("Processing a dead connection. skipping...");
		return CONNECTION_DONE;
	}

	log_debug("Processing a connection\n"		\
			  "Type: %d\n" \
			  "State: %d\n" \
			  "Started: %ld\n" \
			  "Refresh: %ld\n",
			  conn->type,
			  conn->state,
			  conn->timeinitialized,
			  conn->timerefreshed);

	char buffer [SMALL_BUFFER_SIZE] = {0};
	ssize_t readresult = read(conn->clientsocket,
						  buffer,
						  SMALL_BUFFER_SIZE);

	log_debug("read amount: %ld",
			  readresult);
	if (readresult >= SMALL_BUFFER_SIZE - 1) {
		return CONNECTION_CONTINUE;
	} else if (readresult == -1) {
		if (errno == EWOULDBLOCK) {
			return CONNECTION_CONTINUE;
		} else {
			return CONNECTION_ERROR;
		}
	} else if (readresult == 0) {
		return CONNECTION_ERROR;
	}

	log_debug("Received: %s", buffer);

	httpmsg_redirect_https();
	return CONNECTION_DONE;
}

ConnectionHandlerResult https_handler(Connection conn[static 1]) {
	if (!conn->live) {
		log_debug("Processing a dead connection. skipping...");
		return CONNECTION_DONE;
	}

	log_debug("Processing a connection\n"		\
			  "Type: %d\n" \
			  "State: %d\n" \
			  "Started: %ld\n" \
			  "Refresh: %ld\n",
			  conn->type,
			  conn->state,
			  conn->timeinitialized,
			  conn->timerefreshed);

	if (SSL_accept(conn->sslconn) <= 0) {
		log_debug("Failed TLS negotiation.");
		return CONNECTION_ERROR;
	}

	char buffer [SMALL_BUFFER_SIZE] = {0};
	ssize_t readresult = SSL_read(conn->sslconn,
						  buffer,
						  SMALL_BUFFER_SIZE);

	log_debug("read amount: %ld",
			  readresult);

	if (readresult >= SMALL_BUFFER_SIZE - 1) {
		return CONNECTION_CONTINUE;
	} else if (readresult == -1) {
		if (errno == EWOULDBLOCK) {
			return CONNECTION_CONTINUE;
		} else {
			return CONNECTION_ERROR;
		}
	} else if (readresult == 0) {
		return CONNECTION_ERROR;
	}

	httpsmsg_no_content();
	return CONNECTION_DONE;
}
