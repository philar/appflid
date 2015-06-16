#include <linux/netlink.h>
#include <linux/version.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>

#include "appflid/comm/types.h"
#include "appflid/comm/constants.h"
#include "appflid/mod/nl_log.h"

struct sock *nl_sk=NULL;
int pid;
static DEFINE_MUTEX(my_mutex);

int nl_log_send_to_user(const void* szdata, unsigned int len)
{

        struct sk_buff *skb;
        struct nlmsghdr *nlhdr;
	int err = -1;

        skb = nlmsg_new(NLMSG_ALIGN(len), GFP_ATOMIC);
        if (!skb) {
                log_err(ERR_ALLOC_FAILED, "Failed to allocate a  skb to send log messages the user.");
                return -ENOMEM;
        }

        nlhdr=nlmsg_put(skb,0,0,NLMSG_DONE,len,0);  
        memcpy(nlmsg_data(nlhdr), szdata, len);

        nlhdr->nlmsg_len = skb->tail;
        nlhdr->nlmsg_pid = 0;
        nlhdr->nlmsg_flags = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
        NETLINK_CB(skb).pid = 0;
#else 
        NETLINK_CB(skb).portid = 0;
#endif
        NETLINK_CB(skb).dst_group = 0;

	mutex_lock(&my_mutex);
        err = netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
	mutex_unlock(&my_mutex);
	return err;
        
}
EXPORT_SYMBOL(nl_log_send_to_user);

/*just get pid from user program*/
static void nl_log_recv_msg(struct sk_buff *skb){
       struct nlmsghdr *nlh;
       nlh=(struct nlmsghdr *)skb->data;
       if (nlh->nlmsg_type != MSG_CONF) {
                log_debug("Expecting %#x but got %#x.", MSG_CONF, nlh->nlmsg_type);
                return ;
       }
       pid=nlh->nlmsg_pid;
}

int nl_log_init(void){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
	nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, 0, nl_log_recv_msg,
			NULL, THIS_MODULE);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	struct netlink_kernel_cfg nl_cfg = { .input  = nl_log_recv_msg };
	nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, THIS_MODULE, &nl_cfg);
#else
	struct netlink_kernel_cfg nl_cfg = { .input  = nl_log_recv_msg };
	nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, &nl_cfg);
#endif
    if(!nl_sk){
                log_err(ERR_NETLINK, "Creation of netlink socket failed.");
                return -EINVAL;
    }
    log_debug("Netlink socket created.");

    return 0;
}

void nl_log_destroy(void){
	netlink_kernel_release(nl_sk);
}


