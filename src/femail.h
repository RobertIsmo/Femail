#ifndef FEMAIL_H
#define FEMAIL_H

#include <stdbool.h>
#include <syslog.h>
#include <time.h>
#include <signal.h>
#include <openssl/ssl.h>
#include "comm.h"

#define NAMEVERSION_SIZE	1024
#define DOMAIN_MAX_SIZE		255
#define SMALL_BUFFER_SIZE	1024

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define LOG_LOCATION __FILE__ ":" TOSTRING(__LINE__)

#define log_base(level, fmt, ...)				\
    syslog(level, fmt, ##__VA_ARGS__)

#define log_emerg(fmt, ...)  log_base(LOG_EMERG,  "EMERG %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#define log_alert(fmt, ...)  log_base(LOG_ALERT,  "ALERT %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#define log_crit(fmt, ...)   log_base(LOG_CRIT,   "CRIT %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#define log_err(fmt, ...)    log_base(LOG_ERR,    "ERROR %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#define log_warn(fmt, ...)   log_base(LOG_WARNING,"WARN %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#define log_notice(fmt, ...) log_base(LOG_NOTICE, "NOTICE %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#define log_info(fmt, ...)   log_base(LOG_INFO,   "INFO %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#ifdef DEBUG
#define log_debug(fmt, ...)  log_base(LOG_DEBUG,  "DEBUG %s: " fmt, LOG_LOCATION, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

typedef enum {
	CONNECTION_DONE,
	CONNECTION_CONTINUE,
	CONNECTION_RESET,
	CONNECTION_ERROR,
} ConnectionHandlerResult;

typedef enum {
	SMTP_CONNECTION,
	SMTPS_CONNECTION,
	STARTTLS_CONNECTION,
	HTTP_CONNECTION,
	HTTPS_CONNECTION,
} ConnectionType;

typedef enum {
	MAIL_CONNECTION_OPENED,
	MAIL_CONNECTION_EXPECT_ANY,
	MAIL_CONNECTION_GREETED,
	MAIL_CONNECTION_MAILING,
	MAIL_CONNECTION_RCPT,
	MAIL_CONNECTION_EXPECT_DATA,
	HTTP_CONNECTION_OPENED
} ConnectionState;

typedef enum {
	NO_SSL,
	OPPORTUNISTIC_TLS,
	IMPLICIT_TLS
} SSL_STRATEGY;

typedef struct {
	time_t			timeinitialized;
	time_t			timerefreshed;
	bool			live;
	ConnectionType	type;
	ConnectionState state;
	int				clientsocket;
	SSL_STRATEGY	sslstrat;
	SSL *			sslconn;
	char			mailclientdomain[DOMAIN_MAX_SIZE];
	size_t			alloccount;
	size_t			messagebuffersize;
	size_t			messagebufferend;
	char *			messagebuffer;
} Connection;


void	request_stop(int);
int		is_stopped(void);

int	    dns_init(void);
char *  get_mail_domain(void);

void	init_smtp(smtpcontext *);
void	init_smtps(smtpscontext *);
void	init_starttls(starttlscontext *);
void	init_http(httpcontext *);
void	init_https(httpscontext *);

int	start_smtp(smtpcontext *);
int	start_smtps(smtpscontext *);
int	start_starttls(starttlscontext *);
int	start_http(httpcontext *);
int	start_https(httpscontext *);

void check_communications(smtpcontext *,
						  smtpscontext *,
						  starttlscontext *);

void	stop_smtp(smtpcontext *);
void	stop_smtps(smtpscontext *);
void	stop_starttls(starttlscontext *);
void	stop_http(httpcontext *);
void	stop_https(httpscontext *);

ConnectionHandlerResult smtp_handler(Connection *);
ConnectionHandlerResult smtps_handler(Connection *);
ConnectionHandlerResult starttls_handler(Connection *);
ConnectionHandlerResult http_handler(Connection *);
ConnectionHandlerResult https_handler(Connection *);

void *	start_master_service(void *);

extern char						nameversion[NAMEVERSION_SIZE];
extern volatile sig_atomic_t	stop_signal;

extern smtpcontext	smtpctx;
extern smtpscontext smtpsctx;
extern starttlscontext starttlsctx;
extern httpcontext httpctx;
extern httpscontext httpsctx;

extern SSL_CTX * sslctx;

#endif //FEMAIL_H
