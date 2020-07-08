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


/*aac decoder handler*/
typedef void* tAmlAacDecHdl;

typedef struct  {
    /*see enum TRANSPORT_TYPE defined in FDK_audio.h*/
    uint32_t transportFmt;
    /*Number of transport layers.*/
    uint32_t nrOfLayers;
} tAmlAacInitCtx;

typedef struct  {
    /*decoded pcm sample rate*/
    uint32_t sampleRate;
    /*decoded pcm bit depth*/
    uint32_t bitDepth;
    /*number of samples per channel*/
    uint32_t frameSize;
    /*number of channels*/
    uint32_t channelNum;
    /*see enum AUDIO_CHANNEL_TYPE, defined in aacdecoder_lib.h*/
    uint8_t chmask[0x24];
} tAmlAacOutputCtx;

/**
 * Create and initialize a aac decoder
 *
 * @param[in] See define of tAmlAacInitCtx
 *
 * @return aac decoder handler if success, otherwise return NULL
 */
tAmlAacDecHdl AmlACodecInit_AacDec(tAmlAacInitCtx *ctx);

/**
 * Destory aac decoder
 *
 * @param[in] Aac decdoer handler
 *
 * @return
 */
void AmlACodecDeInit_AacDec(tAmlAacDecHdl hdl);

/**
 * Decode a aac frame
 *
 * @param[in] aac decoder handler
 *
 * @param[in] input buffer
 *
 * @param[in] input buffer size
 *
 * @param[in] output buffer address
 *
 * @param[in/out] output buffer size
 *
 * @param[out] remained size of bytes in input buffer
 *
 * @param[out] See define of tAmlAacOutputCtx
 *
 * @return NO_DECODING_ERROR if success, otherwise see ERROR_CODE defined in aacdecoder_lib.h
 */
AAC_DECODER_ERROR AmlACodecExec_AacDec(tAmlAacDecHdl hAacDec, void* aac_input, uint32_t aac_input_size,
										void* pcm_out, uint32_t* pcm_out_size, uint32_t* aac_input_left, tAmlAacOutputCtx* ctx);

/**
 * Decode a aac frame
 * This API execute similar function as AmlACodecExec_AacDec.
 * User need allocate in/out shared memory, user need clean
 * input shared memory and invalidate output shared memory.
 *
 * @param[in] aac decoder handler
 *
 * @param[in] input shared memory physical address
 *
 * @param[in] input shared memory size
 *
 * @param[in] output shared memory physical address
 *
 * @param[in/out] output shared memory size
 *
 * @param[out] remained size of bytes in input buffer
 *
 * @param[out] See define of tAmlAacOutputCtx
 *
 * @return NO_DECODING_ERROR if success, otherwise see ERROR_CODE defined in aacdecoder_lib.h
 */
AAC_DECODER_ERROR AmlACodecExec_UserAllocIoShm_AacDec(tAmlAacDecHdl hAacDec, void* aac_input, uint32_t aac_input_size,
									void* pcm_out, uint32_t* pcm_out_size, uint32_t* aac_input_left, tAmlAacOutputCtx* ctx);


/**
 * Set one single decoder parameter.
 *
 * @param[in] AAC decoder handle.
 *
 * @param[in] Parameter to be set, see enum AACDEC_PARAM defined in aacdecoder_lib.h
 *
 * @param[in] Value of the parameter
 *
 * @return NO_DECODING_ERROR if success, otherwise see ERROR_CODE defined in aacdecoder_lib.h
 */
AAC_DECODER_ERROR AmlACodecSetParam(tAmlAacDecHdl hAacDec, int32_t param, int32_t value);


/**
 * Initialize ancillary data buffer.
 * This API is usually used in downmix case, this case
 * need an ancillary downmix buffer.
 *
 * @param[in] AAC decoder handle.
 *
 * @param[in] Pointer to (external) ancillary data buffer. Physical address of a shared memory.
 *
 * @param[in] Size of the buffer pointed to by buffer.
 *
 * @return NO_DECODING_ERROR if success, otherwise see ERROR_CODE defined in aacdecoder_lib.h
 */
AAC_DECODER_ERROR AmlACodecAncInit(tAmlAacDecHdl hAacDec, void* buf, uint32_t size);


/**
 * Explicitly configure the decoder by passing a raw AudioSpecificConfig
 * (ASC) or a StreamMuxConfig (SMC), contained in a binary buffer. This is
 * required for MPEG-4 and Raw Packets file format bitstreams as well as for
 * LATM bitstreams with no in-band SMC. If the transport format is LATM with or
 * without LOAS, configuration is assumed to be an SMC, for all other file
 * formats an ASC.
 *
 * @param[in] AAC decoder handle.
 *
 * @param[in] Pointer to an shared memory buffer containing the binary
 * configuration buffer (either ASC or SMC). User need clean the shared
 * memory before call this API
 *
 * @param[in] Length of the configuration buffer in bytes.
 *
 * @return NO_DECODING_ERROR if success, otherwise see ERROR_CODE defined in aacdecoder_lib.h
 */
AAC_DECODER_ERROR AmlACodecConfigRaw(tAmlAacDecHdl hAacDec, char** conf, uint32_t* length);


#ifdef __cplusplus
}
#endif

#endif // end _OFFLOAD_ACODEC_MP3_H_
