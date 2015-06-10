#ifndef APPFLID_APROTO_H
#define APPFLID_APROTO_H

#include <linux/skbuff.h>
#include <linux/list.h>
#include "appflid/comm/constants.h"
struct aproto_node{
	struct list_head list_hook;
	char name[APPNAMSIZ];
	struct regexp *rgxp;
	int (*handle)(const struct sk_buff *skb);

};
struct aproto_node *aproto_find(unsigned char *payload,unsigned int payload_len);
void aproto_show(void);
int aproto_add(struct aproto_node *and);
extern int register_aproto(struct aproto_node *and,const char *confile_name);
extern void unregister_aproto(struct aproto_node *and);
int aproto_init(void);
void aproto_destroy(void);

#endif
