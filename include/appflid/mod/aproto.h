#ifndef APPFLID_APROTO_H
#define APPFLID_APROTO_H

#include <linux/skbuff.h>
#include <linux/list.h>
#include <net/netfilter/nf_conntrack.h>
#include "appflid/comm/constants.h"
struct aproto_node{
	struct list_head list_hook;
	char name[APPNAMSIZ];
	struct regexp *rgxp;
	int (*handle)(struct nf_conn *ct,const char *l4_data, const unsigned int data_len);
	void (*show)(const struct nf_conn *ct);

};
struct aproto_node *aproto_find(unsigned char *payload,unsigned int payload_len);
void aproto_show(void);
int aproto_add(struct aproto_node *and);
extern int register_aproto(struct aproto_node *and,const char *confile_name);
extern void unregister_aproto(struct aproto_node *and);
int aproto_init(void);
void aproto_destroy(void);

#endif
