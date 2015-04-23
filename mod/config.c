#include "appflid/comm/types.h"
#include "appflid/mod/config.h"

struct appflid_config *appconf;

struct appflid_config * config_get(void){
	return appconf;
}

int config_init(const char *user,const char *resrc,const char *confiledir){
	appconf = kmalloc(sizeof(struct appflid_config), GFP_ATOMIC);
	if(!appconf){
		log_err(ERR_ALLOC_FAILED,"Counld not alloc the appflid config");
		return -ENOMEM;
	}
	memset(appconf,0,sizeof(struct appflid_config));

	if(!user)
		strncpy(appconf->userif,USERIF_DEF,strlen(USERIF_DEF));
	else
		strncpy(appconf->userif,user,strlen(user));
    
	if(!resrc)
		strncpy(appconf->resrcif,RESRCIF_DEF,strlen(RESRCIF_DEF));
	else
		strncpy(appconf->resrcif,resrc,strlen(resrc));

	if(!confiledir)
		strncpy(appconf->confiledir,CONFILEDIR_DEF,strlen(CONFILEDIR_DEF));
	else
		strncpy(appconf->confiledir,confiledir,strlen(confiledir));
	log_info("Appflid config: userif=%s,resrcif=%s,confiledir=%s",appconf->userif,appconf->resrcif,appconf->confiledir);
	return 0;
}
void config_destroy(void){
	kfree(appconf);
}
