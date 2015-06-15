#ifndef APPFLID_LOG_H
#define APPFLID_LOG_H

#include "appflid/comm/constants.h"
struct tuple{
	__u32 saddr;
	__u32 daddr;
	__u16 sport;
	__u16 dport;
	__u8  l4num;
	
};
struct log_packet{
	char name[APPNAMSIZ];
	struct tuple tp; 
	char app_private[0] ;
};

#endif
