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
 * pcm gain api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#ifndef _AMLOGIC_PCM_GAIN_H_
#define _AMLOGIC_PCM_GAIN_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _PCMGAIN PCMGAIN;

typedef union {
    /*The unit of Gain is dB, from -10 to 10, 1 dB step*/
    int32_t Gain;
    int32_t ChNum;
    int32_t SampRate;
    int32_t SampBits;
    struct {
        int32_t numOfSupportSampRates;
        int32_t supportSampRates[8];
    };
    struct {
        int32_t numOfSupportSampBits;
        int32_t supportSampBits[8];
    };
} __attribute__((packed)) PCMGAIN_PARA;

/*
 */
typedef enum {
    /* static params */
    /* static params can only be set before AML_PCMGAIN_Open */
    PCMGAIN_PARA_CH_NUM,
    PCMGAIN_PARA_SAMPLE_RATE,
    PCMGAIN_PARA_SAMPLE_BITS,
    PCMGAIN_PARA_SUPPORT_SAMPLE_RATES,
    PCMGAIN_PARA_SUPPORT_SAMPLE_BITS,
    /* dynamic params */
    PCMGAIN_PARA_GAIN
} PCMGAIN_PARA_ID;

typedef enum {
    PCMGAIN_RET_OK = 0,
    PCMGAIN_RET_ERR_NO_MEM = -1,
    PCMGAIN_RET_ERR_NOT_SUPPORT = -2,
    PCMGAIN_RET_ERR_NULL_POINTER = -3,
} PCMGAIN_RET;

/**
 * Create and initialize PCMGAIN instance
 *
 * @param[out] PCMGAIN instance handle
 *
 * @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
 */
PCMGAIN_RET AML_PCMGAIN_Create(PCMGAIN **pcmgain);

/**
 * Destory PCMGAIN instance and all resources allocated for PCMGAIN instance.
 *
 * @param[in] PCMGAIN instance handle
 *
 * @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
 */
PCMGAIN_RET AML_PCMGAIN_Destroy(PCMGAIN *pcmgain);

/**
 * Enable PCMGAIN instance
 *
 * All static params should be configured through AML_PCMGAIN_SetParam
 * befor this call. PCMGAIN instance is moved to working status after this call
 *
 * @param[in] PCMGAIN instance handle
 *
 * @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
 */
PCMGAIN_RET AML_PCMGAIN_Open(PCMGAIN *pcmgain);

/**
 * Disable PCMGAIN instance
 *
 * The API AML_PCMGAIN_Process can not working after this call.
 * User call configure static parameter again after this call.
 *
 * @param[in] PCMGAIN instance handle
 *
 * @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
 */
PCMGAIN_RET AML_PCMGAIN_Close(PCMGAIN *pcmgain);

/**
 * Configure PCMGAIN parameter
 *
 * Static params is configured before AML_PCMGAIN_Open.
 * Dynamic params can be configured after AML_PCMGAIN_Open.
 *
 * @param[in] PCMGAIN instance handle
 *
 * @param[in] identification of a parameter see PCMGAIN_PARA_ID
 *
 * @param[in] value of a parameter, associated with paraId,
 * see PCMGAIN_PARA
 *
 * @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
 */
PCMGAIN_RET AML_PCMGAIN_SetParam(PCMGAIN *pcmgain, PCMGAIN_PARA_ID paraId, PCMGAIN_PARA *para);

/**
 * Obtain PCMGAIN parameter
 *
 * @param[in] PCMGAIN instance handler
 *
 * @param[in] identification of a parameter see PCMGAIN_PARA_ID
 *
 * @param[out] value of a parameter, associated with paraId,
 * see PCMGAIN_PARA
 *
 * @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
 */
PCMGAIN_RET AML_PCMGAIN_GetParam(PCMGAIN *pcmgain, PCMGAIN_PARA_ID paraId, PCMGAIN_PARA *para);

/**
* PCMGAIN data processing main routine.
* Application feed input here, application obtain output here
*
* @param[in] PCMGAIN instance handle
*
* @param[in] PCM input buffer
*
* @param[out] PCM output buffer, the pcm after gain is placed here
*
* @param[in] number of samples
*
* @return PCMGAIN_RET_OK if successful, otherwise see PCMGAIN_RET
*/
PCMGAIN_RET AML_PCMGAIN_Process(PCMGAIN *pcmgain, void* in, void* out, int32_t nSamples);


#ifdef __cplusplus
}
#endif

#endif /* _AMLOGIC_PCM_GAIN_H_ */

