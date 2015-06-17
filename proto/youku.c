#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/comm/types.h"
#include "appflid/comm/log.h"
#include "appflid/comm/proto.h"
#include "appflid/mod/aproto.h"
#include "appflid/mod/nl_log.h"

#define FILENAME "youku.conf"

MODULE_LICENSE("GPL");

int youku_send(const char *name, const struct tuple *tp, struct youku_info *yki){
	struct log_packet *lp = NULL;
	int err = 0;
	unsigned int datalen = 0;
	
	datalen = sizeof(struct log_packet) + sizeof(struct youku_info);
	lp =  kmalloc(datalen, GFP_ATOMIC);
	if (!lp) {
		log_err(ERR_ALLOC_FAILED, "Allocation struct log  failed.");
		return -ENOMEM;
	}
	
	memset(lp, 0, datalen);
	memcpy(lp->name, name, strlen(name));
	memcpy(&lp->tp, tp, sizeof(struct tuple));
	memcpy(lp->app_private, yki, sizeof(struct youku_info));
	
	err = nl_log_send_to_user(lp, datalen);
	
	kfree(lp);
	return 0;
}

int youku_handler(const char *name, const struct tuple *tp,
				  const char *l4_data, const unsigned int data_len) {
	const char *pattern = "GET /youku/";
	struct youku_info yki;
	char *p = NULL;
	
	printk("hello %s\n",__func__);
	if(NULL == l4_data || 0 == data_len){
		return -1;
	}
	if( (p = strstr(l4_data, pattern)) != NULL){
		yki.cdn_ip = tp->daddr;
	}
	printk("youku cdn ip is: %d\n", yki.cdn_ip);
	
	return 0;
}

struct aproto_node youku = {
	.handler = youku_handler,
};

static int __init youku_init(void){
	int err = 0;
	
	err = register_aproto(&youku, FILENAME);
	printk("youku modules load, app proto name is: %s\n", youku.name);
	
	return err;
}

static void __exit youku_exit(void){
	unregister_aproto(&youku);
	printk("youku modules remove.\n");
}

module_init(youku_init);
module_exit(youku_exit);
