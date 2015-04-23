#ifndef APPFLID_NL_UCMD_H
#define APPFLID_NL_UCMD_H

#include "appflid/comm/types.h"

int recv_from_kernel(void);
int send_to_kernel(struct arguments *arg);
int nl_ucmd_init(void);
void nl_ucmd_destroy(void);

#endif
