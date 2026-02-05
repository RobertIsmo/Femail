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

	httpmsg_no_content();
	return CONNECTION_DONE;
}

ConnectionHandlerResult https_handler(Connection *) {
	return CONNECTION_ERROR;
}
