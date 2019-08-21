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
 * offload aac decoder
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_AAC_H_
#define _RPC_CLIENT_AAC_H_


#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "rpc_client_shm.h"
#include "aacdecoder_lib.h"

#define AAC_INPUT_SIZE (1024*16)

#define AAC_MAX_CH_NUM 8
#define MAX_FRAME_SIZE 2048
#define PCM_OUTPUT_SIZE (AAC_MAX_CH_NUM*MAX_FRAME_SIZE*(SAMPLE_BITS>>3))



typedef void* tAmlAacDecHdl;

typedef struct  {
    uint32_t transportFmt;
	uint32_t nrOfLayers;
} tAmlAacInitCtx;

typedef struct  {
	uint32_t sampleRate;
	uint32_t bitDepth;
	uint32_t frameSize;
	uint32_t channelNum;
	int chmask[0x24];
} tAmlAacOutputCtx;

tAmlAacDecHdl AmlACodecInit_AacDec(tAmlAacInitCtx *ctx);
void AmlACodecDeInit_AacDec(tAmlAacDecHdl hdl);
AAC_DECODER_ERROR AmlACodecExec_AacDec(tAmlAacDecHdl hAacDec, void* aac_input, uint32_t aac_input_size,
										void* pcm_out, uint32_t* pcm_out_size, uint32_t* aac_input_left, tAmlAacOutputCtx* ctx);

AAC_DECODER_ERROR AmlACodecExec_UserAllocIoShm_AacDec(tAmlAacDecHdl hAacDec, void* aac_input, uint32_t aac_input_size,
									void* pcm_out, uint32_t* pcm_out_size, uint32_t* aac_input_left, tAmlAacOutputCtx* ctx);

AAC_DECODER_ERROR AmlACodecSetParam(tAmlAacDecHdl hAacDec, int32_t param, int32_t value);


AAC_DECODER_ERROR AmlACodecAncInit(tAmlAacDecHdl hAacDec, void* buf, uint32_t size);

AAC_DECODER_ERROR AmlACodecConfigRaw(tAmlAacDecHdl hAacDec, char** conf, uint32_t* length);


#ifdef __cplusplus
}
#endif

#endif // end _OFFLOAD_ACODEC_MP3_H_
