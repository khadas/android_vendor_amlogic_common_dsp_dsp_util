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
 * XAF STREAM API
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "aipc_type.h"
#include "ipc_cmd_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_shm.h"
#include "aml_flatbuf_api.h"
#include "aml_pcm_api.h"
#include "generic_macro.h"

typedef struct AML_PCM_T_ {
    tAcodecPcmSrvHdl hRpcPcm;
    AML_MEM_HANDLE hShm;
    int aipc;
} AML_PCM_T;

static void internal_pcm_close(AML_PCM_HANDLE Handle)
{
    AML_PCM_T* pPcm = (AML_PCM_T*)Handle;
    pcm_close_st closeArg;

    if (pPcm->hRpcPcm) {
        memset(&closeArg, 0, sizeof(closeArg));
        closeArg.pcm = pPcm->hRpcPcm;
        xAIPC(pPcm->aipc, MBX_AML_PCM_CLOSE, &closeArg, sizeof(closeArg));
    }

    xAudio_Ipc_Deinit(pPcm->aipc);
    AML_MEM_Free(pPcm->hShm);

    free(pPcm);
}

AML_PCM_HANDLE AML_PCM_Open(unsigned int card, unsigned int device, unsigned int flags,
                                        const struct aml_pcm_config *config)
{
    AMX_UNUSED(device);
    AMX_UNUSED(flags);
    AML_PCM_T* pPcm = malloc(sizeof(AML_PCM_T));
    if (!pPcm) {
        printf("Failed to malloc pcm context\n");
        goto aml_pcm_open_recycle;
    }
    memset(pPcm, 0, sizeof(AML_PCM_T));

    pPcm->aipc = xAudio_Ipc_Init(card);
    if (pPcm->aipc == -1) {
        printf("Failed to init rpc handle\n");
        goto aml_pcm_open_recycle;
    }

    pPcm->hShm = AML_MEM_Allocate(CHUNK_BYTES);
    if (!pPcm->hShm) {
        printf("Falied to allocate shared memory\n");
        goto aml_pcm_open_recycle;
    }

    pcm_open_st openArg;
    memset(&openArg, 0, sizeof(openArg));
    openArg.card = card;
    openArg.pcm_config.channels = config->channels;
    openArg.pcm_config.rate = config->rate;
    openArg.pcm_config.format = config->format;//format setting is hard code in dsp side
    openArg.pcm_config.period_count = config->period_count;
    openArg.pcm_config.period_size = config->period_size;
    xAIPC(pPcm->aipc, MBX_AML_PCM_OPEN, &openArg, sizeof(openArg));
    pPcm->hRpcPcm = openArg.out_pcm;
    if (!pPcm->hRpcPcm) {
        printf("Falied to create pcm handle\n");
        goto aml_pcm_open_recycle;      
    }

    return (AML_PCM_HANDLE)pPcm;
aml_pcm_open_recycle:
    internal_pcm_close((AML_PCM_HANDLE)pPcm);
    return NULL;
}

void AML_PCM_Close(AML_PCM_HANDLE Handle)
{
    internal_pcm_close(Handle);
}

int AML_PCM_Read(AML_PCM_HANDLE Handle, void *data, unsigned int count)
{
    AML_PCM_T* pPcm = (AML_PCM_T*)Handle;

    pcm_io_st ioArg;
    memset(&ioArg, 0, sizeof(ioArg));
    ioArg.pcm = pPcm->hRpcPcm;
    ioArg.data = (xpointer)AML_MEM_GetPhyAddr(pPcm->hShm);

    char* p = (char*)data;
    unsigned int uRemained = count;
    unsigned int chunkSize = 0;
    while(uRemained) {
        chunkSize = AMX_MIN(CHUNK_BYTES, uRemained);
        ioArg.count = chunkSize;
        xAIPC(pPcm->aipc, MBX_AML_PCM_READ, &ioArg, sizeof(ioArg));
        AML_MEM_Invalidate(AML_MEM_GetPhyAddr(pPcm->hShm), ioArg.out_ret);
        memcpy(p, AML_MEM_GetVirtAddr(pPcm->hShm), ioArg.out_ret);
        p += ioArg.out_ret;
        uRemained -= ioArg.out_ret;
    }

    return count;
}

