/* I do not want to sully the whole system,
 * so if necessary,I try my best to save something 
 * about this function in one file ,this file .
 * this file is not what I want ,just for suits to the 
 * userspace process,what I want is in the master branch.
 * I can NOT bear such ugly code
 */

#include <linux/netlink.h>
#include <linux/version.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>

#include "appflid/comm/types.h"
#include "appflid/mod/nl_log.h"


#define APPFLID_NETLINK_LOG 31
#define APP_MAXNLGROUPS 32              /* numer of nlgroups */
#define  nlgroupnum 1                  /* group number */
#define MAX_MSGSIZE 1024

typedef struct {
        u_int16_t       len;
        u_int8_t        proto;
        u_int16_t       version;
        u_int32_t       src;
        u_int32_t       dst;
        u_int16_t       sport;
        u_int16_t       dport;
        char            apptype[16];   
        u_int32_t       app_id; /* QQ/Weixin id */
        char            reserve[0];
} app_info_t;


static struct sock *nl_sk=NULL;
//int pid;


/* maybe this API should be :
 * int nl_log_send_to_user(const app_info_t *app_info,unsigned int len)
 * but I do NOT want to reference the struct app_info_t outside this file
 */

int nl_log_send_to_user(u_int8_t proto,u_int16_t version,
			u_int32_t src,u_int32_t dst,u_int16_t sport,
			u_int16_t dport,const char *name,u_int32_t app_id,const char *reserve,u_int16_t rlen)
{

        struct sk_buff *skb;
        struct nlmsghdr *nlhdr;
	app_info_t *app_info;
	unsigned int len = rlen + sizeof(app_info_t);

        skb = nlmsg_new(NLMSG_ALIGN(len), GFP_ATOMIC);
        if (!skb) {
                log_err(ERR_ALLOC_FAILED, "Failed to allocate a  skb to send log messages the user.");
                return -ENOMEM;
        }

        nlhdr=nlmsg_put(skb,0,0,NLMSG_DONE,len,0);  
	
 	app_info = kmalloc(len, GFP_ATOMIC); //malloc(sendmsg_size);
                if (!app_info){
                        log_err(ERR_ALLOC_FAILED ,"Failed to allocate app_info struct data");
                        return -ENOMEM;
        }	
	
	memset(app_info,0,len);
	app_info->proto = proto;
	app_info->src = src;
	app_info->dst = dst;
	app_info->sport = sport;
	app_info->dport = dport;
	memcpy(app_info->apptype,name,strlen(name));

	if (rlen) {
		app_info->len = rlen;
		memcpy(app_info->reserve,reserve,rlen);
	}

	if (version)
		app_info->version = version;	
	
	if (app_id)
		app_info->app_id = app_id;

        memcpy(nlmsg_data(nlhdr), app_info, len);

        nlhdr->nlmsg_len = skb->tail;
        nlhdr->nlmsg_pid = 0;
        nlhdr->nlmsg_flags = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
        NETLINK_CB(skb).pid = 0;
#else 
        NETLINK_CB(skb).portid = 0;
#endif
        NETLINK_CB(skb).dst_group = 5; /*multicast number*/

/*        return  netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);*/
	return netlink_broadcast(nl_sk, skb, 0,5, GFP_ATOMIC); 
        
}
EXPORT_SYMBOL(nl_log_send_to_user);

/*just get pid from user program*/
static void nl_log_recv_msg(struct sk_buff *skb){

/*need to make sure the process in userspace is running*/

/*	struct nlmsghdr *nlh;
	nlh=(struct nlmsghdr *)skb->data;
        if (nlh->nlmsg_type != MSG_CONF) {
                log_debug("Expecting %#x but got %#x.", MSG_CONF, nlh->nlmsg_type);
                return ;
        }
	pid=nlh->nlmsg_pid;*/
        
}

int nl_log_init(void){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
	nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK_LOG, 0, nl_log_recv_msg,
			NULL, THIS_MODULE);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	struct netlink_kernel_cfg nl_cfg = { .input  = nl_log_recv_msg };
	nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, THIS_MODULE, &nl_cfg);
#else
	struct netlink_kernel_cfg nl_cfg = { .input  = nl_log_recv_msg };
	nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK_LOG, &nl_cfg);
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


