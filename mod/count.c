#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <linux/rculist_nulls.h>
#include <linux/kprobes.h>

#include "appflid/comm/types.h"
#include "appflid/mod/count.h"

static struct app_counter active[MAX_PROTO];
static struct app_counter inactive[MAX_PROTO];
static int count_index;

static DEFINE_SPINLOCK(count_lock);


int  count_add_proto(const char *app_proto)
{
	if (!app_proto)	
		return -1;

	spin_lock_bh(&count_lock);
	if (count_index >= MAX_PROTO){
		log_debug("the number of proto (%d) is greater than MAX_PROTO (%d) ",count_index + 1, MAX_PROTO);
		spin_unlock_bh(&count_lock);
		return -1;	
	}
	
	strcpy(active[count_index].app_proto,app_proto);
	strcpy(inactive[count_index].app_proto,app_proto);
	count_index++;
	spin_unlock_bh(&count_lock);
	return 0;
}

int count_remove_proto(const char *app_proto)
{
	int i;
	int rindex = -1;

	if (!app_proto)	
		return -1;
	
	spin_lock_bh(&count_lock);
	for (i = 0;i < count_index; i++) {
		if (rindex >= 0 ) {
			memcpy(&active[i-1],&active[i],sizeof(struct app_counter));
			memcpy(&inactive[i-1],&inactive[i],sizeof(struct app_counter));
		}

		if (rindex == -1 && !strcmp(active[i].app_proto,app_proto)) {
			rindex = i;			
		}
	}

	count_index--;			
	memset(&active[count_index],0,sizeof(struct app_counter));
	memset(&inactive[count_index],0,sizeof(struct app_counter));
	spin_unlock_bh(&count_lock);
		
	return rindex == -1 ? -1:0;


}

static struct app_counter *count_strcmp(const char *app_proto,struct app_counter *app_cnt)
{	
	int i;
	if(!app_proto)
		return NULL;

	spin_lock_bh(&count_lock);

	for (i = 0;i < count_index;i++) {
		if (!strcmp(app_cnt[i].app_proto,app_proto)) {
			spin_unlock_bh(&count_lock);
			return &app_cnt[i];
		}
	}

	spin_unlock_bh(&count_lock);
	return NULL;
}


static int count_add(struct nf_conn *ct ,struct app_counter *app_cnt)
{
	struct nf_conn_acct *acct;
	const struct nf_conn_counter *counters;
 
        acct = nf_conn_acct_find(ct);
        if (!acct)
                return -1;
        counters = acct->counter;

        atomic64_add(atomic64_read(&counters[IP_CT_DIR_ORIGINAL].packets),&app_cnt->packets[IP_CT_DIR_ORIGINAL]);
        atomic64_add(atomic64_read(&counters[IP_CT_DIR_REPLY].packets),&app_cnt->packets[IP_CT_DIR_REPLY]);
        atomic64_add(atomic64_read(&counters[IP_CT_DIR_ORIGINAL].bytes),&app_cnt->bytes[IP_CT_DIR_ORIGINAL]);
        atomic64_add(atomic64_read(&counters[IP_CT_DIR_REPLY].bytes),&app_cnt->bytes[IP_CT_DIR_REPLY]);
	return 0;
}

