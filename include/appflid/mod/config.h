#ifndef APPFLID_CONFIG_H
#define APPFLID_CONFIG_H

#include "appflid/comm/constants.h"

struct appflid_config{
	char userif[IFNAMSIZ];
	char resrcif[IFNAMSIZ];
	char confiledir[MAX_FILEPATH];

};


struct appflid_config * config_get(void);
int config_init(const char *user,const char *resrc,const char *confilepath);
void config_destroy(void);

#endif
