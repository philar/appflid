#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "appflid/mod/aproto.h"

#define FILENAME "mqq.conf"

MODULE_LICENSE("GPL");

void mqq_show(const struct nf_conn *ct)
{

}

int mqq_handle(struct nf_conn *ct,
               const char *l4_data, 
               const unsigned int data_len)
{
	printk("hello %s\n",__func__);
	return 0;

}

struct aproto_node mqq = {
	.handler = mqq_handle,
	.show = mqq_show
}; 

static int __init  mqq_init(void)
{
        int err;
        err = register_aproto(&mqq,FILENAME);
        printk("mqq_ modules load,app proto name %s\n",mqq.name);
        return err;
}

static void __exit mqq_exit(void)
{
        unregister_aproto(&mqq);
        printk("mqq modules remove\n");
}

module_init(mqq_init);
module_exit(mqq_exit);


