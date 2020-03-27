#include<stdio.h>

int RPC_init(const char *path, int flags,mode_t mode);
int RPC_deinit(int handle);
void RPC_invoke(int handle, int cmd, void *data, unsigned int len);
