#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "appflid/comm/types.h"
#include "appflid/comm/constants.h"
#include "appflid/comm/print.h"
#include "appflid/mod/config.h"
#include "appflid/mod/ndinfo.h"
#include "appflid/mod/wellkn_port.h"
#include "appflid/mod/pattern.h"

static int payload_offset(const struct sk_buff *skb){
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
/*get key ,find ndinfo and update ndinfo ,may be it should be more functions ,
 * but I do not find a perfect way to handle them
 * */
static int  preprocess_ndinfo(int dir,struct ndinfo_key *key,struct iphdr *iphdr){
	struct ndinfo_entry *ndinfo = NULL;
	char *layer4ptr = NULL;
	__u16 tmp_port;
	int find = 0;

	layer4ptr = (char *)iphdr+iphdr->ihl*4;
	if(dir==DOWN){
		key->addr.s_addr = iphdr->saddr;
		memcpy(&tmp_port,layer4ptr,sizeof(tmp_port));
	}else {
		key->addr.s_addr = iphdr->daddr;
		memcpy(&tmp_port,layer4ptr+sizeof(tmp_port),sizeof(tmp_port));
	
	}
	key->port=ntohs(tmp_port);
	ndinfo = ndinfo_find(key);
	if(ndinfo){
		find = 1;
		if(ndinfo_update(ndinfo,dir,ntohs(iphdr->tot_len)))/*update faild*/
			log_debug("ndinfo_update faild");
	}
	log_debug("Catch the packet ,dir %s,%pI4->%pI4,keyip %pI4,keyport %d len %d",dir?"DOWN":"UP",(struct in_addr *)&iphdr->saddr,
			 (struct in_addr *)&iphdr->daddr,&key->addr,key->port,ntohs(iphdr->tot_len));

	return find;

}

int core(struct sk_buff *skb,const char  *outif_name){
	struct iphdr *iphdr = NULL;
	struct ndinfo_key key;
	struct wellkn_port_entry *wkp = NULL;
	struct pattern_node *ptn = NULL;
	unsigned char *payload ;
	unsigned int payload_len;
	char app_name[APPNAMSIZ]={};
	int dir;

	iphdr = ip_hdr(skb);
	

    if(strstr(config_get()->userif, outif_name)){/*the outif is user , mean down*/
		/*find the node and update the bytes[DOWN]*/
		dir = DOWN;
		if(preprocess_ndinfo(dir,&key,iphdr))/*find and update*/
			return NF_ACCEPT;
		else
			goto unknown;

	}else if(strstr(config_get()->resrcif,outif_name)){/*the outif is resource ,mean up*/

		dir = UP;
		if(preprocess_ndinfo(dir,&key,iphdr)){/*find and update*/
			return NF_ACCEPT;
		}else{
			log_debug("do not find the node ...........");
			wkp = wellkn_port_find(key.port);
			if(wkp){
				if(ndinfo_add(&key.addr ,key.port,ntohs(iphdr->tot_len),wkp->app_name)){/*add faild*/
					og_debug("ndinfo_add failed");
			}		
				return NF_ACCEPT;
			}else {/*can't find the wkp,need to do DPI*/
				
				/*if payload is 0 ,return inmediately*/
				if(skb_is_nonlinear(skb)){
					if(skb_linearize(skb) != 0){ 
			      		log_err(ERR_SYSM," failed to linearize packet, bailing.");
				        return NF_ACCEPT;
				    }   
				}   

				      /* now that the skb is linearized, it's safe to set these. */
				payload = skb->data + payload_offset(skb);
				payload_len = skb_tail_pointer(skb) - payload;
				if(!payload_len||!payload){
					log_debug("payload_len is 0 or get_skb_payload faild");
					return NF_ACCEPT;
				}
				ptn = pattern_find(payload,payload_len);
				if(ptn){
					if(ndinfo_add(&key.addr ,key.port,ntohs(iphdr->tot_len),ptn->app_name)){/*add faild*/
						log_debug("ndinfo_add failed");
			        } 
					return NF_ACCEPT;
				}		
				goto unknown;
			}
		}
	}else{
		log_debug("the USERIF %s,RESOURCEIF %s,unknown out->name %s ",USERIF_DEF,RESRCIF_DEF,outif_name);
		return NF_ACCEPT;
	}

unknown:
//	printk("can't not identify the packet\n");
	if(ndinfo_add(&key.addr ,key.port,ntohs(iphdr->tot_len),"unknown"))/*add faild*/
		log_debug("ndinfo_add faild");
	return NF_ACCEPT;
}	

