#include "appflid/comm/types.h"
#include "appflid/mod/config.h"
#include "appflid/mod/kfile.h"

char * read_confile(const char *filename){
	char *buf = NULL;
	struct file* f = NULL;
	off_t fsize;
	char filepath[255]={};
	
	log_debug("start to read the config file ......");
	if(!filename){
		log_err(ERR_NULL,"can not handle the NUll filename");
		return NULL;
	}
	strcpy(filepath,config_get()->confiledir);
	strcat(filepath,filename);
    f = file_open(filepath, O_RDONLY, 0); 
    if (f == NULL){
			log_err(ERR_OPEN_FILE,"open file %s  failed",filepath);
			return NULL;
	}

	fsize = f->f_dentry->d_inode->i_size;
	
	buf = (char*)kmalloc(fsize+1, GFP_ATOMIC);
	file_read(f,0,buf,fsize);
    file_close(f);
	return buf;
}


