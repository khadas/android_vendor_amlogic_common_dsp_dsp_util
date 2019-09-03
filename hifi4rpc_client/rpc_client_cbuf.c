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
 * hifi cc buffer client api
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
#include "aipc_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_cbuf.h"
#include "rpc_client_shm.h"

struct tAmlCbufCtx {
	tAmlCBufHdlRpc cbufsrvhdl;
	int aipchdl;
    AML_MEM_HANDLE hWorkBuf;
    uint32_t cbuf_id;
    size_t size;
    size_t pad_size;
};

AML_CBUF_HANDLE AML_CBUF_Create(uint32_t cbuf_id, size_t size, size_t pad_size)
{
    aml_cbuf_create_st arg;
    struct tAmlCbufCtx* pAmlCbufCtx = (struct tAmlCbufCtx*)malloc(sizeof(struct tAmlCbufCtx));
    memset(pAmlCbufCtx, 0, sizeof(struct tAmlCbufCtx));
    pAmlCbufCtx->aipchdl = xAudio_Ipc_init();
    memset(&arg, 0, sizeof(arg));
    arg.cbuf_id = cbuf_id;
    arg.pad_size = pad_size;
    arg.size = size;
    xAIPC(pAmlCbufCtx->aipchdl, MBX_CMD_CCBUF_CREATE, &arg, sizeof(arg));
    if (!arg.hCbuf) {
        printf("Allocate Hifi CC buffer failed\n");
        free(pAmlCbufCtx);
        return NULL;
    }
    pAmlCbufCtx->cbufsrvhdl = arg.hCbuf;
    pAmlCbufCtx->hWorkBuf = AML_MEM_Allocate(size);
    pAmlCbufCtx->cbuf_id = cbuf_id;
    pAmlCbufCtx->size = size;
    pAmlCbufCtx->pad_size = pad_size;
    return pAmlCbufCtx;
}

void AML_CBUF_Destory(AML_CBUF_HANDLE hCbuf)
{
    aml_cbuf_destory_st arg;
    struct tAmlCbufCtx* pAmlCbufCtx = (struct tAmlCbufCtx*)hCbuf;
    memset(&arg, 0, sizeof(arg));
    arg.hCbuf = (tAmlCBufHdlRpc)pAmlCbufCtx->cbufsrvhdl;
    arg.cbuf_id = pAmlCbufCtx->cbuf_id;
    xAIPC(pAmlCbufCtx->aipchdl, MBX_CMD_CCBUF_DESTORY, &arg, sizeof(arg));
    AML_MEM_Free(pAmlCbufCtx->hWorkBuf);
    free(pAmlCbufCtx);
}


size_t AML_CBUF_Read(AML_CBUF_HANDLE hCbuf, void* buf, size_t size)
{
    aml_cbuf_read_st arg;
    struct tAmlCbufCtx* pAmlCbufCtx = (struct tAmlCbufCtx*)hCbuf;
    memset(&arg, 0, sizeof(arg));
    arg.hCbuf = (tAmlCBufHdlRpc)pAmlCbufCtx->cbufsrvhdl;
    arg.size = size;
    arg.mem = (xpointer)AML_MEM_GetPhyAddr(pAmlCbufCtx->hWorkBuf);
    xAIPC(pAmlCbufCtx->aipchdl, MBX_CMD_CCBUF_READ, &arg, sizeof(arg));
    if (arg.size) {
        AML_MEM_Invalidate(pAmlCbufCtx->hWorkBuf,arg.size);
        memcpy(buf, AML_MEM_GetVirtAddr(pAmlCbufCtx->hWorkBuf), arg.size);
    }
    return arg.size;
}




