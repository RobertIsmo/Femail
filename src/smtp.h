#ifndef SMTP_H
#define SMTP_H

#include "femail.h"

#define ESMTP_SIZE			10485760

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
	SMTP_EXPN,
	SMTP_STARTTLS
} SMTPCommand;

typedef struct {
	SMTPCommand command;
	char * param;
} SMTPClientCommand;

#define smtp_send_msg(connection, fmt, ...)		\
	do {										\
		if (conn->sslconn != nullptr) {			\
			char msgbuf[256];					\
			int len = snprintf(msgbuf,			\
							   sizeof(msgbuf),	\
							   fmt,				\
							   ##__VA_ARGS__);	\
			if (len > 0)						\
				SSL_write(connection->sslconn,	\
						  msgbuf,				\
						  len);					\
		} else {								\
			dprintf(connection->clientsocket,	\
					fmt,						\
					##__VA_ARGS__);				\
		}										\
	} while(0);


#define smtpsmsg_accept_connection()	smtp_send_msg(conn, "220 %s ESMTP Femail\r\n", get_mail_domain())
#define smtpsmsg_accept_hello()			smtp_send_msg(conn, "250 Hello %s\r\n", conn->mailclientdomain)
#define smtpsmsg_accept_ehello()		smtp_send_msg(conn,	\
												"250-%s Hello %s\n"	\
												"250-SIZE %d\n"	\
												"250-STARTTLS\n"	\
												"250 HELP\r\n",	\
												get_mail_domain(),	\
												conn->mailclientdomain,	\
												ESMTP_SIZE)
#define smtpsmsg_accept_ok()			smtp_send_msg(conn, "250 Ok\r\n")
#define smtpsmsg_accept_goodbye()		smtp_send_msg(conn, "221 Bye\r\n")
#define smtpsmsg_accept_starttls()		smtp_send_msg(conn, "220 Go ahead\r\n")
#define smtpsmsg_start_data()			smtp_send_msg(conn, "354\r\n")
#define smtpsmsg_reject_error()			smtp_send_msg(conn, "500 Bye\r\n")
#define smtpsmsg_reject_sequence()		smtp_send_msg(conn, "503 Invalid Sequence\r\n")
#define smtpsmsg_reject_too_long()		smtp_send_msg(conn, "500 Too long\r\n")
#define smtpsmsg_reject_too_large()		smtp_send_msg(conn, "554 5.3.4 Message Too Large\r\n")
#define smtpsmsg_reject_unrecognized()	smtp_send_msg(conn, "500 Command Unrecognized\r\n")
#define smtpsmsg_reject_unimplemented() smtp_send_msg(conn, "502 Command Unimplemented\r\n")
#define smtpsmsg_reject_starttls()		smtp_send_msg(conn, "523 Use SMTPS instead\r\n")

bool	ends_word(char);
bool	found_crlf_end(char *);
int parse_client_command(SMTPClientCommand [static 1],
						 char *);
int		move_client_mail_domain(char *,
								Connection [static 1],
								size_t);
ssize_t smtp_receive_msg(Connection [static 1],
						 char *,
						 int);
#endif //SMTP_H
