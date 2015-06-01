#include <linux/kernel.h>
#include "appflid/comm/print.h"

void print_hex(const char * s,unsigned int len){
	    int i;
	    for(i=0;i<len;i++){
			if(i%16==0)
				printk("\n");
            printk("%02x ",s[i]&0xff);
		}
	    printk("\n");
				    
}
