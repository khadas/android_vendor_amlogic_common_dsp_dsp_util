/*
 * Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * audio rpc shm api implementation
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "hifi4dsp_api.h"

struct _ACodecShmPoolInfo_t_ {
	int rpchdl;
	int fd;
	void* ShmVirBase;
	long ShmPhyBase;
	size_t size;
};

struct _ACodecShmPoolInfo_t_ gACodecShmPoolInfo = {
	.rpchdl = -1,
	.fd = -1,
	.ShmVirBase = 0,
	.ShmPhyBase = 0,
	.size = 0,
};

static int Aml_ACodecMemory_Init(void)
{
	int ret = 0;
	struct hifi4dsp_info_t info;
	gACodecShmPoolInfo.fd = open("/dev/hifi4dsp0", O_RDWR);
	if (gACodecShmPoolInfo.fd < 0)
	{
		printf("open fail:%s\n", strerror(errno));
		return -1;
	}

	memset(&info, 0, sizeof(info));
	if ((ret = ioctl(gACodecShmPoolInfo.fd, HIFI4DSP_GET_INFO, &info)) < 0)
	{
		printf("ioctl invalidate cache fail:%s\n", strerror(errno));
		return -1;
	}
	gACodecShmPoolInfo.ShmPhyBase = info.phy_addr;
	gACodecShmPoolInfo.size = info.size;

	gACodecShmPoolInfo.ShmVirBase =  mmap(NULL,
			gACodecShmPoolInfo.size, PROT_READ | PROT_WRITE, MAP_SHARED,
			gACodecShmPoolInfo.fd, 0);

	gACodecShmPoolInfo.rpchdl = xAudio_Ipc_init();

	printf("fd = %d, Vir: %p, Phy:%lu, size:%zu\n",
		gACodecShmPoolInfo.fd, gACodecShmPoolInfo.ShmVirBase,
		gACodecShmPoolInfo.ShmPhyBase, gACodecShmPoolInfo.size);
	return 0;
}

tAcodecShmHdl Aml_ACodecMemory_Allocate(size_t size)
{
	int ret = 0;
	acodec_shm_alloc_st arg;
	arg.size = size;
	if (gACodecShmPoolInfo.fd < 0) {		
		if(Aml_ACodecMemory_Init()) {
			printf("Initialize audio codec shm pool fail\n");
			return NULL;
		}		
	}
	
	xAIPC(gACodecShmPoolInfo.rpchdl, MBX_CMD_SHM_ALLOC, &arg, sizeof(arg));
	return (tAcodecShmHdl)arg.hShm;
}

void Aml_ACodecMemory_Free(tAcodecShmHdl hShm)
{
	acodec_shm_free_st arg;
	arg.hShm = (tAcodecShmHdlRpc)hShm;
	xAIPC(gACodecShmPoolInfo.rpchdl, MBX_CMD_SHM_FREE, &arg, sizeof(arg));
}

void Aml_ACodecMemory_Transfer(tAcodecShmHdl hDst, tAcodecShmHdl hSrc, size_t size)
{
	acodec_shm_transfer_st arg;
	arg.hSrc = (tAcodecShmHdlRpc)hSrc;
	arg.hDst = (tAcodecShmHdlRpc)hDst;
	arg.size = size;
	xAIPC(gACodecShmPoolInfo.rpchdl, MBX_CMD_SHM_TRANSFER, &arg, sizeof(arg));
}


void* Aml_ACodecMemory_GetVirtAddr(tAcodecShmHdl hShm)
{
	void* pVir = NULL;
	pVir = (void*)(((long)hShm - gACodecShmPoolInfo.ShmPhyBase) + (long)gACodecShmPoolInfo.ShmVirBase);
}


void* Aml_ACodecMemory_GetPhyAddr(tAcodecShmHdl hShm)
{
	return hShm;
}

uint32_t Aml_ACodecMemory_Clean(tAcodecShmHdl phy, size_t size)
{
	int ret = 0;
	struct hifi4_shm_info_t info;
	info.addr = (long)phy;
	info.size = size;
	if ((ret = ioctl(gACodecShmPoolInfo.fd, HIFI4DSP_SHM_CLEAN, &info)) < 0)
	{
		printf("ioctl clean cache fail:%s\n", strerror(errno));
		return -1;
	}
	return 0;
}

uint32_t Aml_ACodecMemory_Inv(tAcodecShmHdl phy, size_t size)
{
	int ret = 0;
	struct hifi4_shm_info_t info;
	info.addr = (long)phy;
	info.size = size;
	if ((ret = ioctl(gACodecShmPoolInfo.fd, HIFI4DSP_SHM_INV, &info)) < 0)
	{
		printf("ioctl invalidate cache fail:%s\n", strerror(errno));
		return -1;
	}
	return 0;
}


