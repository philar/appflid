#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/mod/aproto.h"

#define FILENAME "qq.conf"

MODULE_LICENSE("GPL");

struct qq_info{
	unsigned char   version[2];    
        uint32_t        num;
};

static int qq_header(const char *l4_data)
{
	int i;
	for(i = 0;i < 3;i++){
	      	if (l4_data[i] == 0x02)
                	return i;
	}
        return -1;
}

int qq_handle(struct nf_conn *ct,
 	      const char *l4_data, 
              const unsigned int data_len)
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
	
	ct->appflid.app_private = kmalloc(sizeof(struct qq_info), GFP_ATOMIC);
        if (!ct->appflid.app_private) {
                printk("Allocation ct->appflid.app_private  failed.");
                return -ENOMEM;
        }
	
	memset(ct->appflid.app_private,0,sizeof(struct qq_info));
	memcpy(ct->appflid.app_private,&qq,sizeof(struct qq_info));

	return 0;

}

struct aproto_node qq = {
	.handle = qq_handle,
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


