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

static void count_add(struct nf_conn *ct ,struct app_counter *app_cnt){
	struct  
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
				count_add(ct,app_cnt);
		}			
	}
	rcu_read_unlock();
}

int count_total(void)
{

}



int count_init(void)
{
        count_index = 0;
        return 0;
}

void count_destroy(void)
{

}
