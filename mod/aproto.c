#include <linux/ctype.h>

#include "appflid/comm/types.h"
#include "appflid/comm/print.h"
#include "appflid/mod/regexp.h"
#include "appflid/mod/aproto.h"
#include "appflid/mod/count.h"
#include "appflid/mod/read_confile.h"

#define FILENAME "aprotocol.conf"
#define MAX_RGXP_LEN 255

static LIST_HEAD(aprotos);
static unsigned int  aprotos_count;
static DEFINE_SPINLOCK(aproto_lock);


struct aproto_node *aproto_find(unsigned char *payload,unsigned int payload_len){
	struct aproto_node *node;
	int match=0;
	int len = 0, i;
	unsigned char *nnpayload;
	
//	printk("paylod:");
//	print_hex(payload,payload_len);
	nnpayload=(unsigned char *)kmalloc(payload_len+1,GFP_ATOMIC);
	if(!nnpayload){
		log_err(ERR_ALLOC_FAILED, "Allocation of nnpayload failed.");
		return NULL;
	}
	memset(nnpayload,0,payload_len+1);

	for(i = 0; i < payload_len; i++) {
		if(payload[i] != '\0') {
			nnpayload[len] = isascii(payload[i])? tolower(payload[i]) : payload[i];
			len++;
		}   
	}   
	
	if(!len){
		log_debug("the payload is null");
		return NULL;
	}

	spin_lock_bh(&aproto_lock);
	list_for_each_entry(node, &aprotos, list_hook) {
		match=regexec(node->rgxp,nnpayload);
		if(match){
			spin_unlock_bh(&aproto_lock);
			return node;	
		}
	}
	spin_unlock_bh(&aproto_lock);
	log_debug("match aproto fail\n");
	return NULL;
}
void aproto_show(void)
{
	struct aproto_node *node;

	spin_lock_bh(&aproto_lock);
	printk("the total count of aproto is %u\n",aprotos_count);
	list_for_each_entry(node, &aprotos, list_hook) {
		printk("app_proto:%s\n",node->name);
	}
	spin_unlock_bh(&aproto_lock);
}


static int hex2dec(char c){
	switch (c) {   
	case '0' ... '9':
		return c - '0';
    case 'a' ... 'f':
        return c - 'a' + 10; 
    case 'A' ... 'F':
        return c - 'A' + 10; 
    default:
        log_err(ERR_OTHER, "hex2dec: bad value!\n");
		return 0;
    }   
}
/* takes a string with \xHH escapes and returns one with the characters 
they stand for */
static char *  pre_process(const char * s)
{
	char * result = kmalloc(strlen(s) + 1,GFP_ATOMIC);
	int sindex = 0, rrindex = 0;
        while( sindex < strlen(s) )
        {
            if( sindex + 3 < strlen(s) &&
                s[sindex] == '\\' && s[sindex+1] == 'x' && 
                isxdigit(s[sindex + 2]) && isxdigit(s[sindex + 3]) ) 
                {
                        /* carefully remember to call tolower here... */
                        result[rrindex] = tolower( hex2dec(s[sindex + 2])*16 +
                                                  hex2dec(s[sindex + 3] ) );

			switch ( result[rrindex] )
			{
			case 0x24:
			case 0x28:
			case 0x29:
			case 0x2a:
			case 0x2b:
			case 0x2e:
			case 0x3f:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5e:
			case 0x7c:
				printk("Warning: regexp contains a control character, %c, in hex (\\x%c%c).\n"
					"I recommend that you write this as %c or \\%c, depending on what you meant.\n",
					result[rrindex], s[sindex + 2], s[sindex + 3], result[rrindex], result[rrindex]);
				break;
			case 0x00:
				printk("Warning: null (\\x00) in regexp.  A null terminates the regexp string!\n");
				break;
			default:
				break;
			}


                        sindex += 3; /* 4 total */
                }
                else
                        result[rrindex] = tolower(s[sindex]);

		sindex++; 
		rrindex++;
        }
	result[rrindex] = '\0';

	return result;
}
static int is_contains_aproto(const char *name)
{
	struct aproto_node *node;

	spin_lock_bh(&aproto_lock);
	list_for_each_entry(node, &aprotos, list_hook) {
		if(!strcmp(name,node->name)){
			spin_unlock_bh(&aproto_lock);
			return 1;
		}
	}
	spin_unlock_bh(&aproto_lock);
	
	return 0;
}

