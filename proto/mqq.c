#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/comm/types.h"
#include "appflid/comm/log.h"
#include "appflid/comm/proto.h"
#include "appflid/mod/aproto.h"
#include "appflid/mod/nl_log.h"
#include "appflid/comm/print.h"

#define FILENAME "mqq.conf"

MODULE_LICENSE("GPL");

static int mqq_send(const char *name, const struct tuple *tp, struct qq_info *mqq){
	return  nl_log_send_to_user(tp->l4num, mqq->version.all, tp->saddr, tp->daddr,
                                tp->sport, tp->dport, name, htonl(mqq->num), NULL, 0);
}

static void mqq_show(const struct qq_info *mqq)
{
	printk("version=%02x%02x, mobileqq num=%u\n",	mqq->version.v[0],
                                        			mqq->version.v[1],
                                        			mqq->num);
}

static int mqq_handler(const char *name, const struct tuple *tp,
 	 		    const char *l4_data, const unsigned int data_len){
	struct qq_info mqq;
	char qq[32] = "";
	uint16_t qqlen = 0;
	int err = -1;
	char *stop_at = NULL;

	printk("hello %s\n",__func__);

    if(tp->l4num != 6){
        return -1;
    }

	if(data_len < 9){
		return -1;
	}

	/* 0x00 0x00 */
	if( l4_data[0] != 0x00 && l4_data[1] != 0x00 &&
        l4_data[4] != 0x00 && l4_data[5] != 0x00 &&
        l4_data[6] != 0x00 ){
        return -1;
    }
    /* get qq len */
    unsigned char zerobytes[4] = {0x00, 0x00, 0x00, 0x00};
    int n = 9;
    while(n++){
    	if( memcmp(l4_data+n, zerobytes, 4) == 0){
    		break;
    	}
    	if(n > (data_len - 4)){
    		return -1;
    	}
    }
    memcpy(&qqlen, l4_data+n+3, 2);
    qqlen = ntohs(qqlen);
    if(qqlen > 18 || qqlen < 5){
    	return -1;
    }

    memcpy(qq, l4_data+n+5, qqlen-4);
    // printk("qqlen=%d, mobileqq=%s\n", qqlen-4, qq);
    if(strlen(qq) < 5){
        return -1;
    }

    mqq.num = (uint32_t)simple_strtoul(qq, &stop_at, 0);
    if(mqq.num <= 10000){
    	return -1;
    }
    mqq.version.v[0] = (unsigned char)l4_data[7];
	mqq.version.v[1] = (unsigned char)l4_data[8];

	mqq_show(&mqq);
	//appflid_print_tuple(tp);

	err = mqq_send(name, tp, &mqq);
	if(err < 0){
		return -1;
	}
	return 0;
}

struct aproto_node mqq = {
	.handler = mqq_handler,
}; 

static int __init mqq_init(void){
	int err = -1;
	err = register_aproto(&mqq, FILENAME);
	printk("mqq_modules load, app proto name %s\n",mqq.name);
	return err;
}

static void __exit mqq_exit(void){
    unregister_aproto(&mqq);
    printk("mqq modules remove\n");
}

module_init(mqq_init);
module_exit(mqq_exit);
