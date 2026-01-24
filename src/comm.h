#ifndef COMM_H
#define COMM_H

#include <stdbool.h>
#include <sys/socket.h>

#define SMTP_PORT 25
#define SMTPS_PORT 465
#define STARTTLS_PORT 587

typedef struct {
	bool active;
	int socket4;
	int socket6;
} smtpcontext;
typedef struct {
	bool active;
	int socket4;
	int socket6;
} smtpscontext;
typedef struct {
	bool active;
	int socket4;
	int socket6;
} starttlscontext;

void init_smtp(smtpcontext *);
void init_smtps(smtpscontext *);
void init_starttls(starttlscontext *);

int start_smtp(smtpcontext *);
int start_smtps(smtpscontext *);
int start_starttls(starttlscontext *);

void check_communications(smtpcontext *,
						  smtpscontext *,
						  starttlscontext *);

void stop_smtp(smtpcontext *);
void stop_smtps(smtpscontext *);
void stop_starttls(starttlscontext *);

extern smtpcontext smtpctx;
extern smtpscontext smtpsctx;
extern starttlscontext starttlsctx;

#endif //COMM_H
