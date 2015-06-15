#ifndef APPFLID_NL_LOG_H
#define APPFLID_NL_LOG_H

int nl_log_send_to_user(const void* szdata, unsigned int len);
int nl_log_init(void);
void nl_log_destroy(void);

#endif


