#ifndef APPFLID_PATTERN_H
#define APPFLID_PATTERN_H

#include <linux/list.h>
#include "appflid/comm/constants.h"
struct pattern_node{
	struct list_head list_hook;
	char app_proto[APPNAMSIZ];
	struct regexp *rgxp;

};
struct pattern_node *pattern_find(unsigned char *payload,unsigned int payload_len);
void pattern_show(void);
int pattern_add(char *_app_name,char *_str_rgxp);
int pattern_init(void);
void pattern_destroy(void);

#endif
