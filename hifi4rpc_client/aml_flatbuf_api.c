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
 * flat buffer api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "aipc_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_shm.h"
#include "aml_flatbuf_api.h"
#define UNUSED(x) (void)(x)

typedef struct _FLATBUFS {
    int aipchdl;
    tAmlFlatBufHdlRpc hFbuf;
    AML_MEM_HANDLE hShm;
} FLATBUFS;

AML_FLATBUF_HANDLE AML_FLATBUF_Create(char* buf_id, int flags,
                                    struct flatbuffer_config* config)
{
    aml_flatbuf_create_st arg;
    FLATBUFS * pFbufCtx = (FLATBUFS*)malloc(sizeof(FLATBUFS));
    memset(pFbufCtx, 0, sizeof(FLATBUFS));
    strcpy(arg.buf_id, buf_id);
    arg.flags = flags;
    arg.phy_addr = (xpointer)config->phy_addr;
    arg.size = config->size;
    pFbufCtx->aipchdl = xAudio_Ipc_init();
    xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_CREATE, &arg, sizeof(arg));
    pFbufCtx->hFbuf = arg.hFbuf;

    /*Allocate intermediate shm*/
    aml_flatbuf_size_st arg1;
    arg1.hFbuf = pFbufCtx->hFbuf;
    xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_GETBUFSIZE, &arg1, sizeof(arg1));
    printf("bufsize:0x%x\n", (size_t)arg1.size);
    pFbufCtx->hShm = AML_MEM_Allocate((size_t)arg1.size);

    return pFbufCtx;
}

void AML_FLATBUF_Destory(AML_FLATBUF_HANDLE hFbuf)
{
    aml_flatbuf_destory_st arg;
    FLATBUFS * pFbufCtx = (FLATBUFS*)hFbuf;

    arg.hFbuf = (tAmlFlatBufHdlRpc)pFbufCtx->hFbuf;
    xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_DESTORY, &arg, sizeof(arg));
    xAudio_Ipc_Deinit(pFbufCtx->aipchdl);
    if (pFbufCtx->hShm)
        AML_MEM_Free(pFbufCtx->hShm);
    free(pFbufCtx);
}

size_t AML_FLATBUF_Read(AML_FLATBUF_HANDLE hFbuf, void* buf, size_t size)
{
    aml_flatbuf_read_st arg;
    FLATBUFS * pFbufCtx = (FLATBUFS*)hFbuf;

    arg.hFbuf = (tAmlFlatBufHdlRpc)pFbufCtx->hFbuf;
    arg.mem = (xpointer)AML_MEM_GetPhyAddr(pFbufCtx->hShm);
    arg.size = size;
	xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_READ, &arg, sizeof(arg));
    AML_MEM_Invalidate(pFbufCtx->hShm, arg.size);
    memcpy(buf, AML_MEM_GetVirtAddr(pFbufCtx->hShm), arg.size);

    return arg.size;
}


size_t AML_FLATBUF_Write(AML_FLATBUF_HANDLE hFbuf, void* buf, size_t size)
{
    aml_flatbuf_write_st arg;
    FLATBUFS * pFbufCtx = (FLATBUFS*)hFbuf;

    arg.hFbuf = (tAmlFlatBufHdlRpc)pFbufCtx->hFbuf;
    arg.mem = (xpointer)AML_MEM_GetPhyAddr(pFbufCtx->hShm);
    arg.size = size;
    memcpy(AML_MEM_GetVirtAddr(pFbufCtx->hShm), buf, arg.size);
    AML_MEM_Clean(pFbufCtx->hShm, arg.size);
    xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_WRITE, &arg, sizeof(arg));

    return arg.size;
}

size_t AML_FLATBUF_GetFullness(AML_FLATBUF_HANDLE hFbuf)
{
    aml_flatbuf_size_st arg;
    FLATBUFS * pFbufCtx = (FLATBUFS*)hFbuf;

    arg.hFbuf = (tAmlFlatBufHdlRpc)pFbufCtx->hFbuf;
    xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_GETFULLNESS, &arg, sizeof(arg));

    return arg.size;
}

size_t AML_FALTBUF_GetSpace(AML_FLATBUF_HANDLE hFbuf)
{
    aml_flatbuf_size_st arg;
    FLATBUFS * pFbufCtx = (FLATBUFS*)hFbuf;

    arg.hFbuf = pFbufCtx->hFbuf;
    xAIPC(pFbufCtx->aipchdl, MBX_CMD_FLATBUF_GETSPACE, &arg, sizeof(arg));

    return arg.size;
}

