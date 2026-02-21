#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <openssl/ssl.h>
#include "femail.h"
#include "smtp.h"

int parse_client_command(SMTPClientCommand * command,
						 char * buffer) {
	if (strncasecmp(buffer,
					"NOOP",
					4) == 0) {
		command->command = SMTP_NOOP;
		command->param = NULL;
		return 0;
	} else if (strncasecmp(buffer,
					"QUIT",
					4) == 0) {
		command->command = SMTP_QUIT;
		command->param = NULL;
		return 0;
	} else if (strncasecmp(buffer,
					"RSET",
					4) == 0) {
		command->command = SMTP_RSET;
		command->param = NULL;
		return 0;
	} else if (strncasecmp(buffer,
					"DATA",
					4) == 0) {
		command->command = SMTP_DATA;
		command->param = NULL;
		return 0;
	} else if (strncasecmp(buffer,
					"HELP",
					4) == 0) {
		command->command = SMTP_HELP;
		command->param = buffer + 5;
		return 0;
	} else if (strncasecmp(buffer,
					"HELO ",
					5) == 0) {
		command->command = SMTP_HELO;
		command->param = buffer + 5;
		return 0;
	} else if (strncasecmp(buffer,
					"EHLO ",
					5) == 0) {
		command->command = SMTP_EHLO;
		command->param = buffer + 5;
		return 0;
	} else if (strncasecmp(buffer,
					"MAIL FROM:",
					10) == 0) {
		command->command = SMTP_MAIL;
		command->param = buffer + 10;
		return 0;
	} else if (strncasecmp(buffer,
					"RCPT TO:",
					8) == 0) {
		command->command = SMTP_RCPT;
		command->param = buffer + 8;
		return 0;
	} else if (strncasecmp(buffer,
					"VRFY ",
					5) == 0) {
		command->command = SMTP_VRFY;
		command->param = buffer + 5;
		return 0;
	} else if (strncasecmp(buffer,
					"EXPN ",
					5) == 0) {
		command->command = SMTP_EXPN;
		command->param = buffer + 5;
		return 0;
	} else if (strncasecmp(buffer,
					"STARTTLS",
					8) == 0) {
		command->command = SMTP_STARTTLS;
		command->param = buffer + 8;
		return 0;
	} else {
		return 1;
	}
}

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

