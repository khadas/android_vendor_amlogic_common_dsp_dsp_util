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
 * offload aac api implmentation
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <rpc_client_aipc.h>
#include "rpc_client_aac.h"
#include "rpc_client_shm.h"
#include "aipc_type.h"

struct tAmlAacCtx {
	tAmlAacDecRpcHdl aacsrvhdl;
	int aipchdl;
	AML_MEM_HANDLE hShmIn;
	AML_MEM_HANDLE hShmOut;
	AML_MEM_HANDLE hConf;
	tAmlAacInitCtx InitCtx;
};

static AAC_DECODER_ERROR aac_decoding_exec(tAmlAacDecHdl hAacDec,
									void* aac_input, uint32_t aac_input_size,
									void* pcm_out, uint32_t* pcm_out_size,
									uint32_t* aac_input_left,tAmlAacOutputCtx* out_ctx)
{
	aacdec_decode_st arg;
	struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)hAacDec;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)pAmlAacCtx->aacsrvhdl;
	arg.input_buf = (xpointer)aac_input;
	arg.input_size = aac_input_size;
	arg.output_buf = (xpointer)pcm_out;
	arg.output_size = *pcm_out_size;
	xAIPC(pAmlAacCtx->aipchdl, MBX_CODEC_AACDEC_API_DECODE, &arg, sizeof(arg));
	*aac_input_left = arg.input_left_size;
	*pcm_out_size = arg.output_size;
	memcpy(out_ctx->chmask,arg.out_ctx.channel_counts, sizeof(out_ctx->chmask));
	out_ctx->channelNum = arg.out_ctx.channels;
	out_ctx->frameSize = arg.out_ctx.frameSize;
	out_ctx->sampleRate = arg.out_ctx.sampleRate;
	out_ctx->bitDepth = 16;
	return arg.ret;
}



tAmlAacDecHdl AmlACodecInit_AacDec(tAmlAacInitCtx *ctx)
{
    aacdec_init_st arg;
    struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)malloc(sizeof(struct tAmlAacCtx));
    memset(pAmlAacCtx, 0, sizeof(struct tAmlAacCtx));
    pAmlAacCtx->aipchdl = xAudio_Ipc_init();
    memset(&arg, 0, sizeof(arg));
    arg.transportFmt = ctx->transportFmt;
    arg.nrOfLayers = ctx->nrOfLayers;
    xAIPC(pAmlAacCtx->aipchdl, MBX_CODEC_AACDEC_API_INIT, &arg, sizeof(arg));
    pAmlAacCtx->aacsrvhdl = arg.hdl;
    if (pAmlAacCtx->aacsrvhdl) {
        pAmlAacCtx->hShmIn = AML_MEM_Allocate(AAC_INPUT_SIZE);
        pAmlAacCtx->hShmOut = AML_MEM_Allocate(PCM_OUTPUT_SIZE);
        pAmlAacCtx->InitCtx.transportFmt = ctx->transportFmt;
        pAmlAacCtx->InitCtx.nrOfLayers = ctx->nrOfLayers;
        pAmlAacCtx->hConf =  AML_MEM_Allocate(ctx->nrOfLayers*MAX_CONFRAW_LENGTH);
    } else {
        free(pAmlAacCtx);
        pAmlAacCtx = NULL;
    }
    return (void*)pAmlAacCtx;
}

void AmlACodecDeInit_AacDec(tAmlAacDecHdl hAacDec)
{
	aacdec_deinit_st arg;
	struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)hAacDec;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)pAmlAacCtx->aacsrvhdl;
	xAIPC(pAmlAacCtx->aipchdl, MBX_CODEC_AACDEC_API_DEINIT, &arg, sizeof(arg));
	xAudio_Ipc_Deinit(pAmlAacCtx->aipchdl);
	AML_MEM_Free(pAmlAacCtx->hShmIn);
	pAmlAacCtx->hShmIn = 0;
	AML_MEM_Free(pAmlAacCtx->hShmOut);
	pAmlAacCtx->hShmOut = 0;
	AML_MEM_Free(pAmlAacCtx->hConf);
	pAmlAacCtx->hConf = 0;
	free(pAmlAacCtx);
}

AAC_DECODER_ERROR AmlACodecExec_UserAllocIoShm_AacDec(tAmlAacDecHdl hAacDec,
													void* aac_input, uint32_t aac_input_size,
													void* pcm_out, uint32_t* pcm_out_size,
													uint32_t* aac_input_left, tAmlAacOutputCtx* out_ctx)
{
	return aac_decoding_exec(hAacDec,
							aac_input, aac_input_size,
							pcm_out, pcm_out_size,
							aac_input_left, out_ctx);
}

