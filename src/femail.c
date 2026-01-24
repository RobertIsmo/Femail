#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include "femail.h"
#include "comm.h"
#include "master.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef APP_NAME
#define APP_NAME ""
#error "APP_NAME not provided."
#endif

#ifndef VERSION_RELEASE
#define VERSION_RELEASE 0
#error "VERSION_RELEASE not provided."
#endif

#ifndef VERSION_UPDATE
#define VERSION_UPDATE 0
#error "VERSION_UPDATE not provided."
#endif

#ifndef VERSION_PATCH
#define VERSION_PATCH 0
#error "VERSION_PATCH not provided."
#endif

#ifndef VERSION_TAG
#define VERSION_TAG ""
#endif

char nameversion[NAMEVERSION_SIZE];
volatile sig_atomic_t stop_signal;
smtpcontext smtpctx;
smtpscontext smtpsctx;
starttlscontext starttlsctx;

void request_stop(int) {
	log_notice("Stop requested on master service.");
	stop_signal = 1;
}

int is_stopped(void) {
	return stop_signal;
}


int main() {
	snprintf(nameversion,
			 NAMEVERSION_SIZE,
			 "%s Version %d.%d.%d%s",
			 APP_NAME, VERSION_RELEASE, VERSION_UPDATE,
			 VERSION_PATCH, VERSION_TAG);

	openlog("femail",
			LOG_PID | LOG_CONS | LOG_NDELAY | LOG_PERROR,
			LOG_DAEMON);

	log_info("Starting Femail...");

	if (DEBUG) {
		log_warn("Running in debugging mode.");
	}
	
	if(start_smtp(&smtpctx) != 0) {
		log_crit("failed to start SMTP service.");
	}
	if(start_smtps(&smtpsctx) != 0) {
		log_crit("failed to start SMTPS service.");
	}
	if(start_starttls(&starttlsctx) != 0) {
		log_crit("failed to start STARTTLS service.");
	}

	check_communications(&smtpctx,
						 &smtpsctx,
						 &starttlsctx);

	signal(SIGINT,
		   request_stop);
	signal(SIGTERM,
		   request_stop);

	pthread_t mastert;

	if (pthread_create(&mastert,
					   NULL,
					   start_master_service,
					   NULL) != 0) {
		log_err("Unable to start master thread.");
		abort();
	}
	
	log_info("%s is currently running and operational.", nameversion);

	pthread_join(mastert, NULL);
	
	log_info("Stopping Femail...");

	stop_smtp(&smtpctx);
	stop_smtps(&smtpsctx);
	stop_starttls(&starttlsctx);

	log_info("Successfully stopped Femail.");
	closelog();
	return 0;
}
