#include <linux/ip.h>
#include <linux/ctype.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>



#include "appflid/comm/types.h"
#include "appflid/comm/constants.h"
#include "appflid/comm/print.h"
#include "appflid/mod/config.h"
#include "appflid/mod/ndinfo.h"
#include "appflid/mod/wellkn_port.h"
#include "appflid/mod/aproto.h"

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35)
#else
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_acct.h>
#endif

static int num_packets = 10;
static int maxdatalen = 2048;
static int total_match=0;

static DEFINE_SPINLOCK(appflid_lock);


static bool can_handle(const struct sk_buff *skb)
{
	if(!ip_hdr(skb)) /* not IP */
		return false;
        if(ip_hdr(skb)->protocol != IPPROTO_TCP &&
	               ip_hdr(skb)->protocol != IPPROTO_UDP &&
	               ip_hdr(skb)->protocol != IPPROTO_ICMP)
	        return 0;
	return true;
}

static int app_data_offset(const struct sk_buff *skb)
{
	int ip_hl = 4*ip_hdr(skb)->ihl;

	if( ip_hdr(skb)->protocol == IPPROTO_TCP ) {
		int tcp_hl = 4*(skb->data[ip_hl + 12] >> 4);
	    return ip_hl + tcp_hl;
	}else if( ip_hdr(skb)->protocol == IPPROTO_UDP  ) {
		return ip_hl + 8; /* UDP header is always 8 bytes */
	}else if( ip_hdr(skb)->protocol == IPPROTO_ICMP ) {
		return ip_hl + 8; /* ICMP header is 8 bytes */
	}else{
		log_err(ERR_SYSM,"tried to handle unknown protocol!");
		return ip_hl + 8; /* something reasonable */
	}
}

static int total_acct_packets(struct nf_conn *ct)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35)
	BUG_ON(ct == NULL);
	return (ct->counters[IP_CT_DIR_ORIGINAL].packets + ct->counters[IP_CT_DIR_REPLY].packets);
#else
	struct nf_conn_acct *acct;
	const struct nf_conn_counter *counters;

	BUG_ON(ct == NULL);
	acct = nf_conn_acct_find(ct);
	if (!acct)
		return 0;
	counters = acct->counter;
	return (atomic64_read(&counters[IP_CT_DIR_ORIGINAL].packets)+atomic64_read(&counters[IP_CT_DIR_REPLY].packets));
#endif
}
void free_appdata(struct nf_conn *ct)
{
	if(ct->appflid.app_data != NULL) {
		kfree(ct->appflid.app_data);
		ct->appflid.app_data = NULL; /* don't free again */
	}
}

/* add the new app data to the conntrack.  Return number of bytes added. */
static int add_data(struct nf_conn * mct,
                    char * app_data, int appdatalen)
{
	int length = 0, i;
	int oldlength = mct->appflid.app_data_len;


	/* This is a fix for a race condition by Deti Fliegl. However, I'm not 
	   clear on whether the race condition exists or whether this really 
	   fixes it.  I might just be being dense... Anyway, if it's not really 
	   a fix, all it does is waste a very small amount of time. */
	if(!mct->appflid.app_data) return 0;

	/* Strip nulls. Make everything lower case (our regex lib doesn't
	do case insensitivity).  Add it to the end of the current data. */
	for(i = 0; i < maxdatalen-oldlength-1 &&
		   i < appdatalen; i++) {
		if(app_data[i] != '\0') {
			/* the kernel version of tolower mungs 'upper ascii' */
			mct->appflid.app_data[length+oldlength] =
				isascii(app_data[i])? 
					tolower(app_data[i]) : app_data[i];
			length++;
		}
	}

	mct->appflid.app_data[length+oldlength] = '\0';
	mct->appflid.app_data_len = length + oldlength;

	return length;
}

/*just add the appflid.app_proto*/
void nf_ct_appflid_add(struct nf_conn *ct,const char *app_proto)
{
	if(!app_proto){
		log_debug("%s can not handle the NULL app_proto",__func__);
		return ;
	}

	if(!ct->appflid.app_proto){
		ct->appflid.app_proto = kmalloc(strlen(app_proto)+1, GFP_ATOMIC);
        	if(!ct->appflid.app_proto&&net_ratelimit()){
                	log_err(ERR_ALLOC_FAILED, "appflid: out of memory in %s, bailing.",__func__);
		        return ;
		}

	memset(ct->appflid.app_proto,0,strlen(app_proto)+1);
	strcpy(ct->appflid.app_proto,app_proto);
	}
	total_match++;
}

