#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include "rpc_dev.h"
#include "ipc_cmd_type.h"

int main(int argc, char *argv[])
{
	int fd,size;
	char s[]="Data From ARM Data\n";
	int sel = 0;

	if (argc == 1 || argc > 2) {
		printf("Please use './hifi4_rpc_test 1' & or './hifi4_rpc_test 2'\n");
		printf("1 for ARM send data to DSP\n");
		printf("2 for Listen DSP send data to ARM\n");
		return -1;
	}

	sel = atoi(argv[1]);
	/*
	 * 1, open dev/dsp_dev
	 * 2, access right, support rw
	 * 3, access group, current not use
	 * */
	fd = RPC_init("/dev/dsp_dev",O_RDWR, 0);
	if (fd < 0)
		printf("error init device node\n");
	switch (sel) {
	case 1:
		/*
		 * 1, fd get from RPC_init
		 * 2, MBX_TEST_DEMO cmd defined in ipc_cmd_type.h,
		 *    use to regonize handle thing, rtos also need add this cmd
		 * 3, s, buffer base addr
		 * 4, size, max size is 240 bytes
		 * */
		RPC_invoke(fd, MBX_TEST_DEMO, s,sizeof(s));
		printf("Data %s\n", s);
		break;
	case 2:
		/*
		 * 1, fd get from RPC_init
		 * 2, MBX_LISTEN_DEMO cmd defined in ipc_cmd_type.h,
		 *    use to regonize handle thing, rtos also need add this cmd
		 * 3, s, buffer base addr
		 * 4, size, max size is 240 bytes
		 * */
		for(;;) {
			RPC_invoke(fd, MBX_LISTEN_DEMO, s,sizeof(s));
			printf("Data %s\n", s);
		}
		break;
	default:
		printf("Please use './hifi4_rpc_test 1' & or './hifi4_rpc_test 2'\n");
		printf("1 for ARM send data to DSP\n");
		printf("2 for Listen DSP send data to ARM\n");
		break;
	}
	return 0;
}
