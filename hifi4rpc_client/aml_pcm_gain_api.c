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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aml_pcm_gain_api.h"
#include "rpc_client_vsp.h"
#include "rpc_client_shm.h"
#ifdef MOCK_GAIN
#include "aml_pcm_gain_table.h"

/* ...Context structure */
typedef struct _GainContext {
    int32_t isOpen;
    int32_t channels;
    int32_t pcm_width;
    int32_t sample_rate;
    int32_t gain_idx;
} GainContext;

static GainContext* aml_pcm_gain_init(void)
{
    GainContext* d = malloc(sizeof(GainContext));
    memset(d, 0, sizeof(GainContext));

    /* ...set default parameters */
    d->channels = 1;
    d->pcm_width = 16;
    d->sample_rate = 48000;
    d->gain_idx = pcm_gains_16[DB_IDX_SHIT];
    return d;
}

static void aml_pcm_gain_deinit(GainContext* d)
{
    free(d);
}

static PCMGAIN_RET aml_pcm_gain_open(GainContext* d)
{
    d->isOpen = 1;
    return PCMGAIN_RET_OK;
}

static PCMGAIN_RET aml_pcm_gain_close(GainContext* d)
{
    d->isOpen = 0;
    return PCMGAIN_RET_OK;
}

#define PCM_GAIN_CHECK_PARAM_RET(string, value, num, tabel, ret) do{ \
    uint32_t i;                                      \
    for (i = 0; i < num; i++) { \
        if (value == tabel[i]) \
            break; \
    } \
    if (i == num) { \
        printf("%s:%d\n", string, value); \
        return ret; \
    }   \
} while(0);

static PCMGAIN_RET aml_pcm_gain_set_config_param(GainContext *d, PCMGAIN_PARA_ID paraId, PCMGAIN_PARA *para)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    if (d->isOpen &&
        (paraId == PCMGAIN_PARA_SAMPLE_BITS || paraId == PCMGAIN_PARA_SAMPLE_RATE || paraId == PCMGAIN_PARA_CH_NUM)) {
        printf("Do not support set parameter paraId:%d after open\n", paraId);
        return PCMGAIN_RET_ERR_NOT_SUPPORT;
    }
    switch (paraId)
    {
        case PCMGAIN_PARA_SAMPLE_BITS:
            PCM_GAIN_CHECK_PARAM_RET("Do not support this bit width", para->SampBits,
                                sizeof(pcm_bitdepth)/sizeof(int32_t), pcm_bitdepth,
                                PCMGAIN_RET_ERR_NOT_SUPPORT);
            d->pcm_width = para->SampBits;
            break;
        case PCMGAIN_PARA_CH_NUM:
            d->channels = para->ChNum;
            break;
        case PCMGAIN_PARA_SAMPLE_RATE:
            PCM_GAIN_CHECK_PARAM_RET("Do not support this sample rate", para->SampRate,
                                sizeof(pcm_rate)/sizeof(int32_t), pcm_rate,
                                PCMGAIN_RET_ERR_NOT_SUPPORT);
            d->sample_rate = para->SampRate;
            break;
        case PCMGAIN_PARA_GAIN:
            if (para->Gain < -DB_IDX_SHIT) {
                d->gain_idx = 0;
                printf("The lowest gain level is %d\n", (int)-DB_IDX_SHIT);
            }
            else if (para->Gain > DB_IDX_SHIT) {
                d->gain_idx = 2*DB_IDX_SHIT;
                printf("The highest gain level is %d\n", (int)DB_IDX_SHIT);
            }
            else
                d->gain_idx = para->Gain + DB_IDX_SHIT;
            break;
        default:
            printf("Set param invalid para id:%d\n", paraId);
            ret = PCMGAIN_RET_ERR_NOT_SUPPORT;
            break;
    }
    return ret;
}

