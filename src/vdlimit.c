
/*
** (c) 2004 Herbert Poetzl
**
** V0.01	ioctls so far
**
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "vserver.h"
#include "vserver-internal.h"
#include "dlimit.h"
 
#ifdef O_LARGEFILE
#define OPEN_FLAGS (O_RDONLY|O_NONBLOCK|O_LARGEFILE)
#else
#define OPEN_FLAGS (O_RDONLY|O_NONBLOCK)
#endif

static	char	err_flag = 0;

static	char	opt_xid = 0;
static	char	opt_flag = 0;
static	char	opt_vers = 0;
static	char	opt_add = 0;
static	char	opt_rem = 0;
static	char	opt_set = 0;

static	char *	cmd_name = NULL;
static	char *	set_arg = NULL;

static	int 	num_xid = 0;
static	int 	num_flag = 0;




#define OPTIONS "+hadf:x:S:V"

int	main(int argc, char *argv[])
{
        extern int optind;
        extern char *optarg;
        char c, errflg = 0;
	int r;


        cmd_name = argv[0];
        while ((c = getopt(argc, argv, OPTIONS)) != EOF) {
            switch (c) {
            case 'h':
                fprintf(stderr,
                    "This is %s " VERSION "\n"
                    "options are:\n"
                    "-h        print this help message\n"
                    "-a        add dlimit entry\n"
                    "-d        delete dlimit entry\n"
                    "-f <num>  flag value in decimal\n"
                    "-x <num>  context id\n"
                    "-S <vals> current/limit values\n"
                    "-V        verify interface version\n"
                    "--        end of options\n"
                    ,cmd_name);
                exit(0);
                break;
            case 'a':
                opt_add = 1;
                break;
            case 'd':
                opt_rem = 1;
                break;
            case 'f':
                num_flag = atol(optarg);
                opt_flag = 1;
                break;
            case 'x':
                num_xid = atol(optarg);
                opt_xid = 1;
                break;
            case 'S':
                set_arg = optarg;
                opt_set = 1;
                break;
            case 'V':
                opt_vers = 1;
                break;
            case '?':
            default:
                errflg++;
                break;
            }
        }
        if (errflg) {
            fprintf(stderr, 
                "Usage: %s -[" OPTIONS "] <path> ...\n"
                "%s -h for help.\n",
                cmd_name, cmd_name);
            exit(2);
        }

	if (opt_vers) {
	    r = vc_get_version();
	    if (r<0)
	    	perror("vc_get_version");
    	    else 
	    	printf("version: %04x:%04x\n", 
		    (r>>16)&0xFFFF, r&0xFFFF);
	}

        for (; optind < argc; optind++) {
	    struct vcmd_ctx_dlimit_base_v0 init;
	    struct vcmd_ctx_dlimit_v0 data;

	    init.name = argv[optind];
	    init.flags = num_flag;

	    if (opt_rem) {
		r = vserver(VCMD_rem_dlimit, num_xid, &init);
		if (r<0)
		    perror("vc_rem_dlimit");
	    } 
	    if (opt_add) {
		r = vserver(VCMD_add_dlimit, num_xid, &init);
		if (r<0)
		    perror("vc_add_dlimit");
	    }
		    
    	    memset(&data, 0, sizeof(data));
	    data.name = argv[optind];
	    data.flags = num_flag;

	    if (opt_set) {
	    	sscanf(set_arg, "%u,%u,%u,%u,%u",
	    	    &data.space_used, &data.space_total,
	    	    &data.inodes_used, &data.inodes_total,
	    	    &data.reserved);

	    	r = vserver(VCMD_set_dlimit, num_xid, &data);
	    	if (r<0)
	    	    perror("vc_set_dlimit");
	    }

    	    memset(&data, 0, sizeof(data));
	    data.name = argv[optind];
	    data.flags = num_flag;

	    r = vserver(VCMD_get_dlimit, num_xid, &data);
	    if (r<0)
	    	perror("vc_get_dlimit");

	    printf("%s: %u,%u,%u,%u,%u\n", argv[optind],
	    	data.space_used, data.space_total,
	    	data.inodes_used, data.inodes_total,
	    	data.reserved);
	}
	
	exit((err_flag)?1:0);
}

