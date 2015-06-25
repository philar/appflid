#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/comm/types.h"
#include "appflid/comm/log.h"
#include "appflid/comm/proto.h"
#include "appflid/mod/aproto.h"
#include "appflid/mod/nl_log.h"
#include "appflid/comm/print.h"

#define FILENAME "httpqq.conf"

MODULE_LICENSE("GPL");

static int hqq_send(const char *name, const struct tuple *tp, struct qq_info *hqq){
	return  nl_log_send_to_user(tp->l4num, hqq->version.all, tp->saddr, tp->daddr,
                                tp->sport, tp->dport, name, htonl(hqq->num), NULL, 0);
}

static void hqq_show(const struct qq_info *hqq)
{
	printk("version=%02x%02x, http qq num=%d\n",	hqq->version.v[0],
                                        			hqq->version.v[1],
                                        			hqq->num);
}

static int httpqq_handler(const char *name, const struct tuple *tp,
 		 		   		  const char *l4_data, const unsigned int data_len){
	struct qq_info hqq;
	char *patter  = ".qq.com";
	char *phost   = "host:";
	char *puin    = "uin=";
	char *pstart  = NULL;
	char *pend    = NULL;
	char qq[32]   = "";
	char *stop_at = NULL;
	int err       = 0;

	if(data_len <= 0){
		return -1;
	}

	if( (pstart = strstr(l4_data, patter)) == NULL){
		return 0;
	}

	/* uin=o0756354161, uin=756354161,  pt2gguin=o0756354161, clientuin=756354161 */
	if( (pstart = strstr(l4_data, puin)) != NULL){
		pstart = pstart + 4;
		// if(pstart[0] == 'o'){
		// 	pstart = pstart + 2;
		// }
		
		while(*pstart){
			if(*pstart < '1' && *pstart > '9')
				pstart++;
			else
				break;
		}
		pend = pstart;

		while(*pend){
			if(*pend >= '0' && *pend <= '9')
				pend++;
			else
				break;
		}
		memcpy(qq, pstart, pend - pstart);		
		if(strlen(qq) < 5){
			printk("http qq num error.\n");
			return -1;
		}

		printk("http qq num = %s\n", qq);
	}

	hqq.num = (uint32_t)simple_strtoul(qq, &stop_at, 0);
	if(hqq.num <= 10000){
		return -1;
	}
	hqq.version.v[0] = (unsigned char)0;
	hqq.version.v[1] = (unsigned char)0;

	hqq_show(&hqq);
	appflid_print_tuple(tp);

	err = hqq_send(name, tp, &hqq);
	if(err < 0){
		return err;
	}

	return 0;
}

struct aproto_node httpqq = {
	.handler = httpqq_handler,
}; 

static int __init httpqq_init(void){
	int err = -1;
	err = register_aproto(&httpqq, FILENAME);
	printk("httpqq_modules load, app proto name %s\n",httpqq.name);
	return err;
}

static void __exit httpqq_exit(void){
    unregister_aproto(&httpqq);
    printk("httpqq modules remove\n");
}

module_init(httpqq_init);
module_exit(httpqq_exit);