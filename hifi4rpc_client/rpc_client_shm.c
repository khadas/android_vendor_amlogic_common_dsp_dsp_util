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
#include <unistd.h>
#include <pthread.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "hifi4dsp_api.h"

struct _ACodecShmPoolInfo_t_ {
    int rpchdl;
    int fd;
    void* ShmVirBase;
    void* ShmPhyBase;
    size_t size;
    pthread_mutex_t mutex;
};

struct _ACodecShmPoolInfo_t_ gACodecShmPoolInfo[2] = {
    {
        .rpchdl = -1,
        .fd = -1,
        .ShmVirBase = 0,
        .ShmPhyBase = 0,
        .size = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
    },
    {
        .rpchdl = -1,
        .fd = -1,
        .ShmVirBase = 0,
        .ShmPhyBase = 0,
        .size = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
    },
};

static int Aml_Address2DspId(void* phy, size_t size)
{
    if ((uint8_t*)phy >= (uint8_t*)gACodecShmPoolInfo[0].ShmPhyBase &&
        ((uint8_t*)phy + size) < ((uint8_t*)gACodecShmPoolInfo[0].ShmPhyBase + gACodecShmPoolInfo[0].size)) {
        return 0;
    } else if ((uint8_t*)phy >= (uint8_t*)gACodecShmPoolInfo[1].ShmPhyBase &&
        ((uint8_t*)phy + size) < ((uint8_t*)gACodecShmPoolInfo[1].ShmPhyBase + gACodecShmPoolInfo[1].size)) {
        return 1;
    } else {
        return -1;
    }
}

static int Aml_ACodecMemory_Init(int dsp_id)
{
    int ret = 0;
    struct hifi4dsp_info_t info;
    char dspDev[64];
    if (dsp_id != 0 && dsp_id != 1) {
        printf("Invalid dsp_id:%d\n", dsp_id);
        ret = -1;
        goto tab_end;
    }
    pthread_mutex_lock(&gACodecShmPoolInfo[dsp_id].mutex);
    if (gACodecShmPoolInfo[dsp_id].fd >= 0) {
        ret = 0;
        goto tab_end;
    }

    snprintf(dspDev, sizeof(dspDev), "/dev/hifi4dsp%d", dsp_id);
    gACodecShmPoolInfo[dsp_id].fd = open(dspDev, O_RDWR);
    if (gACodecShmPoolInfo[dsp_id].fd < 0)
    {
        printf("open fail:%s\n", strerror(errno));
        ret = -1;
        goto tab_end;
    }

    memset(&info, 0, sizeof(info));
    if ((ret = ioctl(gACodecShmPoolInfo[dsp_id].fd, HIFI4DSP_GET_INFO, &info)) < 0)
    {
        printf("ioctl invalidate cache fail:%s\n", strerror(errno));
        ret = -1;
        goto tab_end;
    }
    gACodecShmPoolInfo[dsp_id].ShmPhyBase = (void*)info.phy_addr;
    gACodecShmPoolInfo[dsp_id].size = info.size;

    gACodecShmPoolInfo[dsp_id].ShmVirBase =  mmap(NULL,
                                gACodecShmPoolInfo[dsp_id].size,
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                gACodecShmPoolInfo[dsp_id].fd, 0);

    printf("fd = %d, Vir: %p, Phy:%p, size:%zu\n",
    gACodecShmPoolInfo[dsp_id].fd, gACodecShmPoolInfo[dsp_id].ShmVirBase,
    gACodecShmPoolInfo[dsp_id].ShmPhyBase, gACodecShmPoolInfo[dsp_id].size);
    gACodecShmPoolInfo[dsp_id].rpchdl = xAudio_Ipc_init();
tab_end:
    pthread_mutex_unlock(&gACodecShmPoolInfo[dsp_id].mutex);

    return ret;
}

AML_MEM_HANDLE AML_MEM_Allocate(size_t size)
{
    acodec_shm_alloc_st arg;
    arg.size = size;
    arg.pid = getpid();
    /*By default HiFi4 shared memory is allocated from Hifi4A*/
    if (gACodecShmPoolInfo[0].rpchdl < 0) {
        if (Aml_ACodecMemory_Init(0)) {
            printf("Initialize audio codec shm pool fail\n");
            return NULL;
        }
    }
    xAIPC(gACodecShmPoolInfo[0].rpchdl, MBX_CMD_SHM_ALLOC, &arg, sizeof(arg));
    return (AML_MEM_HANDLE)arg.hShm;
}

