#ifndef MASTER_H
#define MASTER_H

int get_accept_state(int);
int handle_message(int);
int handle_submission(int);
int handle_reject_starttls(int);
void * start_master_service(void *);

#endif //MASTER_H
