#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include "rpc_dev.h"
#include "ipc_cmd_type.h"

#define	CMD_HIFI_DSPA	0x1
#define CMD_HIFI_DSPB	0x2

extern char *optarg;
extern int optind, opterr, optopt;
int lopt;
static char optString[]="d:12h";

static struct option longOpts[] = {
	{ "dsp", required_argument, NULL, 'd' },
	{ "test-1", no_argument, NULL, '1' },
	{ "test-2", no_argument, NULL, '2' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

void showUsage() {
	printf("Usage:[options]\n");
	printf(" -d, --dsp=DSPNAME    The dspname, hifi4a or hifi4b\n");
	printf(" --test-1             Test 1\n");
	printf(" --test-2             Test 2\n");
	printf(" -h, --help           Print this message and exit.\n");
}

int dsp_dev_is_exist(char *path)
{
	if (access(path,F_OK) != -1) {
		printf("dsp dev %s is exist\n", path);
	} else {
		printf("dsp dev %s is not exist\n", path);
		return -1;
	}
	return 0;
}

int do_cmd(int cmd, int digit)
{
	int fd;
	char path[64];

	switch (cmd) {
	case CMD_HIFI_DSPA:
		sprintf(path, "/dev/dsp_dev");
	break;
	case CMD_HIFI_DSPB:
		sprintf(path, "/dev/dspb_dev");
	break;
	default:
	break;
	}

	if (dsp_dev_is_exist(path) < 0)
		return -1;
	/*
	 * 1, open /dev/dsp_dev or /dev/dspb_dev
	 * 2, access right, support rw
	 * 3, access group, current not use
	 * */
	fd = RPC_init(path, O_RDWR, 0);

	switch (digit) {
	case '1':
		/*
		 * 1, fd get from RPC_init
		 * 2, MBX_TEST_DEMO cmd defined in ipc_cmd_type.h,
		 *    use to regonize handle thing, rtos also need
		 *    add this cmd
		 * 3, s, buffer base addr
		 * 4, size, max size is 240 bytes
		 * */
		RPC_invoke(fd, MBX_TEST_DEMO, path, sizeof(path));
		printf("Data %s\n", path);
	break;
	case '2':
		for (;;) {
			/*
			 * 1, fd get from RPC_init
			 * 2, MBX_LISTEN_DEMO cmd defined in ipc_cmd_type.h,
			 *    use to regonize handle thing, rtos also need
			 *    add this cmd
			 * 3, s, buffer base addr
			 * 4, size, max size is 240 bytes
			 * */
			RPC_invoke(fd, MBX_LISTEN_DEMO, path, sizeof(path));
			printf("Data %s\n", path);
		}
	break;
	default:
	break;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int c;
	int digit_optind = 0;
	int cmd = 0;
	int option_index;

	if (argc < 2) {
		showUsage();
		return -1;
	}

	while (1) {
		int this_option_optind = optind ? optind : 1;
		c = getopt_long(argc, argv, optString,
				longOpts, &option_index);
		if (c < 0)
			break;

		switch (c) {
		case 'd':
			printf("dspname %s\n", optarg);
			if (!strcmp(optarg, "hifi4a")) {
				cmd |= CMD_HIFI_DSPA;
			} else if (!strcmp(optarg, "hifi4b")) {
				cmd |= CMD_HIFI_DSPB;
			} else {
				printf("not support dsp name\n");
				showUsage();
				return 0;
			}
		break;
		case '1':
		case '2':
			if (digit_optind != 0) {
				printf("two digit\n");
				break;
			}
			printf("option %c\n", c);
			digit_optind = c;
		break;
		case 'h':
			showUsage();
			return 0;
		break;
		default:
			showUsage();
			return -1;
		break;
		};
	}

	do_cmd(cmd, digit_optind);
	return 0;
}