static PCMGAIN_RET aml_pcm_gain_get_config_param(GainContext *d, PCMGAIN_PARA_ID paraId, PCMGAIN_PARA *para)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    switch (paraId)
    {
        case PCMGAIN_PARA_SAMPLE_RATE:
            para->SampRate = d->sample_rate;
            break;
        case PCMGAIN_PARA_SAMPLE_BITS:
            para->SampBits = d->pcm_width;
            break;
        case PCMGAIN_PARA_CH_NUM:
            para->ChNum = d->channels;
            break;
        case PCMGAIN_PARA_GAIN:
            para->Gain = d->gain_idx - DB_IDX_SHIT;
            break;
        case PCMGAIN_PARA_SUPPORT_SAMPLE_RATES:
            para->numOfSupportSampRates = sizeof(pcm_rate)/sizeof(int32_t);
            memcpy(&para->supportSampRates, pcm_rate, sizeof(pcm_rate));
            break;
        case PCMGAIN_PARA_SUPPORT_SAMPLE_BITS:
            para->numOfSupportSampBits = sizeof(pcm_bitdepth)/sizeof(int32_t);
            memcpy(&para->supportSampRates, pcm_bitdepth, sizeof(pcm_bitdepth));
            break;
        default:
            printf("Get param invalid para id:%d\n", paraId);
            ret = PCMGAIN_RET_ERR_NOT_SUPPORT;
            break;
    }
    return ret;
}

/* ...apply gain to 16-bit PCM stream */
static PCMGAIN_RET aml_pcm_gain_do_execute_16bit(GainContext *d, int16_t* pIn, int16_t* pOut, int32_t nSample)
{
    int32_t     i;
    int16_t     input;
    int16_t     gain = pcm_gains_16[d->gain_idx];
    int32_t     product;

    /* ...Processing loop */
    for (i = 0; i < (nSample*d->channels); i++)
    {
        input = *pIn++;
        product = (int32_t)input*gain;
        product = product >> 12;

        if(product > MAX_16BIT)
            product = MAX_16BIT;
        else if(product < MIN_16BIT)
            product = MIN_16BIT;

        *pOut++ = (int16_t)product;
    }
    return PCMGAIN_RET_OK;
}

/* ...apply gain to 32-bit PCM stream */
static PCMGAIN_RET aml_pcm_gain_do_execute_32bit(GainContext *d, int32_t* pIn, int32_t* pOut, int32_t nSample)
{
    int32_t     i;
    int32_t     input;
    int32_t     gain = pcm_gains_32[d->gain_idx];
    int64_t     product;

    /* ...Processing loop */
    for (i = 0; i < (nSample*d->channels); i++)
    {
        input = *pIn++;
        product = (int64_t)input*gain;
        product = product >> 24;
        if((int32_t)product > (int32_t)MAX_32BIT)
            product = MAX_32BIT;
        else if((int32_t)product < (int32_t)MIN_32BIT)
            product = MIN_32BIT;

        *pOut++ = (int32_t)product;
    }
    return PCMGAIN_RET_OK;
}

#endif

#define PCMGAIN_MS_CHUNK 20
#define PCMGAIN_CHECK_NULL(ptr)    if ((ptr) == 0)          \
    {printf("%s:%d:NULL POINT\n", __FUNCTION__, __LINE__);return PCMGAIN_RET_ERR_NULL_POINTER;}
#define PCMGAIN_MIN(a, b) ((a) < (b) ? (a) : (b))


struct _PCMGAIN {
#ifdef MOCK_GAIN
    GainContext* pGainInstance;
#else
    AML_VSP_HANDLE hVsp;
    AML_MEM_HANDLE hParam;
    size_t param_size;
    AML_MEM_HANDLE hInput;
    size_t input_size;
    AML_MEM_HANDLE hOutput;
    int32_t nSampleRate;
    int32_t nSampleBit;
    int32_t nChannels;
    int32_t nGains;
#endif
};

PCMGAIN_RET AML_PCMGAIN_Create(PCMGAIN **pcmgain)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    PCMGAIN_CHECK_NULL(pcmgain);
    PCMGAIN* pGain = malloc(sizeof(PCMGAIN));
    memset(pGain, 0, sizeof(PCMGAIN));
