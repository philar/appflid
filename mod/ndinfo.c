#include <linux/rbtree.h>
#include "appflid/comm/types.h"
#include "appflid/mod/rbtree.h"
#include "appflid/mod/ndinfo.h"


static struct ndinfo_table ndinfos;
static struct kmem_cache *entry_cache;
//TODO (ndinfo_lock);
DEFINE_SPINLOCK(ndinfo_lock);

static int compare_key(struct ndinfo_entry *ndinfo,struct ndinfo_key *key ){
	int gap;

	gap = memcmp(&ndinfo->addr,&key->addr , sizeof(key->addr));
	if (gap != 0)
   		return gap;

	gap = ndinfo->port - key->port;
	return gap;
	
} 

struct ndinfo_entry * ndinfo_find(struct ndinfo_key *key){
	struct ndinfo_entry *result=NULL;
	if (!key) {
			log_warning("The ndinfo_table cannot contain NULL.");
			return result;
	}
	spin_lock_bh(&ndinfo_lock);
	result = rbtree_find(key, &ndinfos.tree, compare_key, struct ndinfo_entry, tree_hook);
	spin_unlock_bh(&ndinfo_lock);
	return result;

}
int ndinfo_update(struct ndinfo_entry * ndinfo,int dir ,long bytes){
	/*TODO*/
	/*
	 *update the timer and the bytes;
	 * */
	spin_lock_bh(&ndinfo_lock);
	ndinfo->bytes[dir] += bytes;
	spin_unlock_bh(&ndinfo_lock);
	return 0;
}

int ndinfo_add(const struct in_addr *addr ,const __u16 port,const long up_bytes,const char * app_name/*,const int app_type*/){
	int error;
	struct ndinfo_key key;
	struct ndinfo_entry *result = kmem_cache_alloc(entry_cache, GFP_ATOMIC);
	if (!result)
			return -1;
	memset(result,0,sizeof(struct ndinfo_entry));
	memcpy(&result->addr,addr,sizeof(result->addr));
	result->port = port;
	result->bytes[UP] = up_bytes;
	memcpy(result->app_name,app_name,strlen(app_name));
//	result->app_type = app_type;
	RB_CLEAR_NODE(&result->tree_hook);

	key.addr=result->addr;
	key.port=result->port;
	
	spin_lock_bh(&ndinfo_lock);
	error = rbtree_add(result,&key, &ndinfos.tree, compare_key, struct ndinfo_entry, tree_hook);
	if (error)
			return error;
	ndinfos.count++;
	spin_unlock_bh(&ndinfo_lock);
	return 0;
}
/***************count**************************/
void ndinfo_count(const char *app_name,long *up,long * down){
	struct rb_node *node;
	spin_lock_bh(&ndinfo_lock);
	for (node = rb_first(&ndinfos.tree); node; node = rb_next(node)){
		struct ndinfo_entry *ndinfo=rb_entry(node, struct ndinfo_entry, tree_hook);
		if(strcmp(app_name,ndinfo->app_name)==0){
  			(*up)+=ndinfo->bytes[UP];
  			(*down)+=ndinfo->bytes[DOWN];
    	}   
	}
	spin_unlock_bh(&ndinfo_lock);

}
 /**************************************/
void ndinfo_show(void){
	  struct rb_node *node;
	  spin_lock_bh(&ndinfo_lock);
      printk("the total ndinfo is %ld\n",ndinfos.count);
      for (node = rb_first(&ndinfos.tree); node; node = rb_next(node)){
	      struct ndinfo_entry *ndinfo=rb_entry(node, struct ndinfo_entry, tree_hook);
    	  printk("%pI4  %d   %s %ld/up  %ld/down\n",&ndinfo->addr,ndinfo->port,
				   ndinfo->app_name,ndinfo->bytes[UP],ndinfo->bytes[DOWN]);
      } 
	  spin_unlock_bh(&ndinfo_lock);

}
int ndinfo_init(void){
	entry_cache = kmem_cache_create("appflid_ndinfo_entries", sizeof(struct ndinfo_entry),
			0, 0, NULL);
//			0, SLAB_POISON, NULL);
	if (!entry_cache) {
				log_err(ERR_ALLOC_FAILED, "Could not allocate the ndinfo_entry cache.");
				return -ENOMEM;
	}

	ndinfos.tree = RB_ROOT;
	ndinfos.count = 0;
	return 0;
}
static void ndinfo_destroy_aux(struct rb_node *node)
{
		kmem_cache_free(entry_cache,rb_entry(node, struct ndinfo_entry, tree_hook));
}
void ndinfo_destroy(void){
	log_debug("Emptying the ndinfo_table...");
	ndinfo_show();
	spin_lock_bh(&ndinfo_lock);
	rbtree_clear(&ndinfos.tree, ndinfo_destroy_aux);
	spin_unlock_bh(&ndinfo_lock);
	kmem_cache_destroy(entry_cache);
}
