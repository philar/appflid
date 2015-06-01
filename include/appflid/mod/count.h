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


int  count_active(struct net *net);
int  count_add_proto(const char *app_proto);
int count_init(void);
void count_destroy(void);
#endif
