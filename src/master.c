#include <stddef.h>
#include "femail.h"
#include "master.h"

void * start_master_service(void *) {
	log_info("Starting master service...");

	while(!is_stopped()) {
		
	}

	log_info("Master service stopped successfully.");

	return NULL;
}
