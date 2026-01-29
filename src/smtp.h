#ifndef SMTP_H
#define SMTP_H

#include <stdbool.h>
#include "femail.h"

#define HELLO_COMMAND_SIZE	5
#define NOOP_COMMAND_SIZE	4
#define DATA_COMMAND_SIZE	4
#define RCPT_COMMAND_SIZE	8
#define QUIT_COMMAND_SIZE	4
#define ESMTP_SIZE			10485760
#define SMALL_BUFFER_SIZE	1024

#define smtpsmsg_accept_generic()		dprintf(conn->clientsocket, "250 Ok\r\n")
#define smtpsmsg_accept_connection()	dprintf(conn->clientsocket, "220 %s ESMTP Femail\r\n", get_mail_domain())
#define smtpsmsg_accept_goodbye()		dprintf(conn->clientsocket, "221 Bye\r\n")
#define smtpsmsg_accept_hello()			dprintf(conn->clientsocket, "250 Hello %s\r\n", conn->mailclientdomain)
#define smtpsmsg_accept_ehello()		dprintf(conn->clientsocket, "250-%s Hello %s\n250 SIZE %d\r\n",	\
												get_mail_domain(),		\
												conn->mailclientdomain, \
												ESMTP_SIZE)
#define smtpsmsg_start_mail()			dprintf(conn->clientsocket, "354\r\n")
#define smtpsmsg_reject_error()			dprintf(conn->clientsocket, "500 Bye\r\n")
#define smtpsmsg_reject_commands()		dprintf(conn->clientsocket, "503 Bye\r\n")
#define smtpsmsg_reject_too_long()		dprintf(conn->clientsocket, "500 Too long\r\n")
#define smtpsmsg_reject_too_large()		dprintf(conn->clientsocket, "554 5.3.4 Message Too Large\r\n")
#define smtpsmsg_reject_unimplemented() dprintf(conn->clientsocket, "502 Submission unimplemented\r\n")
#define smtpsmsg_reject_starttls()		dprintf(conn->clientsocket, "523 Use SMTPS instead\r\n")

bool	ends_word(char);
bool	found_crlf_end(char *);
int		move_client_mail_domain(char *,
								Connection *,
								size_t);

#endif //SMTP_H
