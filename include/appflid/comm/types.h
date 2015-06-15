#ifndef APPFLID_TYPES_H
#define APPFLID_TYPES_H

#include <linux/types.h>
#include <linux/version.h>
#ifdef __KERNEL__
        #include <linux/in.h>
#else
        #include <stdbool.h>
        #include <string.h>
        #include <arpa/inet.h>
#endif
#include <linux/netfilter.h>
#include <linux/errno.h>


#include "appflid/comm/appflid.h"

#ifdef __KERNEL__
    #define log_error(func, id, text, ...) func("%s: ERR%d (%s): " text "\n", MODULE_NAME, id, \
            __func__, ##__VA_ARGS__)
    #define log_informational(func, text, ...) func(text "\n", ##__VA_ARGS__)
#else
    #define log_error(func, id, text, ...) printf("ERR%d: " text "\n", id, ##__VA_ARGS__)
    #define log_informational(func, text, ...) printf(text "\n", ##__VA_ARGS__)
#endif

/** Messages to help us walk through a run. */
#define log_debug(text, ...)    log_informational(pr_debug, text, ##__VA_ARGS__)
/** "I'm dropping the packet and it's perfectly normal." */
#define log_info(text, ...)     log_informational(pr_info, text, ##__VA_ARGS__)
/** "I'm dropping the packet because it's corrupted." (i. e. nothing's wrong with the NAT64) */
#define log_warning(text, ...)  log_informational(pr_warning, text, ##__VA_ARGS__)
/** "I'm dropping the packet because the config's flipped out or a kmalloc failed." */
#define log_err(id, text, ...)  log_error(pr_err, id, text, ##__VA_ARGS__)
/** "I'm dropping the packet because I detected a programming error." */
#define log_crit(id, text, ...) log_error(pr_crit, id, text, ##__VA_ARGS__)


enum FLOW_DIR{
	    UP = 0 ,
	    DOWN =1 ,
};
enum APP_TYPE{
	    APP_HTTP_WEB = 1000,
	    APP_HTTP_VIDEO = 1001,

	    APP_INSTANT_CHAT = 2000,
	    APP_INSTANT_FILE = 2001,

};
enum ERROR_CODE {
		/* General */
	ERR_SUCCESS = 0,
	ERR_NULL = 1,
	ERR_ALLOC_FAILED = 2,
	ERR_OPEN_FILE = 3,
	ERR_UNKNOWN_ERROR = 4,
	ERR_OTHER = 5,
	ERR_SYSM = 6,
	ERR_REGISTER = 7,

	/*netlink*/
	ERR_PID = 4000,
	ERR_SOCKET = 4001,
	ERR_NLSEND_FAILED = 4002,
	ERR_NETLINK = 4003,

							
};

/*****************************/
/*
 * The parameters received from the user, formatted and ready to be read in any order.
 */
struct arguments {
	    __u16 key;
	    __u32 operation;
			    
};

/*
 * The flags the user can write as program parameters.
 */
enum argp_flags {
	    ARGP_ALL = 'A',
	    ARGP_COUNT = 'c',

	    ARGP_HTTP = 1000,
	    ARGP_DNS = 1001,

};


#endif

