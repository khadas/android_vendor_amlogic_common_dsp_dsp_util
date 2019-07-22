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
#include "rpc_client_mp3.h"
#include "aipc_type.h"

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

tAmlMp3DecHdl AmlACodecInit_Mp3Dec(tAmlACodecConfig_Mp3DecExternal* pconfig)
{
	mp3_init_st arg;
	memset(&arg, 0, sizeof(arg));

	mp3_sync_local_config_to_rpctype(pconfig, &arg.config);
	//printf("size of config in arm:%u\n", sizeof(tAmlACodecConfig_Mp3DecRpc));
	xAIPC(MBX_CODEC_MP3_API_INIT, &arg, sizeof(arg));
	mp3_sync_rpctype_config_to_local(&arg.config, pconfig);
	return (void*)arg.hdl;
}

void AmlACodecDeInit_Mp3Dec(tAmlMp3DecHdl hMp3Dec)
{
	mp3_deinit_st arg;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)hMp3Dec;
	xAIPC(MBX_CODEC_MP3_API_DEINIT, &arg, sizeof(arg));
}

ERROR_CODE AmlACodecExec_Mp3Dec(tAmlMp3DecHdl hMp3Dec, tAmlACodecConfig_Mp3DecExternal *pconfig)
{
	mp3_decode_st arg;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlMp3DecRpcHdl)hMp3Dec;
	mp3_sync_local_config_to_rpctype(pconfig, &arg.config);

	/*int i;
	char* samples = (char*)&arg;
	for (i = 0; i < sizeof(mp3_decode_st); i++) {
		printf("0x%x ", samples[i]);
	}
	printf("\n");*/

    //vCacheCleanDcacheRange((uint64_t)arg.config.pInputBuffer, arg.config.inputBufferCurrentLength);
	//printf("arg.hdl=0x%lx\n",arg.hdl);
	xAIPC(MBX_CODEC_MP3_API_DECODE, &arg, sizeof(arg));
	mp3_sync_rpctype_config_to_local(&arg.config, pconfig);
	//vCacheInvDcacheRange((uint64_t)arg.config.pOutputBuffer, arg.config.outputFrameSize*sizeof(uint16_t));
	return arg.ret;
}

