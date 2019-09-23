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
#include <pthread.h>
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
    pthread_mutex_t mutex;
};

struct _ACodecShmPoolInfo_t_ gACodecShmPoolInfo = {
    .rpchdl = -1,
    .fd = -1,
    .ShmVirBase = 0,
    .ShmPhyBase = 0,
    .size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

static int Aml_ACodecMemory_Init(void)
{
    int ret = 0;
    struct hifi4dsp_info_t info;
    pthread_mutex_lock(&gACodecShmPoolInfo.mutex);

    if (gACodecShmPoolInfo.fd >= 0) {
        ret = 0;
        goto tab_end;
    }

    gACodecShmPoolInfo.fd = open("/dev/hifi4dsp0", O_RDWR);
    if (gACodecShmPoolInfo.fd < 0)
    {
        printf("open fail:%s\n", strerror(errno));
        ret = -1;
        goto tab_end;
    }

    memset(&info, 0, sizeof(info));
    if ((ret = ioctl(gACodecShmPoolInfo.fd, HIFI4DSP_GET_INFO, &info)) < 0)
    {
        printf("ioctl invalidate cache fail:%s\n", strerror(errno));
        ret = -1;
        goto tab_end;
    }
    gACodecShmPoolInfo.ShmPhyBase = info.phy_addr;
    gACodecShmPoolInfo.size = info.size;

    gACodecShmPoolInfo.ShmVirBase =  mmap(NULL,
                                gACodecShmPoolInfo.size,
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                gACodecShmPoolInfo.fd, 0);

    gACodecShmPoolInfo.rpchdl = xAudio_Ipc_init();
    printf("fd = %d, Vir: %p, Phy:0x%lx, size:%zu\n",
    gACodecShmPoolInfo.fd, gACodecShmPoolInfo.ShmVirBase,
    gACodecShmPoolInfo.ShmPhyBase, gACodecShmPoolInfo.size);
tab_end:
    pthread_mutex_unlock(&gACodecShmPoolInfo.mutex);

    return ret;
}

AML_MEM_HANDLE AML_MEM_Allocate(size_t size)
{
    acodec_shm_alloc_st arg;
    arg.size = size;
    if (gACodecShmPoolInfo.fd < 0) {
        if(Aml_ACodecMemory_Init()) {
            printf("Initialize audio codec shm pool fail\n");
            return NULL;
        }
    }

    xAIPC(gACodecShmPoolInfo.rpchdl, MBX_CMD_SHM_ALLOC, &arg, sizeof(arg));
    return (AML_MEM_HANDLE)arg.hShm;
}

void AML_MEM_Free(AML_MEM_HANDLE hShm)
{
    acodec_shm_free_st arg;
    arg.hShm = (AML_MEM_HANDLERpc)hShm;
    xAIPC(gACodecShmPoolInfo.rpchdl, MBX_CMD_SHM_FREE, &arg, sizeof(arg));
}

void Aml_ACodecMemory_Transfer(AML_MEM_HANDLE hDst, AML_MEM_HANDLE hSrc, size_t size)
{
    acodec_shm_transfer_st arg;
    arg.hSrc = (AML_MEM_HANDLERpc)hSrc;
    arg.hDst = (AML_MEM_HANDLERpc)hDst;
    arg.size = size;
    xAIPC(gACodecShmPoolInfo.rpchdl, MBX_CMD_SHM_TRANSFER, &arg, sizeof(arg));
}


void* AML_MEM_GetVirtAddr(AML_MEM_HANDLE hShm)
{
    void* pVir = NULL;
    pVir = (void*)(((long)hShm - gACodecShmPoolInfo.ShmPhyBase) + (long)gACodecShmPoolInfo.ShmVirBase);
    return pVir;
}


void* AML_MEM_GetPhyAddr(AML_MEM_HANDLE hShm)
{
    return hShm;
}

uint32_t AML_MEM_Clean(AML_MEM_HANDLE phy, size_t size)
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

uint32_t AML_MEM_Invalidate(AML_MEM_HANDLE phy, size_t size)
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


