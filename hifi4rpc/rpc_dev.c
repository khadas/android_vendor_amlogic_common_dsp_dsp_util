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

int RPC_close(int handle)
{
	return close(handle);
}

void RPC_invoke(int handle, int cmd, void *data, unsigned int len)
{
	struct merge_data {
		int cmd;
		char msg[240];
	} merge_data;

	merge_data.cmd = cmd;
	memcpy(merge_data.msg, data, len);
	write(handle, &merge_data, sizeof(merge_data));
	memset(data, 0, len);
	read(handle, data, len);
}
