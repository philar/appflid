#ifndef APPFLID_PRINT_H
#define APPFLID_PRINT_H

#include "appflid/comm/log.h"

void appflid_print_hex(const char * s,unsigned int len){
            int i;
            for(i=0;i<len;i++){
                        if(i%16==0)
                                printk("\n");
            printk("%02x ",s[i]&0xff);
                }
            printk("\n");

}


void appflid_print_tuple(const struct tuple *tp)
{
        printk("%pI4#%hu->%pI4#%hu %hu\n",&tp->saddr,ntohs(tp->sport),
                                          &tp->daddr,ntohs(tp->dport),
                                          tp->l4num);
}

#endif
