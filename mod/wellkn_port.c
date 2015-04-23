#include <linux/rbtree.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "appflid/comm/types.h"
#include "appflid/comm/constants.h"
#include "appflid/mod/kstrtox.h"
#include "appflid/mod/config.h"
#include "appflid/mod/rbtree.h"
#include "appflid/mod/wellkn_port.h"
#include "appflid/mod/read_confile.h"

#define FILENAME "wellkn_port.conf"


static struct wellkn_port_table wkps;
static struct kmem_cache *wkp_entry_cache;

DEFINE_SPINLOCK(wellkn_port_lock);

static int compare_port(struct wellkn_port_entry *wkp,__u16 port ){
	return  wkp->port - port;
}

int wellkn_port_add(const __u16 port,const char * app_name){
	int error;
	struct wellkn_port_entry *result = kmem_cache_alloc(wkp_entry_cache, GFP_ATOMIC);
	if (!result)
		return -1;

	log_debug("wellkn_port_add %d %s",port,app_name);
	memset(result,0,sizeof(struct wellkn_port_entry));
	result->port = port;
	memcpy(result->app_name,app_name,strlen(app_name));
	RB_CLEAR_NODE(&result->tree_hook);
	
	spin_lock_bh(&wellkn_port_lock);
	error = rbtree_add(result,port,&wkps.tree, compare_port, struct wellkn_port_entry, tree_hook);
	if (error){
		log_debug("rbtree_add failed");
		return error;
	}

	wkps.count++;
	spin_unlock_bh(&wellkn_port_lock);
	return 0;
}
struct wellkn_port_entry *  wellkn_port_find(__u16 port){
	struct wellkn_port_entry *result=NULL;
	if (ntohs(port)<0||ntohs(port)>65535) {
		log_warning("The port %d is out of range.",port);
		return result;
	}   
	spin_lock_bh(&wellkn_port_lock);
    result = rbtree_find(port, &wkps.tree, compare_port, struct wellkn_port_entry, tree_hook);
	spin_unlock_bh(&wellkn_port_lock);
    return result;
} 
/*for debug*/
void wellkn_port_show(void){
	struct rb_node *node;
	spin_lock_bh(&wellkn_port_lock);
	printk("the total wellkn_port is %ld\n",wkps.count);
	for (node = rb_first(&wkps.tree); node; node = rb_next(node)){
			  struct wellkn_port_entry *wkp=rb_entry(node, struct wellkn_port_entry, tree_hook);
		      printk("port %d,app_name %s\n", wkp->port,wkp->app_name);
    }
	spin_unlock_bh(&wellkn_port_lock);
}
static int parse_conf(char * conf){
	char* line=NULL;
	int err=0;

	log_debug("start parse config file ....");
    while ( (line = strsep(&conf, "\n")) != NULL){
		__u16 port;
		char app_name[APPNAMSIZ]={};
		char *ptr = NULL; 
	    char tmp_port[10]={};

	    if (*line == '#' || strlen(line) == 0)
            continue;

    	ptr = strchr(line,DECOLLATER);
    	strncpy(app_name,ptr+1,strlen(ptr+1));
	    strncpy(tmp_port,line,ptr-line);
		if(kstrtou16(tmp_port,10,&port))
			return -EINVAL;
		err=wellkn_port_add(port,app_name);
		if(err)
			return err;
   	}
	return err;
}


int wellkn_port_init(void){
	  int err = -1;
	  char *confile = NULL;
	  wkp_entry_cache = kmem_cache_create("appflid_wkp_entries", sizeof(struct wellkn_port_entry),
			  0, 0, NULL);
      if (!wkp_entry_cache) {
	      log_err(ERR_ALLOC_FAILED, "Could not allocate the wellkn_port_entry cache.");
	      return -ENOMEM;
	  }

	  wkps.tree = RB_ROOT;
	  wkps.count = 0;
	  /*read the config file*/
	  confile=read_confile(FILENAME);
	  if(confile){
	  	err=parse_conf(confile);
      	kfree(confile);
	  }
      return err;
	  
}
static void wellkn_port_destroy_aux(struct rb_node *node)
{
	        kmem_cache_free(wkp_entry_cache,rb_entry(node, struct wellkn_port_entry, tree_hook));
}
void wellkn_port_destroy(void){
	log_debug("Emptying the wellkn_port_table...");
	wellkn_port_show();
	spin_lock_bh(&wellkn_port_lock);

	rbtree_clear(&wkps.tree, wellkn_port_destroy_aux);

	spin_unlock_bh(&wellkn_port_lock);
	kmem_cache_destroy(wkp_entry_cache);
}


