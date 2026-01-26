#define _GNU_SOURCE
#include <stdio.h>
#include "femail.h"

ConnectionHandlerResult smtp_handler(Connection * conn) {
	dprintf(conn->clientsocket,
			"421 Unimplemented\n");
	return CONNECTION_DONE;
}

ConnectionHandlerResult smtps_handler(Connection * conn) {
	dprintf(conn->clientsocket,
			"421 Submission Unimplemented\n");
	return CONNECTION_DONE;
}

ConnectionHandlerResult starttls_handler(Connection * conn) {
	dprintf(conn->clientsocket,
			"523 Use SMTPS instead\n");
	return CONNECTION_DONE;
}
