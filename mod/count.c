#include "appflid/mod/count.h"

static struct app_counter active[MAX_PROTO];
static struct app_counter inactive[MAX_PROTO];
static int count_index;


int  count_add_proto(const char *app_proto)
{
	if (!app_proto)	
		return -1;
	if (count_index >= MAX_PROTO){
		log_debug("the number of proto (%d) is greater than MAX_PROTO (%d) ",count_index + 1, MAX_PROTO);
		return -1;	
	}

	strcpy(active[count_index].app_proto,app_proto);
	strcpy(inactive[count_index].app_proto,app_proto);
	count_index++;
	return 0;
}

static struct app_counter *count_strcmp(const char *app_proto,struct app_counter *app_cnt)
{	
	int i;
	if(!app_proto)
		return NULL;

	for (i = 0;i < count_index;i++) {
		if(!strcmp(app_cnt[i].app_proto,app_proto))
			return &app_cnt[i];
	}
	return NULL;
}

int count_inactive(struct nf_conn *ct)
{
	
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

int count_active(struct net *net)
{
	int bucket;
        struct nf_conntrack_tuple_hash *h;
        struct hlist_nulls_node *n;
	struct nf_conn *ct;
	struct app_counter *app_cnt;

	rcu_read_lock();

	for (bucket = 0; bucket < net->ct.htable_size;bucket++) {
		hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[bucket], hnnode) {
			ct = nf_ct_tuplehash_to_ctrack(h)
			if(ct && (app_cnt = count_strcmp(ct->appflid.app_proto,active)))
				if (count_add(ct,app_cnt)) {
					rcu_read_unlock();
					return -1;	
				}	
	
		}			
	}
	rcu_read_unlock();
	return 0;
}

int count_total(struct net *net,char *buf,size_t buf_len)
{
	int i;
	size_t per_len;

	count_active(net);

	memset(buf,0,buf_len);
	for (i = 0; i < count_index; i++) {
		sprintf(buf,"%s%s packets %ld/up %ld/down,bytes %ld/up %ld/down"
			buf,active[i].proto,
		atomic64_read(&active[i].packets[IP_CT_DIR_ORIGINAL])+atomic64_read(&inactive[i].packets[IP_CT_DIR_ORIGINAL]),
		atomic64_read(&active[i].packets[IP_CT_DIR_REPLY])+atomic64_read(&inactive[i].packets[IP_CT_DIR_REPLY]),
		atomic64_read(&active[i].bytes[IP_CT_DIR_ORIGINAL])+atomic64_read(&inactive[i].bytes[IP_CT_DIR_ORIGINAL]),
		atomic64_read(&active[i].bytes[IP_CT_DIR_REPLY])+atomic64_read(&inactive[i].bytes[IP_CT_DIR_REPLY]));
		if(i==0)
			per_len = strlen(buf);
		if(buf_len - strlen(buf) < per_len + 1)
			return -1;
	}
	return 0;
}



int count_init(void)
{
        count_index = 0;
        return 0;
}

void count_destroy(void)
{

}
