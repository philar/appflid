#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/comm/types.h"
#include "appflid/comm/log.h"
#include "appflid/comm/proto.h"
#include "appflid/mod/aproto.h"
#include "appflid/mod/nl_log.h"
#include "appflid/comm/print.h"

#define FILENAME "youku_cdn.conf"

MODULE_LICENSE("GPL");

static int youku_cdn_send(const char *name, const struct tuple *tp, struct youku_cdn_info *yki){
	return nl_log_send_to_user(tp->l4num, 0, tp->saddr, tp->daddr,
                                    tp->sport, tp->dport, name, 0, NULL, 0);
}

static int youku_cdn_handler(const char *name, const struct tuple *tp,
				  const char *l4_data, const unsigned int data_len) {
	const char *pattern = "GET /youku/";
	struct youku_cdn_info yki;
	char *p = NULL;
	int err = -1;
	
	printk("hello %s\n",__func__);
	if(NULL == l4_data || 0 == data_len){
		return -1;
	}
	if( (p = strstr(l4_data, pattern)) != NULL){
	    //yki.cdn_ip = tp->daddr;
	
	    //appflid_print_tuple(tp);
	    //printk("youku_cdn cdn ip is: %pI4\n", &yki.cdn_ip);

	    err = youku_cdn_send(name, tp, &yki);
	    if(err < 0){
		   log_debug("youku_cdn_send to userspace faild.");
		   return err;
	    }
	}

	return 0;
}

struct aproto_node youku_cdn = {
	.handler = youku_cdn_handler,
};

static int __init youku_cdn_init(void){
	int err = 0;
	
	err = register_aproto(&youku_cdn, FILENAME);
	printk("youku_cdn modules load, app proto name is: %s\n", youku_cdn.name);
	
	return err;
}

static void __exit youku_cdn_exit(void){
	unregister_aproto(&youku_cdn);
	printk("youku_cdn modules remove.\n");
}

module_init(youku_cdn_init);
module_exit(youku_cdn_exit);
