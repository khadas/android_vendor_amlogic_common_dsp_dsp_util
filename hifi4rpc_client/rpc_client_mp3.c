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
 * offload mp3 api implmentation
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <rpc_client_aipc.h>
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"
#include "aipc_type.h"

struct tAmlMp3Ctx {
	tAmlMp3DecRpcHdl mp3rpchdl;
	int aipchdl;
	tAcodecShmHdl hShmIn;
	tAcodecShmHdl hShmOut;
};

static void mp3_sync_rpctype_config_to_local(tAmlACodecConfig_Mp3DecRpc* remote,
											tAmlACodecConfig_Mp3DecExternal* local) {
	local->bitRate = remote->bitRate;
	local->crcEnabled = remote->crcEnabled;
	local->CurrentFrameLength = remote->CurrentFrameLength,
	local->inputBufferCurrentLength = remote->inputBufferCurrentLength;
	local->inputBufferUsedLength = remote->inputBufferUsedLength;
	local->equalizerType = remote->equalizerType;
	local->inputBufferMaxLength = remote->inputBufferMaxLength;
	local->num_channels = remote->num_channels;
	local->version = remote->version;
	local->samplingRate = remote->samplingRate;
	local->outputFrameSize = remote->outputFrameSize;
	local->totalNumberOfBitsUsed = remote->totalNumberOfBitsUsed;
	local->pOutputBuffer = (uint16*)remote->pOutputBuffer;
	local->pInputBuffer = (uint8*)remote->pInputBuffer;
}

static void mp3_sync_local_config_to_rpctype(tAmlACodecConfig_Mp3DecExternal* local,
													tAmlACodecConfig_Mp3DecRpc* remote) {
	remote->bitRate = local->bitRate;
	remote->crcEnabled = local->crcEnabled;
	remote->CurrentFrameLength = local->CurrentFrameLength,
	remote->inputBufferCurrentLength = local->inputBufferCurrentLength;
	remote->inputBufferUsedLength = local->inputBufferUsedLength;
	remote->equalizerType = local->equalizerType;
	remote->inputBufferMaxLength = local->inputBufferMaxLength;
	remote->num_channels = local->num_channels;
	remote->version = local->version;
	remote->samplingRate = local->samplingRate;
	remote->outputFrameSize = local->outputFrameSize;
	remote->totalNumberOfBitsUsed = local->totalNumberOfBitsUsed;
	remote->pOutputBuffer = (xpointer)local->pOutputBuffer;
	remote->pInputBuffer = (xpointer)local->pInputBuffer;
}

tAmlMp3DecHdl AmlACodecInit_Mp3Dec(tAmlACodecConfig_Mp3DecExternal* pconfig, int bInplace)
{
	mp3_init_st arg;
	struct tAmlMp3Ctx* pAmlMp3Ctx = (struct tAmlMp3Ctx*)malloc(sizeof(struct tAmlMp3Ctx));
	memset(pAmlMp3Ctx, 0, sizeof(struct tAmlMp3Ctx));
	pAmlMp3Ctx->aipchdl = xAudio_Ipc_init();
	memset(&arg, 0, sizeof(arg));
	mp3_sync_local_config_to_rpctype(pconfig, &arg.config);
	//printf("size of config in arm:%u\n", sizeof(tAmlACodecConfig_Mp3DecRpc));
	xAIPC(pAmlMp3Ctx->aipchdl, MBX_CODEC_MP3_API_INIT, &arg, sizeof(arg));
	mp3_sync_rpctype_config_to_local(&arg.config, pconfig);
	pAmlMp3Ctx->mp3rpchdl = arg.hdl;
	if (!bInplace) {
		printf("Allocate codec shared memory\n");
 		pAmlMp3Ctx->hShmIn = Aml_ACodecMemory_Allocate(kInputBufferSize);
		pAmlMp3Ctx->hShmOut = Aml_ACodecMemory_Allocate(kOutputBufferSize);
	} else {
		pAmlMp3Ctx->hShmIn = 0;
		pAmlMp3Ctx->hShmOut = 0;
	}
	return (void*)pAmlMp3Ctx;
}

void AmlACodecDeInit_Mp3Dec(tAmlMp3DecHdl hMp3Dec)
{
	mp3_deinit_st arg;
	struct tAmlMp3Ctx* pAmlMp3Ctx = (struct tAmlMp3Ctx*)hMp3Dec;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)pAmlMp3Ctx->mp3rpchdl;
	xAIPC(pAmlMp3Ctx->aipchdl, MBX_CODEC_MP3_API_DEINIT, &arg, sizeof(arg));
	xAudio_Ipc_Deinit(pAmlMp3Ctx->aipchdl);
	if (pAmlMp3Ctx->hShmIn) {
		Aml_ACodecMemory_Free(pAmlMp3Ctx->hShmIn);
		pAmlMp3Ctx->hShmIn = 0;
	}
	if (pAmlMp3Ctx->hShmOut) {
		Aml_ACodecMemory_Free(pAmlMp3Ctx->hShmOut);
		pAmlMp3Ctx->hShmOut = 0;
	}
	free(pAmlMp3Ctx);
}

ERROR_CODE AmlACodecExec_Mp3Dec(tAmlMp3DecHdl hMp3Dec, tAmlACodecConfig_Mp3DecExternal *pconfig)
{
	mp3_decode_st arg;
	struct tAmlMp3Ctx* pAmlMp3Ctx = (struct tAmlMp3Ctx*)hMp3Dec;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)pAmlMp3Ctx->mp3rpchdl;
	mp3_sync_local_config_to_rpctype(pconfig, &arg.config);
	if (pAmlMp3Ctx->hShmIn) {
		void* p = Aml_ACodecMemory_GetVirtAddr(pAmlMp3Ctx->hShmIn);
		if (pconfig->inputBufferCurrentLength < kInputBufferSize)
			memcpy(p, pconfig->pInputBuffer, pconfig->inputBufferCurrentLength);
		else {
			printf("too large input frame:%d\n", pconfig->inputBufferCurrentLength);
			memcpy(p, pconfig->pInputBuffer, kInputBufferSize);
		}
		Aml_ACodecMemory_Clean(pAmlMp3Ctx->hShmIn, kInputBufferSize);
	}
	/*int i;
	char* samples = (char*)&arg;
	for (i = 0; i < sizeof(mp3_decode_st); i++) {
		printf("0x%x ", samples[i]);
	}
	printf("\n");*/

	//printf("arg.hdl=0x%lx\n",arg.hdl);
	xAIPC(pAmlMp3Ctx->aipchdl, MBX_CODEC_MP3_API_DECODE, &arg, sizeof(arg));
	mp3_sync_rpctype_config_to_local(&arg.config, pconfig);
	if (pAmlMp3Ctx->hShmOut) {
		Aml_ACodecMemory_Inv(pAmlMp3Ctx->hShmOut, kOutputBufferSize);
		void* p = Aml_ACodecMemory_GetVirtAddr(pAmlMp3Ctx->hShmOut);
		if (pconfig->outputFrameSize*sizeof(int16_t) < kOutputBufferSize)
			memcpy(pconfig->pOutputBuffer, p, pconfig->outputFrameSize*sizeof(int16_t));
		else {
			memcpy(pconfig->pOutputBuffer, p, kOutputBufferSize);
		}
	}

	return arg.ret;
}

