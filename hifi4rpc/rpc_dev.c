#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<string.h>
#include "rpc_dev.h"

int RPC_init(const char *path, int flags,mode_t mode)
{
	return open(path, flags, mode);
}

int RPC_deinit(int handle)
{
	return close(handle);
}
/*
 * rpc invoke
 * write or read error, ret < 0
 * write and read success, ret = msg len >= 0
 */
int RPC_invoke(int handle, int cmd, void *data, unsigned int len)
{
	int ret = 0;
	struct merge_data {
		int cmd;
		char msg[240];
	} merge_data;

	merge_data.cmd = cmd;
	memcpy(merge_data.msg, data, len);
	ret = write(handle, &merge_data, sizeof(merge_data));
	if (ret < 0) {
		printf("RPC invoke write error: %d\n", ret);
		return ret;
	}
	memset(data, 0, len);
	ret = read(handle, data, len);
	if (ret < 0) {
		printf("RPC invoke read error: %d\n", ret);
	}
	return ret;
}
