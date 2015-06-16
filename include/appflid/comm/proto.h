#ifndef	 APPFLID_PROTO_H
#define	 APPFLID_PROTO_H

struct qq_info{
	union{
        	unsigned char   v[2];
		__u16           all;
	}version;
        uint32_t        num;
};

#endif
