#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "femail.h"
#include "http.h"

ConnectionHandlerResult http_handler(Connection * conn) {
	if (!conn->live) {
		log_debug("Processing a dead connection. skipping...");
		return CONNECTION_DONE;
	}

	log_debug("Processing a connection\n" \
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

	httpmsg_no_content();
	return CONNECTION_DONE;
}

ConnectionHandlerResult https_handler(Connection *) {
	return CONNECTION_ERROR;
}
