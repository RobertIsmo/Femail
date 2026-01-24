#ifndef FEMAIL_H
#define FEMAIL_H

#include <syslog.h>
#include <signal.h>
#include "comm.h"

#define NAMEVERSION_SIZE 1024

#define log_base(level, fmt, ...) \
    syslog(level, fmt, ##__VA_ARGS__)

#define log_emerg(fmt, ...)  log_base(LOG_EMERG,  "EMERG: "  fmt, ##__VA_ARGS__)
#define log_alert(fmt, ...)  log_base(LOG_ALERT,  "ALERT: "  fmt, ##__VA_ARGS__)
#define log_crit(fmt, ...)   log_base(LOG_CRIT,   "CRIT: "   fmt, ##__VA_ARGS__)
#define log_err(fmt, ...)    log_base(LOG_ERR,    "ERROR: "  fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)   log_base(LOG_WARNING,"WARN: "   fmt, ##__VA_ARGS__)
#define log_notice(fmt, ...) log_base(LOG_NOTICE, "NOTICE: " fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)   log_base(LOG_INFO,   fmt,       ##__VA_ARGS__)
#define log_debug(fmt, ...)  log_base(LOG_DEBUG,  "DEBUG: "  fmt, ##__VA_ARGS__)

void request_stop(int);
int is_stopped(void);

extern char nameversion[NAMEVERSION_SIZE];
extern volatile sig_atomic_t stop_signal;

#endif //FEMAIL_H
