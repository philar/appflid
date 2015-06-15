#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/comm/types.h"
#include "appflid/comm/log.h"
#include "appflid/comm/proto.h"
#include "appflid/mod/aproto.h"
#include "appflid/mod/nl_log.h"

#define FILENAME "qq.conf"

MODULE_LICENSE("GPL");

static int qq_send(const char *name,const struct tuple *tp ,struct qq_info *qq )
{
	struct log_packet *lp;
	int err;
	unsigned int datalen = sizeof(struct log_packet) + sizeof(struct qq_info);
	
	lp =  kmalloc(datalen, GFP_ATOMIC);
        if (!lp) {
                log_err(ERR_ALLOC_FAILED, "Allocation struct log  failed.");
                return -ENOMEM;
        }

	memset(lp,0,datalen);
	memcpy(lp->name,name,strlen(name));
	memcpy(&lp->tp,tp,sizeof(*tp));
	memcpy(lp->app_private,qq,sizeof(struct qq_info));
	err = nl_log_send_to_user(lp,datalen);
	kfree(lp);
	return err;
		
}

static void qq_show(const struct qq_info *qq)
{
	printk("version=%02x%02x,num=%d\n",qq->version[0],
                                           qq->version[1],
                                           ntohl(qq->num));

}

static int qq_header(const char *l4_data)
{
	int i;
	for(i = 0;i < 3;i++){
	      	if (l4_data[i] == 0x02)
                	return i;
	}
        return -1;
}

int qq_handler(const char *name,const struct tuple *tp,
 	      const char *l4_data,const unsigned int data_len)
{
	int head = -1;
	struct qq_info qq;

	printk("hello %s\n",__func__);
	if ((head = qq_header(l4_data)) < 0) {
        	return head;
        }

        if (l4_data[head + 1] == 0x03 && l4_data[head + 2] == 0x00) { /*version:0300*/
        	if (data_len < (11 + head))
                	return -1;
                memcpy(&qq.num, l4_data + head + 6, 4);
        }else {
                if(data_len < (12 + head))
                        return -1;
                memcpy(&qq.num, l4_data + head + 7, 4);
        }
	
	/* qq num len > 5 */
        if( ntohl(qq.num) <= 10000)
        	return -1;

	qq.version[0] = (unsigned char)l4_data[head + 1];
	qq.version[1] = (unsigned char)l4_data[head + 2];
        
        qq_show(&qq);	

	return  0;/*qq_send(name,tp,&qq);*/

}

struct aproto_node qq = {
	.handler = qq_handler,
}; 

static int __init  qq_init(void)
{
        int err;
        err = register_aproto(&qq,FILENAME);
        printk("qq_ modules load,app proto name %s\n",qq.name);
        return err;
}

static void __exit qq_exit(void)
{
        unregister_aproto(&qq);
        printk("qq modules remove\n");
}

module_init(qq_init);
module_exit(qq_exit);


