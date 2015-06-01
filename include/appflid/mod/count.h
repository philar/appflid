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


int  nf_ct_appflid_count(struct net *net,struct app_counter *app_cnt);
int  count_add_proto(const char *proto);
int count_init(void);
void count_destroy(void);
#endif
