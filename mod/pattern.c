#include <linux/ctype.h>

#include "appflid/comm/types.h"
#include "appflid/comm/print.h"
#include "appflid/mod/regexp.h"
#include "appflid/mod/pattern.h"
#include "appflid/mod/count.h"
#include "appflid/mod/read_confile.h"

#define FILENAME "protocol.conf"
#define MAX_RGXP_LEN 255

static LIST_HEAD(patterns);
static unsigned int  patterns_count;
static DEFINE_SPINLOCK(pattern_lock);


struct pattern_node *pattern_find(unsigned char *payload,unsigned int payload_len){
	struct pattern_node *node;
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

	spin_lock_bh(&pattern_lock);
	list_for_each_entry(node, &patterns, list_hook) {
		match=regexec(node->rgxp,nnpayload);
		if(match){
			spin_unlock_bh(&pattern_lock);
			return node;	
		}
	}
	spin_unlock_bh(&pattern_lock);
	log_debug("match pattern fail\n");
	return NULL;
}
void pattern_show(void){
	struct pattern_node *node;

	spin_lock_bh(&pattern_lock);
	printk("the total count of pattern is %u\n",patterns_count);
	list_for_each_entry(node, &patterns, list_hook) {
		printk("app_proto:%s\n",node->app_proto);
	}
	spin_unlock_bh(&pattern_lock);
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
static char *  pre_process(char * s)
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


int pattern_add(char *_app_proto,char *_str_rgxp){
	struct pattern_node *node;
	int str_len=0;
	char *str_rgxp;


	if (!_app_proto||!_str_rgxp) {
		log_err(ERR_NULL, "app_proto or regexp is NULL.");
		return -EINVAL;
	}

	str_rgxp=pre_process(_str_rgxp);


	node = kmalloc(sizeof(struct pattern_node), GFP_ATOMIC);
	if (!node) {
		log_err(ERR_ALLOC_FAILED, "Allocation of pattern node failed.");
		return -ENOMEM;
	}
	str_len=strlen(str_rgxp);
	memset(node,0,sizeof(struct pattern_node));
	memcpy(node->app_proto,_app_proto,strlen(_app_proto));
	
/*	printk("recomp:");
	print_hex(str_rgxp,str_len);*/

	node->rgxp=regcomp(str_rgxp,&str_len);

	kfree(str_rgxp);
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
		char app_proto[APPNAMSIZ]={};
		char *ptr = NULL; 
	    char str_rgxp[MAX_RGXP_LEN] = {};

	    if (*line == '#' || strlen(line) == 0)
            continue;

	    ptr = strchr(line,DECOLLATER);
	    strncpy(app_proto,line,ptr-line);
            strncpy(str_rgxp,ptr+1,strlen(ptr+1));
	    count_add_proto(app_proto);

//		printk("%s:app_proto=%s,str_rgxp=%s",__func__,app_proto,str_rgxp);
		err=pattern_add(app_proto,str_rgxp);
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
