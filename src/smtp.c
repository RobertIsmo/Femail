#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "femail.h"
#include "smtp.h"

bool ends_word(char word) {
	return 	word == 0 || word == ' ' || word == '\t'	||
			word	== '\r' || word == '\n';
}

bool found_crlf_end(char * str) {
	size_t offset = 0;
	while (str[offset]) {
		if (str[offset] == '\r') {
			if (strncmp(str + offset,
						"\r\n.\r\n",
						5) == 0) {
				return true;
			}
		}
		offset++;
	}
	return false;
}

int move_client_mail_domain(char * buffer,
							Connection * conn,
							size_t max) {
	size_t offset = 0;
	char * current = buffer + HELLO_COMMAND_SIZE;
	while (!ends_word(*current)) {
		if (offset == max) {
			return 1;
		}
		conn->mailclientdomain[offset] = *current;
		current++;
		offset++;
	}
	conn->mailclientdomain[offset] = 0;
	return 0;
}

ConnectionHandlerResult smtp_handler(Connection * conn) {
	if (conn->state == MAIL_CONNECTION_OPENED) {
		log_debug("SMTP Handler: connection opened.");
		smtpsmsg_accept_connection();
		conn->state = MAIL_CONNECTION_EXPECT_HELLO;
		return CONNECTION_CONTINUE;
	}
	
	size_t readamount = 0;
	char buffer [SMALL_BUFFER_SIZE] = {0};

	log_debug("Reading...");
	readamount = (size_t)read(conn->clientsocket,
							  buffer,
							  SMALL_BUFFER_SIZE);
	if (readamount >= SMALL_BUFFER_SIZE - 1) {
		smtpsmsg_reject_too_long();
		return CONNECTION_ERROR;
	}
	log_debug("Received: %s", buffer);

	if (strncasecmp(buffer,
					"RSET",
					RSET_COMMAND_SIZE) == 0) {
		smtpsmsg_accept_generic();
		return CONNECTION_RESET;
	} else if (strncasecmp(buffer,
					"QUIT",
					QUIT_COMMAND_SIZE) == 0) {
		smtpsmsg_accept_goodbye();
		return CONNECTION_DONE;
	} else if (strncasecmp(buffer,
					"NOOP",
					NOOP_COMMAND_SIZE) == 0) {
		smtpsmsg_accept_generic();
		return CONNECTION_CONTINUE;
	}
	
	switch (conn->state) {
	case MAIL_CONNECTION_EXPECT_HELLO:
		log_debug("SMTP Handler: expecting HELLO.");
		if (strncasecmp(buffer,
						"HELO ",
						HELLO_COMMAND_SIZE) == 0) {
			if (move_client_mail_domain(buffer,
										conn,
										DOMAIN_MAX_SIZE) != 0) {
				log_debug("Couldn't pull the client's mail domain from command.");
				smtpsmsg_reject_too_long();
				return CONNECTION_DONE;
			}
			conn->state = MAIL_CONNECTION_EXPECT_FROM;
			smtpsmsg_accept_hello();
			log_debug("SMTP Handler: Got a HELO.");
			break;
		} else if (strncasecmp(buffer,
							   "EHLO ",
							   HELLO_COMMAND_SIZE) == 0) {
			if (move_client_mail_domain(buffer,
										conn,
										DOMAIN_MAX_SIZE) != 0) {
				log_debug("Couldn't pull the client's mail domain from command.");
				smtpsmsg_reject_too_long();
				return CONNECTION_DONE;
			}
			conn->state = MAIL_CONNECTION_EXPECT_FROM;
			smtpsmsg_accept_ehello();
			log_debug("SMTP Handler: Got a EHLO.");
			break;
		} else {
			smtpsmsg_reject_commands();
			log_debug("SMTP Handler: Didn't get a HELLO, Rejecting instead..");
			return CONNECTION_DONE;
		}
		break;
	case MAIL_CONNECTION_EXPECT_FROM:
		log_debug("SMTP Handler: expecting FROM.");
		// TODO: actually implement handling of FROM
		conn->state = MAIL_CONNECTION_EXPECT_RCPT;
		smtpsmsg_accept_generic();
		break;
	case MAIL_CONNECTION_EXPECT_RCPT:
		log_debug("SMTP Handler: expecting RCPT.");
		if (strncasecmp(buffer,
						"DATA",
						DATA_COMMAND_SIZE) == 0) {
			conn->state = MAIL_CONNECTION_EXPECT_DATA;
			smtpsmsg_start_mail();
			break;
		} else if (strncasecmp(buffer,
							   "RCPT TO:",
							   RCPT_COMMAND_SIZE) == 0) {
			smtpsmsg_accept_generic();
			break;
		} else {
			smtpsmsg_reject_commands();
			return CONNECTION_DONE;
		}
	case MAIL_CONNECTION_EXPECT_DATA:
		log_debug("SMTP Handler: expecting data.");

		conn->alloccount++;
		size_t messagebuffersize = conn->alloccount * SMALL_BUFFER_SIZE + 4;
		if (messagebuffersize > ESMTP_SIZE) {
			log_debug("SMTP Handler: rejecting a large message.");
			smtpsmsg_reject_too_large();
			return CONNECTION_ERROR;
		}
		conn->messagebuffer = realloc(conn->messagebuffer,
									  messagebuffersize);
		if (conn->messagebuffer == NULL) {
			smtpsmsg_reject_error();
			return CONNECTION_ERROR;
		}

		memcpy(conn->messagebuffer + conn->messagebufferend,
			   buffer,
			   readamount);
		conn->messagebufferend += readamount;
		conn->messagebuffer[readamount] = 0;

		if (found_crlf_end(conn->messagebuffer)) {
			log_debug("Found the CRLF!");
			conn->state = MAIL_CONNECTION_EXPECT_CONT;
			smtpsmsg_accept_generic();
			break;
		}
		break;
	case MAIL_CONNECTION_EXPECT_CONT:
		// TODO: Implement open continuation.
		return CONNECTION_CONTINUE;
	default:
		log_err("Unknown connection state %d.", conn->state);
		smtpsmsg_reject_error();
		return CONNECTION_ERROR;
	}
	return CONNECTION_CONTINUE;
}

ConnectionHandlerResult smtps_handler(Connection * conn) {
	smtpsmsg_reject_unimplemented();
	return CONNECTION_DONE;
}

ConnectionHandlerResult starttls_handler(Connection * conn) {
	smtpsmsg_reject_starttls();
	return CONNECTION_DONE;
}