static int aproto_add(struct aproto_node *and)
{
	struct aproto_node *new;


	if (!and) {
		log_err(ERR_NULL, "can not handle  NULL value.");
		return -EINVAL;
	}



	new = kmalloc(sizeof(struct aproto_node), GFP_ATOMIC);
	if (!new) {
		log_err(ERR_ALLOC_FAILED, "Allocation of aproto node failed.");
		return -ENOMEM;
	}

	memset(new,0,sizeof(struct aproto_node));
	memcpy(new->name,and->name,strlen(and->name));
	new->rgxp = and->rgxp;
	if (!and->handler || !and->show) {
		log_err(ERR_NULL,"the handler or show function can not be NULL");
		return -EINVAL;
	}
	new->handler = and->handler;
	new->show = and->show;
	

	spin_lock_bh(&aproto_lock);
	list_add_tail(&new->list_hook, &aprotos);
	aprotos_count++;
	spin_unlock_bh(&aproto_lock);

	return 0;
}
static int parse_conf(char * conf,char *name ,char *str_rgxp){
	char* line=NULL;

	log_debug("start parse config file ....");
    	while ( (line = strsep(&conf, "\n")) != NULL){
		char *ptr = NULL; 

		if (*line == '#' || strlen(line) == 0)
	        	continue;

	    	ptr = strchr(line,DECOLLATER);
		if (ptr - line > APPNAMSIZ - 1) {
			log_info("the protocol name is too long,the max is %d",APPNAMSIZ - 1);
			return -1;
		}
	    	strncpy(name,line,ptr - line);
		
		if (strlen(ptr + 1) > MAX_RGXP_LEN - 1) {
			log_info("the regexp is too long,the max is %d",MAX_RGXP_LEN - 1);
			return -1;
		}
            	strncpy(str_rgxp,ptr + 1,strlen(ptr + 1));

                count_add_proto(name);
		break;
   	}
	if (!strlen(name) || !strlen(str_rgxp)){
		log_info("nothing to be parse");
		return -1;
	}

	return 0;
}
static void aproto_remove(struct aproto_node *and)
{
	struct aproto_node *node;

	spin_lock_bh(&aproto_lock);
        list_for_each_entry(node, &aprotos, list_hook) {
                if(!strcmp(and->name,node->name)){
			list_del(&node->list_hook);
                	kfree(node->rgxp);
                	kfree(node);
			break;	
                }
        }
        spin_unlock_bh(&aproto_lock);
	
}

int register_aproto(struct aproto_node *and,const char *confile_name)
{
	char *confile;
	char *ptr;
	char name[APPNAMSIZ];
	char str_rgxp[MAX_RGXP_LEN];
	int rgxp_len;
	int err = -1 ;
	
	if (!confile_name) 
		return -1;
	

	memset(name,0,APPNAMSIZ);
	memset(str_rgxp,0,MAX_RGXP_LEN);

	confile = read_confile(confile_name);
	if (confile) {
		err = parse_conf(confile,name,str_rgxp);
		kfree(confile);
	}
	if (err < 0)
		return err;

	if (is_contains_aproto(name)) {
		log_info("the aproto %s is exist",name);
		return -1;
	}

	ptr = pre_process(str_rgxp);
	rgxp_len = strlen(ptr);	
	
	and->rgxp = regcomp(ptr,&rgxp_len);	

	kfree(ptr);

        if(!and->rgxp){
                log_err(ERR_NULL,"rgxp is NULL,regcomp fail");
                return -1; 
        }   	
	
	strcpy(and->name,name);
	return aproto_add(and);
	
}
EXPORT_SYMBOL(register_aproto);

void unregister_aproto(struct aproto_node *and)
{
	if (!and->name){
		log_info("the proto is unregister");
		return ;		
	}
	aproto_remove(and);
	count_remove_proto(and->name);	
	
}
EXPORT_SYMBOL(unregister_aproto);

int aproto_init(void)
{
	aprotos_count=0;
	return 0;
}
void aproto_destroy(void){
	spin_lock_bh(&aproto_lock);
	while (!list_empty(&aprotos)) {
		struct aproto_node *node = container_of(aprotos.next, struct aproto_node, list_hook);
		list_del(&node->list_hook);
		kfree(node->rgxp);
		kfree(node);
	}	
	spin_unlock_bh(&aproto_lock);
}