#ifdef MOCK_GAIN
    pGain->pGainInstance = aml_pcm_gain_init();
     *pcmgain = pGain;
#else
    pGain->hParam = AML_MEM_Allocate(sizeof(PCMGAIN_PARA));
    if (!pGain->hParam) {
        printf("Failed to allocate parameter shared memory\n");
        ret = PCMGAIN_RET_ERR_NO_MEM;
        goto end_of_create;
    }
    pGain->hVsp = AML_VSP_Init("AML.VSP.PCMGAIN", NULL, 0);
    if (!pGain->hVsp) {
        printf("Failed to AML.VSP.PCMGAIN\n");
        AML_MEM_Free(pGain->hParam);
        ret = PCMGAIN_RET_ERR_NOT_SUPPORT;
        goto end_of_create;
    }
    pGain->param_size = sizeof(PCMGAIN_PARA);
    *pcmgain = pGain;
end_of_create:
#endif
    return ret;
}

PCMGAIN_RET AML_PCMGAIN_Destroy(PCMGAIN *pcmgain)
{
    PCMGAIN_CHECK_NULL(pcmgain);
#ifdef MOCK_GAIN
    aml_pcm_gain_deinit(pcmgain->pGainInstance);
#else
    AML_MEM_Free(pcmgain->hParam);
    AML_VSP_Deinit(pcmgain->hVsp);
#endif
    free(pcmgain);
    return PCMGAIN_RET_OK;
}

PCMGAIN_RET AML_PCMGAIN_Open(PCMGAIN *pcmgain)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    PCMGAIN_CHECK_NULL(pcmgain);
#ifdef MOCK_GAIN
    return aml_pcm_gain_open(pcmgain->pGainInstance);
#else
    pcmgain->input_size = PCMGAIN_MS_CHUNK*pcmgain->nSampleRate*pcmgain->nChannels*(pcmgain->nSampleBit>>3)/1000;
    pcmgain->hInput = AML_MEM_Allocate(pcmgain->input_size);
    if (!pcmgain->hInput) {
        printf("Failed to allocate input shared memory\n");
        ret = PCMGAIN_RET_ERR_NO_MEM;
        goto end_of_open;
    }
    pcmgain->hOutput = AML_MEM_Allocate(pcmgain->input_size);
    if (!pcmgain->hOutput) {
        printf("Failed to allocate output shared memory\n");
        AML_MEM_Free(pcmgain->hOutput);
        ret = PCMGAIN_RET_ERR_NO_MEM;
        goto end_of_open;
    }
    ret = AML_VSP_Open(pcmgain->hVsp);
    if (ret != PCMGAIN_RET_OK) {
        printf("Failed to open vsp instane\n");
        ret = PCMGAIN_RET_ERR_NOT_SUPPORT;
        goto end_of_open;
    }
end_of_open:
#endif
    return ret;
}

PCMGAIN_RET AML_PCMGAIN_Close(PCMGAIN *pcmgain)
{
    PCMGAIN_CHECK_NULL(pcmgain);
#ifdef MOCK_GAIN
    return aml_pcm_gain_close(pcmgain->pGainInstance);
#else
    AML_VSP_Close(pcmgain->hVsp);
    AML_MEM_Free(pcmgain->hInput);
    pcmgain->hInput = 0;
    AML_MEM_Free(pcmgain->hOutput);
    pcmgain->hOutput = 0;
#endif
    return PCMGAIN_RET_OK;
}

PCMGAIN_RET AML_PCMGAIN_SetParam(PCMGAIN *pcmgain, PCMGAIN_PARA_ID paraId, PCMGAIN_PARA *para)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    PCMGAIN_CHECK_NULL(pcmgain);
    PCMGAIN_CHECK_NULL(para);
#ifdef MOCK_GAIN
    return aml_pcm_gain_set_config_param(pcmgain->pGainInstance, paraId, para);
