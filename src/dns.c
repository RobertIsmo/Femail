#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "femail.h"
#include "dns.h"

static char maildomain[DOMAIN_MAX_SIZE] = {0};

char * get_mail_domain(void) {
	return maildomain;
}

int dns_init(void) {
	char * domain = getenv("MAIL_DOMAIN");
	if (domain == NULL) {
		log_warn("Cannot find a valid mail domain.");
		return 1;
	}
	
	unsigned long domainlen = strlen(domain);
	if (domainlen == 0) {
		log_warn("Cannot find a valid mail domain.");
		return 1;
	} else if (domainlen > DOMAIN_MAX_SIZE - 1) {
		log_warn("Mail domain too long.");
		return 1;
	} else {		
		snprintf(maildomain,
				 DOMAIN_MAX_SIZE,
				 "%s",
				 domain);
		return 0;
	}
}
