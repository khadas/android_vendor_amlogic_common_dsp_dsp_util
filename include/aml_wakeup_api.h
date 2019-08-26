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
 * amlogic wake up engine API
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AMLOGIC_WAKE_API_H_
#define _AMLOGIC_WAKE_API_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "rpc_client_shm.h"
#define AWE_MAX_IN_CHANS 4
#define AWE_MAX_OUT_CHANS 2

typedef struct _AWE AWE;

typedef union {
    int32_t inSampRate;
    int32_t inSampBits;
    struct {
        int32_t supportSampRates[4];
        int32_t numOfSupportSampRates;
    };
    struct {
        int32_t supportSampBits[4];
        int32_t numOfSupportSampBits;
    };
    int32_t micInChannels;
    int32_t refInChannels;
    int32_t outChannels;
} __attribute__((packed)) AWE_PARA;

/*
 */
typedef enum {
    /* static params */
    /* static params can only be set before AWE_Open */
    AWE_PARA_IN_SAMPLE_RATE,
    AWE_PARA_IN_SAMPLE_BITS,
    AWE_PARA_MIC_IN_CHANNELS,
    AWE_PARA_REF_IN_CHANNELS,
    AWE_PARA_OUT_CHANNELS,
    /* dynamic params */
    /* getting only params */
    AWE_PARA_WAKE_UP_SCORE,
    AWE_PARA_SUPPORT_SAMPLE_RATES,
    AWE_PARA_SUPPORT_SAMPLE_BITS,
} AWE_PARA_ID;

typedef enum {
    AWE_RET_OK = 0,
    AWE_RET_ERR_NO_MEM = -1,
    AWE_RET_ERR_NOT_SUPPORT = -2,
    AWE_RET_ERR_NULL_POINTER = -3,
} AWE_RET;

AWE_RET AML_AWE_Create(AWE **awe);
AWE_RET AML_AWE_Destroy(AWE *awe);
AWE_RET AML_AWE_Open(AWE *awe);
AWE_RET AML_AWE_Close(AWE *awe);
AWE_RET AML_AWE_Process(AWE *awe, AML_MEM_HANDLE in[], int32_t *inLenInByte, AML_MEM_HANDLE out[],
        int32_t *outLenInByte, uint32_t *isWaked);
AWE_RET AML_AWE_SetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para);
AWE_RET AML_AWE_GetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para);

#ifdef __cplusplus
}
#endif

#endif /* _AMLOGIC_WAKE_ENGINE_H_ */