static void flush_counter(struct app_counter *app_cnt)
{
	int i;
	
	spin_lock_bh(&count_lock);
	for (i = 0; i < count_index;i++) {
		atomic64_set(&app_cnt[i].packets[IP_CT_DIR_ORIGINAL],0);
		atomic64_set(&app_cnt[i].packets[IP_CT_DIR_REPLY],0);
		atomic64_set(&app_cnt[i].bytes[IP_CT_DIR_ORIGINAL],0);
		atomic64_set(&app_cnt[i].bytes[IP_CT_DIR_REPLY],0);
	}
	spin_unlock_bh(&count_lock);
}
static void show_counter(struct app_counter *app_cnt)
{
	int i;
	printk("the total proto is %d\n",count_index);
	spin_lock_bh(&count_lock);
	for (i = 0; i < count_index;i++) {
		printk("%s packets %ld/up %ld/down,bytes %ld/up %ld/down\n",
		app_cnt[i].app_proto,
		atomic64_read(&app_cnt[i].packets[IP_CT_DIR_ORIGINAL]),
		atomic64_read(&app_cnt[i].packets[IP_CT_DIR_REPLY]),
		atomic64_read(&app_cnt[i].bytes[IP_CT_DIR_ORIGINAL]),
		atomic64_read(&app_cnt[i].bytes[IP_CT_DIR_REPLY]));
	}
	spin_unlock_bh(&count_lock);

	
}
static int count_active(struct net *net)
{
	int bucket;
        struct nf_conntrack_tuple_hash *h;
        struct hlist_nulls_node *n;
	struct nf_conn *ct;
	struct app_counter *app_cnt;


	rcu_read_lock();

	for (bucket = 0; bucket < net->ct.htable_size;bucket++) {
		/*if the hlist_nulls_head is null*/
		if (is_a_nulls(rcu_dereference(hlist_nulls_first_rcu(&net->ct.hash[bucket]))))
			continue;	

		hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[bucket], hnnode) {
			if (NF_CT_DIRECTION(h))/*only count original direction*/
				continue;

			ct = nf_ct_tuplehash_to_ctrack(h);
			if(ct && (app_cnt = count_strcmp(ct->appflid.app_proto,active))){
				if (count_add(ct,app_cnt)) {
					rcu_read_unlock();
					return -1;	
				}	
			}
		}			
	}
	rcu_read_unlock();
	return 0;
}

static void  count_inactive(struct nf_conn *nfct)
{
        struct nf_conn *ct = (struct nf_conn *)nfct;
	struct app_counter *app_cnt;

	
	if (!ct || !ct->appflid.app_proto){
        	jprobe_return();
		return ;
	}
	
	rcu_read_lock();
	app_cnt = count_strcmp(ct->appflid.app_proto,inactive);
	if (!app_cnt){
		rcu_read_unlock();
        	jprobe_return();
		return ;
	} 
	
	if(count_add(ct,app_cnt))
		log_debug("appflid :%s failed,maybe the nf_conntrack_acct is disable  ",__func__);

	rcu_read_unlock();

       	jprobe_return();
	return ;
	
		
}

int count_total(struct net *net,char *buf,size_t buf_len)
{
	int i;
	size_t per_len = 0;
	
	flush_counter(active);
	count_active(net);

	memset(buf,0,buf_len);
	
	spin_lock_bh(&count_lock);
	for (i = 0; i < count_index; i++) {
		sprintf(buf,"%s%s packets %ld/up %ld/down,bytes %ld/up %ld/down\n",
			buf,active[i].app_proto,
		atomic64_read(&active[i].packets[IP_CT_DIR_ORIGINAL])+atomic64_read(&inactive[i].packets[IP_CT_DIR_ORIGINAL]),
		atomic64_read(&active[i].packets[IP_CT_DIR_REPLY])+atomic64_read(&inactive[i].packets[IP_CT_DIR_REPLY]),
		atomic64_read(&active[i].bytes[IP_CT_DIR_ORIGINAL])+atomic64_read(&inactive[i].bytes[IP_CT_DIR_ORIGINAL]),
		atomic64_read(&active[i].bytes[IP_CT_DIR_REPLY])+atomic64_read(&inactive[i].bytes[IP_CT_DIR_REPLY]));
		if(i==0)
			per_len = strlen(buf);
		if (buf_len - strlen(buf) < per_len + 1)	{
			spin_unlock_bh(&count_lock);
			return -1;
		}
	}
	spin_unlock_bh(&count_lock);
	return 0;
}

static struct jprobe count_jprobe = {   
    .entry   = count_inactive,  
    .kp = {
    .symbol_name    = "destroy_conntrack",
    }
};

int count_init(void)
{
	int err;
	memset(active,0,sizeof(struct app_counter) * MAX_PROTO);	
	memset(inactive,0,sizeof(struct app_counter) * MAX_PROTO);	
        count_index = 0;
	count_add_proto("unknown");	

        err = register_jprobe(&count_jprobe);

	return err;
}

void count_destroy(void)
{
	unregister_jprobe(&count_jprobe);
}









