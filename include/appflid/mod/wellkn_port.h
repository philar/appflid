#ifndef APPFLID_WELLKN_PORT_H
#define APPFLID_WELLKN_PORT_H


struct wellkn_port_entry{
	struct rb_node tree_hook;
	__u16 port;
	char app_proto[APPNAMSIZ];
};
struct wellkn_port_table{
	    struct rb_root tree;
	    unsigned int  count;
};

int wellkn_port_add(const __u16 port,const char * app_name);
struct wellkn_port_entry *  wellkn_port_find(__u16 port);
void wellkn_port_show(void);
int wellkn_port_init(void);
void wellkn_port_destroy(void);

#endif


