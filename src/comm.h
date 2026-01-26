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

#endif //COMM_H