#else
    char* pParam = (char*)AML_MEM_GetVirtAddr(pcmgain->hParam);
    memcpy(pParam, para, sizeof(PCMGAIN_PARA));
    AML_MEM_Clean(AML_MEM_GetPhyAddr(pcmgain->hParam), pcmgain->param_size);
    ret = AML_VSP_SetParam(pcmgain->hVsp, (int32_t)paraId,
                        AML_MEM_GetPhyAddr(pcmgain->hParam), pcmgain->param_size);
    /*Update local context if some parameter is succesfully configured*/
    if (ret == PCMGAIN_RET_OK) {
        switch (paraId) {
            case PCMGAIN_PARA_CH_NUM:
                pcmgain->nChannels = para->ChNum;
                break;
            case PCMGAIN_PARA_SAMPLE_RATE:
                pcmgain->nSampleRate = para->SampRate;
                break;
            case PCMGAIN_PARA_SAMPLE_BITS:
                pcmgain->nSampleBit = para->SampBits;
                break;
            default:
                break;
        }
    }
#endif
    return ret;
}

PCMGAIN_RET AML_PCMGAIN_GetParam(PCMGAIN *pcmgain, PCMGAIN_PARA_ID paraId, PCMGAIN_PARA *para)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    PCMGAIN_CHECK_NULL(pcmgain);
    PCMGAIN_CHECK_NULL(para);
#ifdef MOCK_GAIN
    return aml_pcm_gain_get_config_param(pcmgain->pGainInstance, paraId, para);
#else
    ret = AML_VSP_GetParam(pcmgain->hVsp, (int32_t)paraId,
                AML_MEM_GetPhyAddr(pcmgain->hParam), pcmgain->param_size);
    if (ret == PCMGAIN_RET_OK) {
        AML_MEM_Invalidate(AML_MEM_GetPhyAddr(pcmgain->hParam), pcmgain->param_size);
        char* pParam = (char*)AML_MEM_GetVirtAddr(pcmgain->hParam);
        memcpy(para, pParam, sizeof(PCMGAIN_PARA));
    }
#endif
    return ret;
}


PCMGAIN_RET AML_PCMGAIN_Process(PCMGAIN *pcmgain, void* in, void* out, int32_t nSamples)
{
    PCMGAIN_RET ret = PCMGAIN_RET_OK;
    PCMGAIN_CHECK_NULL(pcmgain);
    PCMGAIN_CHECK_NULL(in);
    PCMGAIN_CHECK_NULL(out);
#ifdef MOCK_GAIN
    if (pcmgain->pGainInstance->pcm_width == 16)
        return aml_pcm_gain_do_execute_16bit(pcmgain->pGainInstance, in, out, nSamples);
    else
        return aml_pcm_gain_do_execute_32bit(pcmgain->pGainInstance, in, out, nSamples);
#else
    uint8_t* pIn = in;
    uint8_t* pOut = out;
    int32_t totalSize = nSamples*pcmgain->nChannels*(pcmgain->nSampleBit>>3);
    while (totalSize) {
        size_t procLen = PCMGAIN_MIN(totalSize, pcmgain->input_size);
        size_t outputSize = procLen;
        memcpy(AML_MEM_GetVirtAddr(pcmgain->hInput), pIn, procLen);
        AML_MEM_Clean(pcmgain->hInput, procLen);
        ret = AML_VSP_Process(pcmgain->hVsp, AML_MEM_GetPhyAddr(pcmgain->hInput), procLen,
                            AML_MEM_GetPhyAddr(pcmgain->hOutput), &outputSize);
        if (ret != PCMGAIN_RET_OK) {
            printf("Failed to gain samples\n");
            break;
        }
        AML_MEM_Invalidate(pcmgain->hOutput, procLen);
        memcpy(pOut, AML_MEM_GetVirtAddr(pcmgain->hOutput), procLen);
        totalSize -= procLen;
        pIn += procLen;
        pOut += procLen;
    }
#endif
    return ret;
}