AAC_DECODER_ERROR  AmlACodecExec_AacDec(tAmlAacDecHdl hAacDec,
											void* aac_input, uint32_t aac_input_size,
											void* pcm_out, uint32_t* pcm_out_size,
											uint32_t* aac_input_left, tAmlAacOutputCtx* out_ctx)
{
	AAC_DECODER_ERROR ret;

	struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)hAacDec;
	void* pVirIn = AML_MEM_GetVirtAddr(pAmlAacCtx->hShmIn);
	void* pVirOut = AML_MEM_GetVirtAddr(pAmlAacCtx->hShmOut);
	void* pPhyIn = AML_MEM_GetPhyAddr(pAmlAacCtx->hShmIn);
	void* pPhyOut = AML_MEM_GetPhyAddr(pAmlAacCtx->hShmOut);

	if (aac_input_size <= AAC_INPUT_SIZE) {
		memcpy(pVirIn, aac_input, aac_input_size);
		AML_MEM_Clean(pPhyIn, aac_input_size);
	}
	else {
		printf("too large input frame:%d\n", aac_input_size);
		memcpy(pVirIn, aac_input, AAC_INPUT_SIZE);
		AML_MEM_Clean(pPhyIn, AAC_INPUT_SIZE);
	}

	ret = aac_decoding_exec(hAacDec,
							pPhyIn, aac_input_size,
							pPhyOut, pcm_out_size,
							aac_input_left, out_ctx);

	if (*pcm_out_size <= PCM_OUTPUT_SIZE) {
		AML_MEM_Invalidate(pPhyOut, *pcm_out_size);
		memcpy(pcm_out, pVirOut, *pcm_out_size);
	}
	else {
		printf("too large output frame:%d\n", *pcm_out_size);
		AML_MEM_Invalidate(pPhyOut, PCM_OUTPUT_SIZE);
		memcpy(pcm_out, pVirOut, PCM_OUTPUT_SIZE);
	}
	return ret;
}

AAC_DECODER_ERROR AmlACodecSetParam(tAmlAacDecHdl hAacDec, int32_t param, int32_t value)
{
    aacdec_setparam_st arg;
    struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)hAacDec;
    memset(&arg, 0, sizeof(arg));
    arg.hdl = (tAmlMp3DecRpcHdl)pAmlAacCtx->aacsrvhdl;
    arg.param = param;
    arg.value = value;
    xAIPC(pAmlAacCtx->aipchdl, MBX_CODEC_AACDEC_API_SETPARAM, &arg, sizeof(arg));
    printf("param=0x%x, value=%d, arg.ret=0x%x\n", param, value, arg.ret);
    return arg.ret;
}


AAC_DECODER_ERROR AmlACodecAncInit(tAmlAacDecHdl hAacDec, void* buf, uint32_t size)
{
	aacdec_ancinit_st arg;
	struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)hAacDec;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)pAmlAacCtx->aacsrvhdl;
	arg.buf = (xpointer)buf;
	arg.size = size;
	xAIPC(pAmlAacCtx->aipchdl, MBX_CODEC_AACDEC_API_ANCINIT, &arg, sizeof(arg));
	return arg.ret;
}


AAC_DECODER_ERROR AmlACodecConfigRaw(tAmlAacDecHdl hAacDec, char** conf, uint32_t* length)
{
	int i;
	aacdec_cfgraw_st arg;
	struct tAmlAacCtx* pAmlAacCtx = (struct tAmlAacCtx*)hAacDec;
	char* dst_conf = AML_MEM_GetVirtAddr(pAmlAacCtx->hConf);
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)pAmlAacCtx->aacsrvhdl;
	arg.num_conf = pAmlAacCtx->InitCtx.nrOfLayers;
	if (arg.num_conf > MAX_NUM_CONF) {
		printf("Too large number of config\n");
		arg.num_conf = MAX_NUM_CONF;
	}
	arg.conf = (xpointer)AML_MEM_GetPhyAddr(pAmlAacCtx->hConf);
	for (i = 0; i < arg.num_conf; i++) {
		memcpy((void*)dst_conf, (void*)conf[i], length[i]);
		dst_conf += MAX_CONFRAW_LENGTH;
		arg.length[i] = length[i];
	}
	AML_MEM_Clean(pAmlAacCtx->hConf,pAmlAacCtx->InitCtx.nrOfLayers*MAX_CONFRAW_LENGTH);
	xAIPC(pAmlAacCtx->aipchdl, MBX_CODEC_AACDEC_API_CFGRAW, &arg, sizeof(arg));
	return arg.ret;
}