int move_client_mail_domain(char * param,
							Connection * conn,
							size_t max) {
	size_t offset = 0;
	char * current = param;
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

ssize_t smtp_receive_msg(Connection * conn,
						 char * buf,
						 int nbytes) {
	if (conn->sslconn != NULL) {
		log_debug("SMTP: using SSL connection.");
		return SSL_read(conn->sslconn,
						buf,
						nbytes);
	} else {
		log_debug("SMTP: using PLAIN connection.");
		return read(conn->clientsocket,
					buf,
					(size_t)nbytes);
	}
}

ConnectionHandlerResult smtp_handler(Connection conn[static 1]) {
	if (!conn->live) {
		log_debug("Processing a dead connection. skipping...");
		return CONNECTION_DONE;
	}

#ifdef DEBUG
	size_t messagelength = 0;
	if (conn->messagebuffer) {
		messagelength = strlen(conn->messagebuffer);
	}
	log_debug("Processing a connection\n" \
			  "Type: %d\n" \
			  "State: %d\n" \
			  "Started: %ld\n" \
			  "Refresh: %ld\n" \
			  "Alloc Count: %ld\n" \
			  "Buffer Data Size: %ld\n" \
			  "Buffer String Length: %ld",
			  conn->type,
			  conn->state,
			  conn->timeinitialized,
			  conn->timerefreshed,
			  conn->alloccount,
			  conn->messagebufferend,
			  messagelength);
#endif
	
	if (conn->state == MAIL_CONNECTION_OPENED) {
		smtpsmsg_accept_connection();
		conn->state = MAIL_CONNECTION_EXPECT_ANY;
		return CONNECTION_CONTINUE;
	}

	if (conn->state == MAIL_CONNECTION_EXPECT_TLS_NEG) {
		if (SSL_accept(conn->sslconn) <= 0) {
			log_err("Failed TLS negotiation.");
			smtpsmsg_reject_starttls();
			return CONNECTION_ERROR;
		}
		conn->state = MAIL_CONNECTION_EXPECT_ANY;
		return CONNECTION_CONTINUE;
	}
	
	char buffer [SMALL_BUFFER_SIZE] = {0};
	ssize_t readresult = smtp_receive_msg(conn,
										  buffer,
										  SMALL_BUFFER_SIZE);

	log_debug("read amount: %ld",
			  readresult);
	if (readresult >= SMALL_BUFFER_SIZE - 1 &&
		conn->state != MAIL_CONNECTION_EXPECT_DATA) {
		smtpsmsg_reject_too_long();
		return CONNECTION_ERROR;
	} else if (readresult == -1) {
		if (errno == EWOULDBLOCK) {
			return CONNECTION_CONTINUE;
		} else {
			log_err("%s\n", strerror(errno));
			return CONNECTION_ERROR;
		}
	} else if (readresult == 0) {
		return CONNECTION_ERROR;
	}

	size_t readamount = (size_t)readresult;
	
	if (conn->state == MAIL_CONNECTION_EXPECT_DATA) {
		log_debug("SMTP Handler: handling message data...");

		conn->alloccount++;
		size_t messagebuffersize = (conn->alloccount * SMALL_BUFFER_SIZE) + 1;
		if (messagebuffersize > ESMTP_SIZE) {
			log_debug("SMTP Handler: rejecting a large message.");
			smtpsmsg_reject_too_large();
			return CONNECTION_RESET;
		}
		conn->messagebuffer = realloc(conn->messagebuffer,
									  messagebuffersize);
		if (conn->messagebuffer == NULL) {
			log_err("Allocation Error");
			smtpsmsg_reject_error();
			return CONNECTION_ERROR;
		}

		memcpy(conn->messagebuffer + conn->messagebufferend,
			   buffer,
			   readamount);
		conn->messagebufferend += readamount;
		conn->messagebuffer[conn->messagebufferend] = 0;

		if (found_crlf_end(conn->messagebuffer)) {
			log_debug("Found the CRLF!");
			conn->state = MAIL_CONNECTION_GREETED;
			smtpsmsg_accept_ok();
			log_debug("\n%s\n",
					  conn->messagebuffer);
			return CONNECTION_RESET;
		}
		log_debug("Continuing.");
		return CONNECTION_CONTINUE;
	}

	log_debug("Received: %s", buffer);
	
	SMTPClientCommand command;
	if (parse_client_command(&command,
							 buffer) != 0) {
		smtpsmsg_reject_unrecognized();
		return CONNECTION_CONTINUE;
	}

	switch (command.command) {
	case SMTP_NOOP:
		log_debug("SMTP Handler: handling NOOP...");
		return CONNECTION_CONTINUE;
	case SMTP_QUIT:
		log_debug("SMTP Handler: handling QUIT...");
		smtpsmsg_accept_goodbye();
		return CONNECTION_DONE;
	case SMTP_RSET:
		log_debug("SMTP Handler: handling RSET...");
		smtpsmsg_accept_ok();
		return CONNECTION_RESET;
	case SMTP_DATA:
		log_debug("SMTP Handler: handling DATA...");
		conn->state = MAIL_CONNECTION_EXPECT_DATA;
		smtpsmsg_start_data();
		return CONNECTION_CONTINUE;
	case SMTP_HELP:
		log_debug("SMTP Handler: handling HELP...");
		// TODO: Should look something like this `214 2.0.0 Commands: AUTH BDAT DATA EHLO ETRN HELO HELP MAIL NOOP QUIT RCPT RSET STARTTLS VRFY XCLIENT XFORWARD`
		smtpsmsg_accept_ok();
		return CONNECTION_CONTINUE;
	case SMTP_HELO:
		log_debug("SMTP Handler: handling HELO...");
		if (move_client_mail_domain(command.param,
									conn,
									DOMAIN_MAX_SIZE) != 0) {
			log_debug("Couldn't pull the client's mail domain from command.");
			smtpsmsg_reject_too_long();
			return CONNECTION_CONTINUE;
		}
		conn->state = MAIL_CONNECTION_GREETED;
		smtpsmsg_accept_hello();
		return CONNECTION_CONTINUE;
	case SMTP_EHLO:
		log_debug("SMTP Handler: handling EHLO...");
		if (move_client_mail_domain(command.param,
									conn,
									DOMAIN_MAX_SIZE) != 0) {
			log_debug("Couldn't pull the client's mail domain from command.");
			smtpsmsg_reject_too_long();
			return CONNECTION_CONTINUE;
		}
		conn->state = MAIL_CONNECTION_GREETED;
		smtpsmsg_accept_ehello();
		return CONNECTION_CONTINUE;
	case SMTP_MAIL:
		log_debug("SMTP Handler: handling MAIL...");
		// TODO: technically mail should be acting as a reset
		if (conn->state == MAIL_CONNECTION_EXPECT_ANY) {
			smtpsmsg_reject_sequence();
			return CONNECTION_CONTINUE;
		}
		conn->state = MAIL_CONNECTION_MAILING;
		smtpsmsg_accept_ok();
		return CONNECTION_CONTINUE;
	case SMTP_RCPT:
		log_debug("SMTP Handler: handling RCPT...");
		if (conn->state != MAIL_CONNECTION_MAILING &&
			conn->state != MAIL_CONNECTION_RCPT) {
			smtpsmsg_reject_sequence();
			return CONNECTION_CONTINUE;
		}
		conn->state = MAIL_CONNECTION_RCPT;
		smtpsmsg_accept_ok();
		return CONNECTION_CONTINUE;
	case SMTP_VRFY:
		log_debug("SMTP Handler: handling VRFY...");
		smtpsmsg_reject_unimplemented();
		return CONNECTION_CONTINUE;
	case SMTP_EXPN:
		log_debug("SMTP Handler: handling EXPN...");
		smtpsmsg_reject_unimplemented();
		return CONNECTION_CONTINUE;
	case SMTP_STARTTLS:
		conn->state = MAIL_CONNECTION_EXPECT_TLS_NEG;
		smtpsmsg_accept_starttls();
		return CONNECTION_CONTINUE;
	default:
		log_err("Unknown client command being processed %d.",
				command.command);
		smtpsmsg_reject_error();
		return CONNECTION_ERROR;
	}
}

ConnectionHandlerResult smtps_handler(Connection conn[static 1]) {
	smtpsmsg_reject_unimplemented();
	return CONNECTION_DONE;
}

ConnectionHandlerResult starttls_handler(Connection conn[static 1]) {
	smtpsmsg_reject_starttls();
	return CONNECTION_DONE;
}
