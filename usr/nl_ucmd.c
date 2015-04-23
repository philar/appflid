#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>

#include "appflid/comm/constants.h"
#include "appflid/usr/nl_ucmd.h"


struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int nl_sk;
struct msghdr msg;

int recv_from_kernel(void){
	int recv_num;
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
        recv_num=recvmsg(nl_sk, &msg, 0);
        if(recv_num==-1){
                perror("recv_from kernel error");
        }else
               	printf("%s",(char *)NLMSG_DATA(nlh));

//	memcpy(hello,NLMSG_DATA(nl_ucmdh),recv_num);
	return 0;

}
int send_to_kernel(struct arguments *arg){
//	log_debug("send_to_kernel");
			
        memcpy(NLMSG_DATA(nlh),arg,sizeof(struct arguments));
        if(sendmsg(nl_sk,&msg,0)<0){
                perror("sendmsg to kernel fail ");
                return -1;
        }else{
//                printf("sendmsg to kernel success\n");
        }
        return 0;
	

}
int nl_ucmd_init(void){
	int err;
        nl_sk = socket(PF_NETLINK, SOCK_RAW,APPFLID_NETLINK);
	if(nl_sk<0){
		perror("socket to kernel failed,please check the module SOCS is up or not");
		return -1;
	}

        memset(&msg, 0, sizeof(msg));
        memset(&src_addr, 0, sizeof(src_addr));
        src_addr.nl_family = AF_NETLINK;
        src_addr.nl_pid = getpid();  /* self pid */
        src_addr.nl_groups = 0;  /* not in mcast groups */
	
        err=bind(nl_sk, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if(err<0){
		perror("bind to kernel failed");
		return -1;
	}
	
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.nl_family = AF_NETLINK;
        dest_addr.nl_pid = 0;   /* For Linux Kernel */
        dest_addr.nl_groups = 0; /* unicast */

        nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
        /* Fill the netlink message header */
        nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
        nlh->nlmsg_pid = getpid();  /* self pid */
        nlh->nlmsg_flags = 0;
        /* Fill in the netlink message payload */

        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
        msg.msg_name = (void *)&dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

	return 0;

}
void nl_ucmd_destroy(void){
	free(nlh);
	close(nl_sk);
}