void appflid_get_tuple(struct nf_conn *ct,struct tuple *tp )
{
	memcpy(&tp->saddr,&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.all,sizeof(tp->saddr));
        memcpy(&tp->sport,&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all,sizeof(tp->sport));
        memcpy(&tp->daddr,&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.all,sizeof(tp->daddr));
	memcpy(&tp->dport,&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all,sizeof(tp->dport));
	memcpy(&tp->l4num,&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum,sizeof(tp->l4num));
}

int core(struct sk_buff *skb)
{
//    	struct net *net = nf_ct_net(ct);
	enum ip_conntrack_info mctinfo, ctinfo;
	struct nf_conn *mct, *ct;
	unsigned char * app_data;
	unsigned int appdatalen = 0;
	struct wellkn_port_entry *wkp;
	struct aproto_node *and;
	struct tuple tp;

	/*part 1,preprocess*/

	if(!can_handle(skb)){
		log_info(MODULE_NAME":this is some protocol I can't handle.");
		goto out;
	}

	spin_lock_bh(&appflid_lock);
	if(!(ct = nf_ct_get(skb, &ctinfo)) ||
		       !(mct = nf_ct_get(skb,&mctinfo))){
    	    	log_debug("%s: couldn't get conntrack.",MODULE_NAME);
		goto out;
	}

//	appflid_print_tuple(&tp);

	while (master_ct(mct) != NULL)
	        mct = master_ct(mct);

	/*part 2,the connections has  been identify*/
	if(mct->appflid.app_proto) {
	    	log_debug("appflid:connection has been identify ,app_proto is %s",mct->appflid.app_proto);
	    	free_appdata(mct);
		if(!ct->appflid.app_proto)
			nf_ct_appflid_add(ct,mct->appflid.app_proto);
		goto out;		 
	}

	/*part 3,to identify the connection */
	/*condition 1,the packets > num_packets*/
	if(total_acct_packets(mct)>num_packets){
	    	log_debug("appflid:the total num_packets of connection is gt num_packets ,app_proto is dentified  unknown ");
	    	free_appdata(mct);
		nf_ct_appflid_add(mct,"unknown");
		goto out;
	}

	/*condition 2 ,only packets==1,need to do wellknown,and alloc the app_data*/
	if(total_acct_packets(mct) == 1 && !mct->appflid.app_data ){
		wkp = wellkn_port_find(ntohs(mct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all));
		if(wkp){
	    	    printk("find the wkp,port =%d\n",wkp->port);
		    nf_ct_appflid_add(mct,wkp->app_proto);
		    goto out;
		}  

		mct->appflid.app_data = kmalloc(maxdatalen, GFP_ATOMIC);
		if(!mct->appflid.app_data){
		    if (net_ratelimit())
			log_err(ERR_ALLOC_FAILED,"appflid: out of memory in match, bailing.");
		    goto out;
		}
		mct->appflid.app_data[0] = '\0';

	}	

	/* Can be here, but unallocated, if numpackets is increased near
	the beginning of a connection */
	if(!mct->appflid.app_data){
	    log_debug("appflid:connect  app_data is NULL ,"
		       "the first packet of this connection is not pass here");
	    goto out;
	}

	if(skb_is_nonlinear(skb) && skb_linearize(skb) != 0){ 
		if (net_ratelimit())
			log_debug("appflid: failed to linearize packet, bailing.");
		goto out;
        }   

	/* now that the skb is linearized, it's safe to set these. */

        app_data = skb->data + app_data_offset(skb);
        appdatalen = skb_tail_pointer(skb) - app_data;

	if(!add_data(mct, app_data, appdatalen)){  /* didn't add any data */
	    log_debug("appflid:the datalen of packet is 0");
	    goto out;
	}	    

	and = aproto_find(mct->appflid.app_data,mct->appflid.app_data_len);/*DPI */
	if(and){
	    printk("dpi success and name=%s\n",and->name);
	    nf_ct_appflid_add(mct,and->name);
	    appflid_get_tuple(mct,&tp);
            if (and->handler(and->name,&tp,app_data,appdatalen) < 0) {/*need complete payload*/
		log_debug("handler failed");
	    }
	}else{
	    log_debug("dpi failed");
	}

out:
//	printk("total_match=%d\n",total_match);
	spin_unlock_bh(&appflid_lock);
	return NF_ACCEPT;
}	

