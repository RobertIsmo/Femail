#ifndef FEMAIL_H
#define FEMAIL_H

#include	<syslog.h>
#include <signal.h>
#include "comm.h"

#define NAMEVERSION_SIZE	1024
#define DOMAIN_MAX_SIZE		255

#define log_base(level, fmt, ...)				\
    syslog(level, fmt, ##__VA_ARGS__)

#define log_emerg(fmt, ...)  log_base(LOG_EMERG,  "EMERG: "  fmt, ##__VA_ARGS__)
#define log_alert(fmt, ...)  log_base(LOG_ALERT,  "ALERT: "  fmt, ##__VA_ARGS__)
#define log_crit(fmt, ...)   log_base(LOG_CRIT,   "CRIT: "   fmt, ##__VA_ARGS__)
#define log_err(fmt, ...)    log_base(LOG_ERR,    "ERROR: "  fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)   log_base(LOG_WARNING,"WARN: "   fmt, ##__VA_ARGS__)
#define log_notice(fmt, ...) log_base(LOG_NOTICE, "NOTICE: " fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)   log_base(LOG_INFO,   fmt,       ##__VA_ARGS__)
#ifdef DEBUG
#define log_debug(fmt, ...)  log_base(LOG_DEBUG,  "DEBUG: "  fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

typedef enum {
	CONNECTION_DONE,
	CONNECTION_CONTINUE,
	CONNECTION_ERROR,
} ConnectionHandlerResult;

typedef enum {
	SMTP_CONNECTION,
	SMTPS_CONNECTION,
	STARTTLS_CONNECTION
} ConnectionType;

typedef enum {
	MAIL_CONNECTION_OPENED,
	MAIL_CONNECTION_EXPECT_HELLO
} ConnectionState;

typedef struct {
	ConnectionType	type;
	ConnectionState state;
	int				clientsocket;
} Connection;


void	request_stop(int);
int		is_stopped(void);

int	    dns_init(void);
char *  get_mail_domain(void);

void	init_smtp(smtpcontext *);
void	init_smtps(smtpscontext *);
void	init_starttls(starttlscontext *);

int start_smtp(smtpcontext *);
int start_smtps(smtpscontext *);
int start_starttls(starttlscontext *);

void check_communications(smtpcontext *,
						  smtpscontext *,
						  starttlscontext *);

void	stop_smtp(smtpcontext *);
void	stop_smtps(smtpscontext *);
void	stop_starttls(starttlscontext *);

ConnectionHandlerResult smtp_handler(Connection *);
ConnectionHandlerResult smtps_handler(Connection *);
ConnectionHandlerResult starttls_handler(Connection *);

void *	start_master_service(void *);

extern char						nameversion[NAMEVERSION_SIZE];
extern volatile sig_atomic_t	stop_signal;

extern smtpcontext	smtpctx;
extern smtpscontext smtpsctx;
extern starttlscontext starttlsctx;

#endif //FEMAIL_H
