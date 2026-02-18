#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include "femail.h"

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

smtpcontext		smtpctx;
smtpscontext	smtpsctx;
starttlscontext starttlsctx;
httpcontext		httpctx;
httpscontext	httpsctx;

SSL_CTX * sslctx = NULL;

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

	if (dns_init() != 0) {
		log_err("No MAIL_DOMAIN environment variable provided.");
		return 1;
	} else {
		log_info("Running as %s.", get_mail_domain());
	}

	sslctx = SSL_CTX_new(TLS_server_method());
	uint64_t sslopts = SSL_OP_NO_RENEGOTIATION | SSL_OP_CIPHER_SERVER_PREFERENCE;
	SSL_CTX_set_options(sslctx,
						sslopts);

	char certfile[256] = {0};
	char privfile[256] = {0};
	if (sprintf(certfile,
				"/usr/local/share/ca-certificates/%s.crt",
				get_mail_domain()) < 0) {
		log_err("Error formatting certificate file.");
	}
	if (sprintf(privfile,
				"/usr/local/share/ca-certificates/%s.key",
				get_mail_domain()) < 0) {
		log_err("Error formatting private key file.");
	}

	log_debug("SSL from %s and %s.", certfile, privfile);
	
	if (SSL_CTX_use_certificate_chain_file(sslctx,
										   certfile) <= 0) {
		SSL_CTX_free(sslctx);
		sslctx = NULL;
		log_err("Failed to load the server certificate chain file");
	}
	if (SSL_CTX_use_PrivateKey_file(sslctx,
									privfile,
									SSL_FILETYPE_PEM) <= 0) {
		SSL_CTX_free(sslctx);
		sslctx = NULL;
		log_err("Failed to load the server private key file");
	}

	// TODO: this should be randomized
	char cache_id[] = "ABCDEFG";
	if (sslctx != NULL) {
		SSL_CTX_set_session_id_context(sslctx,
									   (void *)cache_id,
									   sizeof(cache_id));
		SSL_CTX_set_session_cache_mode(sslctx,
									   SSL_SESS_CACHE_SERVER);
		SSL_CTX_sess_set_cache_size(sslctx,
									SSL_SESSION_CACHE_MAX_SIZE_DEFAULT);
		SSL_CTX_set_timeout(sslctx,
							3600);
		SSL_CTX_set_verify(sslctx,
						   SSL_VERIFY_NONE,
						   NULL);
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
	if(start_http(&httpctx) != 0) {
		log_err("failed to start HTTP service.");
	}
	if(start_https(&httpsctx) != 0) {
		log_err("failed to start HTTPS service.");
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
	stop_http(&httpctx);
	stop_https(&httpsctx);

	SSL_CTX_free(sslctx);

	log_info("Successfully stopped Femail.");
	closelog();
	return 0;
}
