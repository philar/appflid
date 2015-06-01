#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>

#include <linux/netdevice.h>



#include "appflid/comm/appflid.h"
#include "appflid/comm/types.h"
#include "appflid/comm/constants.h"
#include "appflid/mod/core.h"
#include "appflid/mod/count.h"
#include "appflid/mod/wellkn_port.h"
#include "appflid/mod/pattern.h"
#include "appflid/mod/nl_kcmd.h"
#include "appflid/mod/config.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PHILAR_LAW");
MODULE_ALIAS("appflid");


static char *userif;
static char *resrcif;
static char *confiledir;
module_param(userif, charp, 0);  /*because no the /proc file so,the authority can be 0*/
module_param(resrcif, charp, 0);  
module_param(confiledir, charp, 0);  


static unsigned int
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35)
hook_v4(unsigned int hooknum, struct sk_buff *skb,
                      const struct net_device *in, const struct net_device *out,
                      int (*okfn)(struct sk_buff *)){
#else
hook_v4(const struct nf_hook_ops *ops, struct sk_buff *skb,
                      const struct net_device *in, const struct net_device *out,
                      int (*okfn)(struct sk_buff *)){

#endif
		return core(skb);
	
}
static struct nf_hook_ops appflid_ops[] __read_mostly = {
	{
                .hook           = hook_v4,
                .owner          = THIS_MODULE,
                .pf             = NFPROTO_IPV4,
                .hooknum        = NF_INET_POST_ROUTING,
                .priority       = NF_PRI_IP_APPFLID_LAST,
	},
	{
                .hook           = hook_v4,
                .owner          = THIS_MODULE,
                .pf             = NFPROTO_IPV4,
                .hooknum        = NF_INET_LOCAL_IN,
                .priority       = NF_PRI_IP_APPFLID_LAST,
	},
};
static void deinit(void){
	nl_kernel_destroy();
	pattern_destroy();
	wellkn_port_destroy();
	config_destroy();
}

static int __init appflid_init(void){
	int error;

	error=config_init(userif,resrcif,confiledir);
	if (error)
    		goto failure;

	error=count_init(); /*must init before wellkn_port and pattern*/
	if (error)
    		goto failure;

	error=wellkn_port_init();
	if (error)
    		goto failure;

	error=pattern_init();
	if (error)
    		goto failure;
	
	error=nl_kernel_init();
	if (error)
    		goto failure;

	error=nf_register_hooks(appflid_ops, ARRAY_SIZE(appflid_ops));
	if (error)
    		goto failure;

	log_info(MODULE_NAME " module inserted");
	return error;

failure:
	deinit();
	nf_unregister_hooks(appflid_ops, ARRAY_SIZE(appflid_ops));
	return error;
}
static void __exit appflid_exit(void){
	nf_unregister_hooks(appflid_ops, ARRAY_SIZE(appflid_ops));
	deinit();
	log_info(MODULE_NAME " module removed");
}

module_init(appflid_init);
module_exit(appflid_exit);

