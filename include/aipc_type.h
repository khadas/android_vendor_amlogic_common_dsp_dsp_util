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
 * data type for audio rpc
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AIPC_TYPE_H_
#define _AIPC_TYPE_H_

#include <stdint.h>
#include "ipc_cmd_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef xpointer tAmlMp3DecRpcHdl;
typedef struct {
	int32_t	  inputBufferCurrentLength;
	int32_t	  inputBufferUsedLength;
	uint32_t	   CurrentFrameLength;
	uint32_t	   equalizerType;
	int32_t	  inputBufferMaxLength;
	int32_t		num_channels;
	int32_t		version;
	int32_t		samplingRate;
	int32_t		bitRate;
	int32_t	  outputFrameSize;
	int32_t	  crcEnabled;
	uint32_t	   totalNumberOfBitsUsed;
	xpointer	pOutputBuffer;
	xpointer	pInputBuffer;
} __attribute__((packed)) tAmlACodecConfig_Mp3DecRpc;

typedef struct {
    tAmlACodecConfig_Mp3DecRpc config;
    tAmlMp3DecRpcHdl hdl;
} __attribute__((packed)) mp3_init_st;

typedef struct {
    tAmlACodecConfig_Mp3DecRpc config;
    tAmlMp3DecRpcHdl hdl;
	uint32_t ret;
} __attribute__((packed)) mp3_decode_st;

typedef struct {
    tAmlMp3DecRpcHdl hdl;
} __attribute__((packed)) mp3_deinit_st;


typedef xpointer tAcodecShmHdlRpc;
typedef struct {
    tAcodecShmHdlRpc hShm;
    size_t size;
} __attribute__((packed)) acodec_shm_alloc_st;

typedef struct {
    tAcodecShmHdlRpc hShm;
} __attribute__((packed)) acodec_shm_free_st;

typedef struct {
    tAcodecShmHdlRpc hDst;
    tAcodecShmHdlRpc hSrc;
    size_t size;
} __attribute__((packed)) acodec_shm_transfer_st;


#ifdef __cplusplus
}
#endif

#endif // end _OFFLOAD_ACODEC_MP3_H_
