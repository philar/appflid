#ifndef APPFLID_NL_LOG_H
#define APPFLID_NL_LOG_H

//int nl_log_send_to_user(const void* szdata, unsigned int len);
int nl_log_send_to_user(u_int8_t proto,u_int16_t version,
                        u_int32_t src,u_int32_t dst,u_int16_t sport,
                        u_int16_t dport,const char *name,u_int32_t app_id,const char *reserve,u_int16_t rlen);
int nl_log_init(void);
void nl_log_destroy(void);

#endif


