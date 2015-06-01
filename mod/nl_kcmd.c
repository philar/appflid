#include <linux/netlink.h>
#include <linux/version.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>

#include "appflid/comm/constants.h"
#include "appflid/comm/types.h"
#include "appflid/mod/nl_kcmd.h"
#include "appflid/mod/ndinfo.h"
#include "appflid/mod/count.h"

struct sock *nl_sk=NULL;
static DEFINE_MUTEX(nl_mtx);


static int  nl_send_to_user(int pid,char *data,int data_len){
	struct sk_buff *skb=NULL;
        struct nlmsghdr *nl_hdr=NULL;
        int res;	

	if(pid<=0){
		log_err(ERR_PID,"the pid  %d is not available",pid);
		return -1;
	}
		
    skb = nlmsg_new(NLMSG_ALIGN(data_len), GFP_ATOMIC);
    if (!skb) {
            log_err(ERR_ALLOC_FAILED, "%s  allocate a response skb to the user failed.",__func__);
                return -1;
	}
    nl_hdr = nlmsg_put(skb,
                     0, /* src_pid (0 = kernel) */
                     0, /* seq */
                     0, /* type */
                     data_len, /* payload len */
                     NLM_F_MULTI); /* flags. */

	NETLINK_CB(skb).dst_group = 0; /* not in mcast group */
    NETLINK_CB(skb).portid = 0;      /* from kernel */
//    NETLINK_CB(skb).portid = 0;      /* from kernel */
    memcpy(nlmsg_data(nl_hdr),data,data_len);
    res= netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);

    if (res < 0){
        log_err(ERR_NLSEND_FAILED, "%d while returning response to the user.", res);
		return -1;
	}

	log_debug("nl_send_skb finish");
    return 0;
}
static int nl_rcv_msg(struct sk_buff *skb){
	struct nlmsghdr *nlh;
	struct arguments *args; 
	char buf[MAX_PAYLOAD]={};
	int i;
//	struct net *net = sock_net(skb->sk);


	nlh = nlmsg_hdr(skb);
	log_debug("nl_rcv_msg ,pid:%d",nlh->nlmsg_pid);
	args=(struct arguments *)NLMSG_DATA(nlh);
	switch(args->key){
	case ARGP_ALL:
		for(i=0;i<5;i++){
	/*		nf_ct_appflid_count(net,&app_cnt[i]);
		sprintf(buf,"%s%s packets %lld/up %lld/down  bytes %lld/up %lld/down\n",
				 buf,app_cnt[i].app_proto,app_cnt[i].packets[0],app_cnt[i].packets[1],
				 app_cnt[i].bytes[0],app_cnt[i].bytes[1]);*/
		}
		nl_send_to_user(nlh->nlmsg_pid,buf,strlen(buf));
	break;
	case ARGP_HTTP:
/*		ndinfo_count("http",&up,&down);
		sprintf(buf,"http %ldG/up %ldG/down\n",up>>30,down>>30);
		nl_send_to_user(nlh->nlmsg_pid,buf,strlen(buf));*/
	case ARGP_DNS:
//		nl_send_to_user(nlh->nlmsg_pid,buf,strlen(buf));
		break;	
	}
//    log_debug("nl_rcv_msg recv from user %s\n",(char *)NLMSG_DATA(nlh));
/*  if(pid==0)
    	pid=nlh->nlmsg_pid;*/
    return 0;
}
static void nl_rcv(struct sk_buff *skb)
{
    mutex_lock(&nl_mtx);
    nl_rcv_msg(skb);
    mutex_unlock(&nl_mtx);

}

int  nl_kernel_init(void){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
        nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, 0, nl_rcv,
                        NULL, THIS_MODULE);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
        struct netlink_kernel_cfg nl_cfg = { .input  = nl_rcv };
        nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, THIS_MODULE, &nl_cfg);
#else
        struct netlink_kernel_cfg nl_cfg = { .input  = nl_rcv };
        nl_sk = netlink_kernel_create(&init_net, APPFLID_NETLINK, &nl_cfg);
#endif

        if (!nl_sk) {
                log_err(ERR_SOCKET,"%s creation of netlink socket failed.",__func__);
                return -EINVAL;
        }
        log_debug("Netlink socket created");

        return 0;
}
void   nl_kernel_destroy(void){
        netlink_kernel_release(nl_sk);
}

