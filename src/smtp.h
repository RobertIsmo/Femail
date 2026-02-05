#ifndef SMTP_H
#define SMTP_H

#include <stdbool.h>
#include "femail.h"

#define ESMTP_SIZE			10485760
#define SMALL_BUFFER_SIZE	1024

typedef enum {
	SMTP_NOOP,
	SMTP_QUIT,
	SMTP_RSET,
	SMTP_DATA,
	SMTP_HELP,
	SMTP_HELO,
	SMTP_EHLO,
	SMTP_MAIL,
	SMTP_RCPT,
	SMTP_VRFY,
	SMTP_EXPN
} SMTPCommand;

typedef struct {
	SMTPCommand command;
	char * param;
} SMTPClientCommand;


#define smtpsmsg_accept_connection()	dprintf(conn->clientsocket, "220 %s ESMTP Femail\r\n", get_mail_domain())
#define smtpsmsg_accept_hello()			dprintf(conn->clientsocket, "250 Hello %s\r\n", conn->mailclientdomain)
#define smtpsmsg_accept_ehello()		dprintf(conn->clientsocket, \
												"250-%s Hello %s\n"	\
												"250-SIZE %d\n"	\
												"250-STARTTLS\n" \
												"250 HELP\r\n",	\
												get_mail_domain(),	\
												conn->mailclientdomain,	\
												ESMTP_SIZE)
#define smtpsmsg_accept_ok()			dprintf(conn->clientsocket, "250 Ok\r\n")
#define smtpsmsg_accept_goodbye()		dprintf(conn->clientsocket, "221 Bye\r\n")
#define smtpsmsg_start_data()			dprintf(conn->clientsocket, "354\r\n")
#define smtpsmsg_reject_error()			dprintf(conn->clientsocket, "500 Bye\r\n")
#define smtpsmsg_reject_sequence()		dprintf(conn->clientsocket, "503 Invalid Sequence\r\n")
#define smtpsmsg_reject_too_long()		dprintf(conn->clientsocket, "500 Too long\r\n")
#define smtpsmsg_reject_too_large()		dprintf(conn->clientsocket, "554 5.3.4 Message Too Large\r\n")
#define smtpsmsg_reject_unrecognized()	dprintf(conn->clientsocket, "500 Command Unrecognized\r\n")
#define smtpsmsg_reject_unimplemented() dprintf(conn->clientsocket, "502 Command Unimplemented\r\n")
#define smtpsmsg_reject_starttls()		dprintf(conn->clientsocket, "523 Use SMTPS instead\r\n")

bool	ends_word(char);
bool	found_crlf_end(char *);
int parse_client_command(SMTPClientCommand *,
						 char *);
int		move_client_mail_domain(char *,
								Connection *,
								size_t);

#endif //SMTP_H
