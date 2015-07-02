#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <string.h>
#include <sys/types.h> 

#include "appflid/comm/types.h"
#include "appflid/usr/nl_ucmd.h"

const char *argp_program_version = "APPFLID userspace app 0.1";
const char *argp_program_bug_address = "<philar_law@163.com>";

/**
 * The parameters received from the user, formatted and ready to be read in any order.
 */
/*struct arguments {
	__u16 key;
    __u32 operation;
	
};*/

/**
 * The flags the user can write as program parameters.
 */
/*enum argp_flags {
	ARGP_ALL = 'A',
	ARGP_COUNT = 'c',

	ARGP_HTTP = 1000,
	ARGP_DNS = 1001,

};*/

/*
 * OPTIONS. Field 1 in ARGP.
 * Order of fields: { NAME, KEY, ARG, FLAGS, DOC }.
 */
static struct argp_option options[] =
{
	{ NULL, 0, NULL, 0, "options:", 10},
	{ "A",	ARGP_ALL,	NULL, 0, "show all the count of app ." },
	{ "F",	ARGP_FLUSH,	NULL, 0, "flush all the count of app ." },
	{ "count",	ARGP_COUNT,	NULL, 0, "show the count " },
	{ "http",	ARGP_HTTP,NULL, 0, "show the app http" },
	{ "dns",	ARGP_DNS,NULL, 0, "show the app dns" },

	{ NULL },
};

/*
 * PARSER. Field 2 in ARGP.
 */
static int parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	int error = 0;
//	printf("parse_opt arg:%s,key=%d\n",arg,key);
	switch (key) {
	case ARGP_ALL:
		arguments->key=ARGP_ALL;
		break;
	case ARGP_FLUSH:
		arguments->key=ARGP_FLUSH;
		break;
	case ARGP_COUNT:
		arguments->operation=ARGP_COUNT;
		break;
	case ARGP_HTTP:
		arguments->key=ARGP_HTTP;
		break;
	case ARGP_DNS:
		arguments->key=ARGP_DNS;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return error;
}

/*
 * ARGS_DOC. Field 3 in ARGP.
 * A description of the non-option command-line arguments we accept.
 */
static char args_doc[] = "";

/*
 * DOC. Field 4 in ARGP.
 * Program documentation.
 */
static char doc[] = "appflid--the appflid module  configuration interface.\v";


/**
 * Uses argp.h to read the parameters from the user, validates them, and returns the result as a
 * structure.
 */
static struct argp argp = { options, parse_opt, args_doc, doc };



/*
 * The main function.
 */
int main(int argc, char **argv)
{
	struct arguments args;
	int error;

	memset(&args,0,sizeof(struct arguments));
	error = argp_parse(&argp, argc, argv, 0, NULL, &args);
	if (error != 0)
		return error;
	
	error = nl_ucmd_init();
	if(error!=0)
		return error;

	send_to_kernel(&args);
	recv_from_kernel();
/*	switch (args.key) {
	case ARGP_ALL:
		printf("you send the A \n");	
		break;
	case ARGP_HTTP:
	case ARGP_DNS:
		switch(args.operation){
			case ARGP_COUNT:
				printf("you need to count  the %d\n",args.key);
				break;
			default:
				printf("you need the detail of %d\n",args.key);
		}
		break;*/
/*	case ARGP_TMDEAD:
		printf("set tmDead :%u\n",args.tmDead);
               	break;
	case ARGP_TMSTABLE:
		printf("set tmStable :%u\n",args.tmStable);
                break;
	case ARGP_MCIP4:
		printf("set mcip4 :%s\n",inet_ntop(AF_INET,&args.mcIP4,buf,32));
		break;
	case ARGP_MCIP6:
		printf("set mcip6 :%s\n",inet_ntop(AF_INET6,&args.mcIP6,buf,128));
		break;
	case ARGP_SAIP4:
		switch(args.operation){
		case ARGP_ADD:
			printf("add saIP4:%s\n",inet_ntop(AF_INET,&args.saIP4,buf,32));
			break;
		case ARGP_REMOVE:
			printf("remove saIP4:%s\n",inet_ntop(AF_INET,&args.saIP4,buf,32));
			break;
		}
		break;
	case ARGP_SAIP6:
		switch(args.operation){
		case ARGP_ADD:
			printf("add saIP6:%s\n",inet_ntop(AF_INET6,&args.saIP6,buf,128));
			break;
		case ARGP_REMOVE:
			printf("remove saIP6:%s\n",inet_ntop(AF_INET6,&args.saIP6,buf,128));
			break;
		}
		break;*/
/*	default:
		log_debug("Command seems empty; --help or --usage for info.");
	}*/
	nl_ucmd_destroy();
	return 0;
}
