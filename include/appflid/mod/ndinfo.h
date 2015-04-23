#ifndef APPFLID_NDINFO_H
#define APPFLID_NDINFO_H

#include "appflid/comm/constants.h"

struct ndinfo_entry{
	struct rb_node tree_hook;
 	struct in_addr addr;
	__u16 port;
	long bytes[2];
	char app_name[APPNAMSIZ];
//	int  app_type;
};
struct ndinfo_key{
	struct in_addr addr;
	__u16 port;
};
struct ndinfo_table{
	struct rb_root tree;
	long count;
};
struct ndinfo_entry * ndinfo_find(struct ndinfo_key *key);
int ndinfo_add(const struct in_addr *addr ,const __u16 port,const long up_bytes,const char * app_name/*,const int app_type*/);
void ndinfo_show(void);
int ndinfo_update(struct ndinfo_entry * ndinfo,int dir ,long bytes);

void ndinfo_count(const char *app_name,long *up,long * down);

int ndinfo_init(void);
void ndinfo_destroy(void);

#endif
