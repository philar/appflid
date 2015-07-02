#ifndef APPFLID_COUNT_H
#define APPFLID_COUNT_H

#include <uapi/linux/netfilter/nf_conntrack_tuple_common.h>
#include "appflid/comm/constants.h"

#define MAX_PROTO 255


struct app_counter{
	char app_proto[APPNAMSIZ];
	atomic64_t packets[IP_CT_DIR_MAX];
	atomic64_t bytes[IP_CT_DIR_MAX];
};


int  count_add_proto(const char *app_proto);
int  count_remove_proto(const char *app_proto);
int count_total(struct net *net,char *buf,size_t buf_len);
void flush_active(void);
void flush_inactive(void);
int count_init(void);
void count_destroy(void);
#endif
