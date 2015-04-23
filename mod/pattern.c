#include "appflid/comm/types.h"
#include "appflid/mod/regexp.h"
#include "appflid/mod/pattern.h"
#include "appflid/mod/read_confile.h"

#define FILENAME "protocol.conf"
#define MAX_RGXP_LEN 255

static LIST_HEAD(patterns);
static long patterns_count;
static DEFINE_SPINLOCK(pattern_lock);


struct pattern_node *pattern_find(char *payload){
	struct pattern_node *node;
	int match=0;

	spin_lock_bh(&pattern_lock);
	list_for_each_entry(node, &patterns, list_hook) {
		match=regexec(node->rgxp,payload);
		if(match){
			spin_unlock_bh(&pattern_lock);
			return node;	
		}
	}
	spin_unlock_bh(&pattern_lock);
	log_debug("match pattern fail");
	return NULL;
}
void pattern_show(void){
	struct pattern_node *node;

	spin_lock_bh(&pattern_lock);
	printk("the total count of pattern is %ld\n",patterns_count);
	list_for_each_entry(node, &patterns, list_hook) {
		printk("app_name:%s\n",node->app_name);
	}
	spin_unlock_bh(&pattern_lock);
}
int pattern_add(char *_app_name,char *_str_rgxp){
	struct pattern_node *node;
	int str_len=0;

	if (!_app_name||!_str_rgxp) {
		log_err(ERR_NULL, "app_name or regexp is NULL.");
		return -EINVAL;
	}


	node = kmalloc(sizeof(struct pattern_node), GFP_ATOMIC);
	if (!node) {
		log_err(ERR_ALLOC_FAILED, "Allocation of IPv6 pool node failed.");
		return -ENOMEM;
	}
	str_len=strlen(_str_rgxp);
	memset(node,0,sizeof(struct pattern_node));
	memcpy(node->app_name,_app_name,strlen(_app_name));
	node->rgxp=regcomp(_str_rgxp,&str_len);
	if(!node->rgxp){
		log_err(ERR_NULL,"rgxp is NULL,regcomp fail");
		kfree(node);
		return -1;
	}

	spin_lock_bh(&pattern_lock);
	list_add_tail(&node->list_hook, &patterns);
	patterns_count++;
	spin_unlock_bh(&pattern_lock);

	return 0;
}
static int parse_conf(char * conf){
	char* line=NULL;
	int err=0;

	log_debug("start parse config file ....");
    while ( (line = strsep(&conf, "\n")) != NULL){
		char app_name[APPNAMSIZ]={};
		char *ptr = NULL; 
	    char str_rgxp[MAX_RGXP_LEN] = {};

	    if (*line == '#' || strlen(line) == 0)
            continue;

    	ptr = strchr(line,DECOLLATER);
	    strncpy(app_name,line,ptr-line);
    	strncpy(str_rgxp,ptr+1,strlen(ptr+1));

//		printk("%s:app_name=%s,str_rgxp=%s",__func__,app_name,str_rgxp);
		err=pattern_add(app_name,str_rgxp);
		if(err)
			return err;
   	}
	return err;
}

int pattern_init(void){
	int err = -1;
	char *confile = NULL;
	patterns_count=0;
	confile=read_confile(FILENAME);
	if(confile){
		err=parse_conf(confile);
		kfree(confile);
	}
	pattern_show();
	return err;
}
void pattern_destroy(void){
	spin_lock_bh(&pattern_lock);
	while (!list_empty(&patterns)) {
		struct pattern_node *node = container_of(patterns.next, struct pattern_node, list_hook);
		list_del(&node->list_hook);
		kfree(node->rgxp);
		kfree(node);
	}	
	spin_unlock_bh(&pattern_lock);
}
