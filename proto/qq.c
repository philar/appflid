#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/mod/aproto.h"

#define FILENAME "qq.conf"

MODULE_LICENSE("GPL");

int qq_handle(const struct sk_buff *skb)
{
	printk("hello %s\n",__func__);
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


