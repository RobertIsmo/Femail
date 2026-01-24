#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "femail.h"
#include "dns.h"

static char maildomain[DOMAIN_MAX_SIZE] = {0};

int dns_init(void) {
	char * domain = getenv("MAIL_DOMAIN");
	unsigned long domainlen = strlen(domain);
	if (domainlen == 0) {
		log_warn("Cannot find a valid mail domain.");
		return 1;
	} else {		
		snprintf(maildomain,
				 domainlen,
				 "%s",
				 domain);
	}
} 