void AML_MEM_Free(AML_MEM_HANDLE hShm)
{
    int dsp_id;
    acodec_shm_free_st arg;
    arg.hShm = (AML_MEM_HANDLERpc)hShm;
    dsp_id = Aml_Address2DspId(hShm, 0);
    xAIPC(gACodecShmPoolInfo[dsp_id].rpchdl, MBX_CMD_SHM_FREE, &arg, sizeof(arg));
}

void AML_MEM_Transfer(AML_MEM_HANDLE hDst, AML_MEM_HANDLE hSrc, size_t size)
{
    int dsp_id;
    acodec_shm_transfer_st arg;
    arg.hSrc = (AML_MEM_HANDLERpc)hSrc;
    arg.hDst = (AML_MEM_HANDLERpc)hDst;
    arg.size = size;
    dsp_id = Aml_Address2DspId(hDst, size);
    if (dsp_id != Aml_Address2DspId(hSrc, size))
        printf("Can not transfer memory corss HiFi cores\n");
    else
        xAIPC(gACodecShmPoolInfo[dsp_id].rpchdl, MBX_CMD_SHM_TRANSFER, &arg, sizeof(arg));
}


void AML_MEM_Recycle(int pid)
{
    acodec_shm_recycle_st arg;
    arg.pid = pid;
    xAIPC(gACodecShmPoolInfo[0].rpchdl, MBX_CMD_SHM_RECYCLE, &arg, sizeof(arg));
}

void* AML_MEM_GetVirtAddr(AML_MEM_HANDLE hShm)
{
    int dsp_id;
    void* pVir = NULL;
    dsp_id = Aml_Address2DspId(hShm, 0);
    if (gACodecShmPoolInfo[dsp_id].rpchdl < 0) {
        if (Aml_ACodecMemory_Init(dsp_id)) {
            printf("Initialize audio codec shm pool fail\n");
            return NULL;
        }
    }
    pVir = (void*)(((uint8_t*)hShm - (uint8_t*)gACodecShmPoolInfo[dsp_id].ShmPhyBase) + (uint8_t*)gACodecShmPoolInfo[dsp_id].ShmVirBase);
    return pVir;
}


void* AML_MEM_GetPhyAddr(AML_MEM_HANDLE hShm)
{
    return hShm;
}

int32_t AML_MEM_Clean(AML_MEM_HANDLE phy, size_t size)
{
    int dsp_id;
    int ret = 0;
    struct hifi4_shm_info_t info;
    info.addr = (long)phy;
    info.size = size;

    dsp_id = Aml_Address2DspId(phy, size);
    if (gACodecShmPoolInfo[dsp_id].rpchdl < 0) {
        if (Aml_ACodecMemory_Init(dsp_id)) {
            printf("Initialize audio codec shm pool fail\n");
            return -1;
        }
    }

    if ((ret = ioctl(gACodecShmPoolInfo[dsp_id].fd, HIFI4DSP_SHM_CLEAN, &info)) < 0)
    {
        printf("ioctl clean cache fail:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int32_t AML_MEM_Invalidate(AML_MEM_HANDLE phy, size_t size)
{
    int dsp_id;
    int ret = 0;
    struct hifi4_shm_info_t info;
    info.addr = (long)phy;
    info.size = size;
    dsp_id = Aml_Address2DspId(phy, size);
    if (gACodecShmPoolInfo[dsp_id].rpchdl < 0) {
        if (Aml_ACodecMemory_Init(dsp_id)) {
            printf("Initialize audio codec shm pool fail\n");
            return -1;
        }
    }

    if ((ret = ioctl(gACodecShmPoolInfo[dsp_id].fd, HIFI4DSP_SHM_INV, &info)) < 0)
    {
        printf("ioctl invalidate cache fail:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

AML_MEM_HANDLE AML_MEM_Import(void* phy, size_t size)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (gACodecShmPoolInfo[i].rpchdl < 0) {
            if (Aml_ACodecMemory_Init(i)) {
                printf("Initialize audio codec shm pool fail\n");
                return NULL;
            }
        }
        if (-1 != Aml_Address2DspId(phy, size))
            return (AML_MEM_HANDLE)phy;
    }
    return NULL;
}


