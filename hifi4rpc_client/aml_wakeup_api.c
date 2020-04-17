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
 * amlogic wake up engine api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include "rpc_client_aipc.h"
#include "rpc_client_vsp.h"
#include "rpc_client_shm.h"
#include "aml_wakeup_api.h"
#include "generic_macro.h"

#define VOICE_CHUNK_MS 20
#define VOICE_CHUNK_NUM 50

#define AWE_CHECK_NULL(ptr)    if ((ptr) == 0)          \
    {printf("%s:%d:NULL POINT\n", __FUNCTION__, __LINE__);return AWE_RET_ERR_NULL_POINTER;}
#define AWE_CHECK_STATUS(st0,st1)    if ((st0) != (st1))          \
    {printf("%s:%d:Invalid status:%d\n", __FUNCTION__, __LINE__, (st0));return AWE_RET_ERR_NOT_SUPPORT;}

typedef enum {
    /* Set to IDLE in Create*/
    AWE_STATUS_IDLE,
    /* Set to EXECUTE in Open*/
    AWE_STATUS_EXECUTE,
    /* Set to FREE_RUN in low power*/
    AWE_STATUS_FREE_RUN,
} AWE_STATUS;


struct _AWE {
    AML_VSP_HANDLE hVsp;
    AML_MEM_HANDLE hParam;
    size_t param_size;
    AML_MEM_HANDLE hInput;
    size_t input_size;
    AML_MEM_HANDLE hOutput;
    size_t output_size;
    int32_t inSampleRate;
    int32_t inSampleBit;
    int32_t micInChannels;
    int32_t refInChannels;
    int32_t outChannels;
    int32_t inputMode;
    AML_AWE_DataHandler awe_data_handler_func[AWE_DATA_TYPE_MAX];
    void* awe_data_handler_userdata[AWE_DATA_TYPE_MAX];
    AML_AWE_EventHandler awe_event_handler_func[AWE_EVENT_TYPE_MAX];
    void* awe_event_handler_userdata[AWE_EVENT_TYPE_MAX];
    int32_t work_thread_exit;
    AML_MEM_HANDLE hinWorkBuf[AWE_MAX_IN_CHANS];
    int32_t inWorkBufLen;
    int32_t inWorkBufLenInSample;
    AML_MEM_HANDLE houtWorkBuf[AWE_MAX_OUT_CHANS];
    int32_t outWorkBufLen;
    char* userFillBuf;
    uint32_t userFillBufRd;
    uint32_t userFillBufWr;
    uint32_t userFillBufSize;
    sem_t userFillSem;
    pthread_t work_thread;
    uint32_t aweStatus;
    uint32_t bKickFreeRun;
};

typedef struct {
    int32_t    numInChn;
    int32_t    lenInByte;
    uint64_t   pChanIn[AWE_MAX_IN_CHANS];
    int32_t    numOutChn;
    int32_t    lenOutByte;
    uint64_t   pChanOut[AWE_MAX_OUT_CHANS];
} __attribute__((packed)) aml_vsp_awe_process_param_in;

typedef struct {
    int32_t    lenInByte;
    int32_t    lenOutByte;
    uint32_t   isWaked;
} __attribute__((packed)) aml_vsp_awe_process_param_out;

/*for PCM_OUT*/
static uint32_t aml_awe_ring_buf_fullness(uint32_t size,
        uint32_t wr, uint32_t rd)
{
    return (wr >= rd) ? (wr - rd) : (size + wr - rd);
}

/*for PCM_OUT*/
static uint32_t aml_awe_ring_buf_space(uint32_t size,
        uint32_t wr, uint32_t rd)
{
    return (wr > rd) ? (size + rd - wr - 1) :
        (rd - wr - 1);
}

static void aml_ch_extract_s16le(int16_t *dst, int16_t *src, uint32_t nSample,
                          uint32_t chn, uint32_t chidx)
{
    uint32_t i;
    if (chidx > chn || !dst || !src) {
        printf("Invalid param: dst:%p, src:%p, chn:%d, chidx:%d",
              dst, src, chn, chidx);
    }
    for (i = 0; i < nSample; i++) {
        dst[i] = src[i * chn + chidx];
    }
}

static AWE_RET internal_aml_awe_process(AWE *awe, AML_MEM_HANDLE in[],
                    int32_t *inLenInByte, AML_MEM_HANDLE out[],
                    int32_t *outLenInByte, uint32_t *isWaked)
{
    AWE_RET ret = AWE_RET_OK;
    if (!awe) {
        printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
    aml_vsp_awe_process_param_in* pParamIn = AML_MEM_GetVirtAddr(awe->hInput);
    aml_vsp_awe_process_param_out* pParamOut = AML_MEM_GetVirtAddr(awe->hOutput);
    size_t output_size = awe->output_size;

    if (awe->inputMode == AWE_USER_INPUT_MODE) {
        pParamIn->lenInByte = *inLenInByte;
        pParamIn->numInChn = awe->micInChannels + awe->refInChannels;
        if (pParamIn->numInChn <= AWE_MAX_IN_CHANS) {
            int i;
            for (i = 0; i < pParamIn->numInChn; i++)
                pParamIn->pChanIn[i] = (uint64_t)AML_MEM_GetPhyAddr(in[i]);
        } else {
            printf("The input channel number is out of AWE capability: mic:%d, ref:%d\n",
                awe->micInChannels, awe->refInChannels);
            return AWE_RET_ERR_NOT_SUPPORT;
        }
    } else {
        pParamIn->lenInByte = 0;
        pParamIn->numInChn = 0;
    }

    pParamIn->lenOutByte = *outLenInByte;
    pParamIn->numOutChn = awe->outChannels;
    if (pParamIn->numOutChn <= AWE_MAX_OUT_CHANS) {
        int i;
        for (i = 0; i < pParamIn->numOutChn; i++)
            pParamIn->pChanOut[i] = (uint64_t)AML_MEM_GetPhyAddr(out[i]);
    } else {
        printf("The output channel number is out of AWE capability: out:%d\n",
            awe->outChannels);
        return AWE_RET_ERR_NOT_SUPPORT;
    }

    AML_MEM_Clean(awe->hInput, awe->input_size);
    ret = AML_VSP_Process(awe->hVsp, AML_MEM_GetPhyAddr(awe->hInput), awe->input_size,
                        AML_MEM_GetPhyAddr(awe->hOutput), &output_size);
    AML_MEM_Invalidate(awe->hOutput, output_size);
    *inLenInByte = pParamOut->lenInByte;
    *outLenInByte = pParamOut->lenOutByte;
    *isWaked = pParamOut->isWaked;
    return ret;
}

void* awe_thread_process_data(void * data)
{
    int i;
    AWE* awe = (AWE*)data;
    char* virOut[AWE_MAX_OUT_CHANS];
    uint32_t isWaked;
    int32_t inLen;
    int32_t outLen;
    while(!awe->work_thread_exit) {
        if (awe->inputMode == AWE_USER_INPUT_MODE) {
            sem_wait(&awe->userFillSem);
            uint32_t numOfChunk = aml_awe_ring_buf_fullness(awe->userFillBufSize,
                                           awe->userFillBufWr, awe->userFillBufRd)/(awe->inWorkBufLen*(awe->micInChannels + awe->refInChannels));
            //printf("%d, %d %d\n", numOfChunk, awe->inWorkBufLen, (awe->micInChannels + awe->refInChannels));
            /*wait sema here, thread unblock once user fill new pcm*/
            //printf("Need handle %d voice chunk, wr:%d, rd:%d\n", numOfChunk, awe->userFillBufWr, awe->userFillBufRd);
            while(numOfChunk--) {
                /*handle wrap around, copy the wrapped data to the of the buff, to ensure continuous*/
                if ((awe->userFillBufSize -  awe->userFillBufRd) < awe->inWorkBufLen*(awe->micInChannels + awe->refInChannels)) {
                    uint32_t block1 = awe->userFillBufSize -  awe->userFillBufRd;
                    uint32_t block2 = awe->inWorkBufLen*(awe->micInChannels + awe->refInChannels) - block1;
                    memcpy(awe->userFillBuf + awe->userFillBufSize, awe->userFillBuf, block2);
                }
                for (i = 0; i < (awe->micInChannels + awe->refInChannels); i++) {
                    aml_ch_extract_s16le((int16_t*)AML_MEM_GetVirtAddr(awe->hinWorkBuf[i]),
                                (int16_t*)(awe->userFillBuf + awe->userFillBufRd),
                                awe->inWorkBufLenInSample, (awe->micInChannels + awe->refInChannels), i);
                    AML_MEM_Clean(awe->hinWorkBuf, awe->inWorkBufLen);
                }
                isWaked = 0;
                inLen =  awe->inWorkBufLen;
                outLen = awe->outWorkBufLen;
                internal_aml_awe_process(awe, awe->hinWorkBuf, &inLen,
                                awe->houtWorkBuf, &outLen, &isWaked);
                awe->userFillBufRd = (awe->userFillBufRd + (awe->inWorkBufLen - inLen)*(awe->micInChannels + awe->refInChannels)) % awe->userFillBufSize;

                for (i = 0; i < awe->outChannels; i++) {
                    AML_MEM_Invalidate(awe->houtWorkBuf[i], awe->outWorkBufLen);
                    virOut[i] = (char*)AML_MEM_GetVirtAddr(awe->houtWorkBuf[i]);
                }
                /*throw out asr data*/
                if (!awe->awe_data_handler_func[AWE_DATA_TYPE_ASR]) {
                    printf("Do not install ASR data handler callback\n");
                    exit(0);
                }
                awe->awe_data_handler_func[AWE_DATA_TYPE_ASR](awe, AWE_DATA_TYPE_ASR,
                                     virOut[0], outLen,
                                     awe->awe_data_handler_userdata[AWE_DATA_TYPE_ASR]);

                /*throw out voip data*/
                if (!awe->awe_data_handler_func[AWE_DATA_TYPE_VOIP]) {
                    printf("Do not install VOIP data handler callback\n");
                    exit(0);
                }
                awe->awe_data_handler_func[AWE_DATA_TYPE_VOIP](awe, AWE_DATA_TYPE_VOIP,
                                     virOut[1], outLen,
                                     awe->awe_data_handler_userdata[AWE_DATA_TYPE_VOIP]);

                /*throw out event*/
                if (isWaked) {
                        if (!awe->awe_event_handler_func[AWE_EVENT_TYPE_WAKE]) {
                        printf("Do not install wake up event handler callback\n");
                        exit(0);
                    }
                    awe->awe_event_handler_func[AWE_EVENT_TYPE_WAKE](awe, AWE_EVENT_TYPE_WAKE, 0,
                                          NULL, awe->awe_event_handler_userdata[AWE_EVENT_TYPE_WAKE]);
                }
            }
        } else if (awe->inputMode == AWE_DSP_INPUT_MODE) {
            if (awe->bKickFreeRun == 1) {
                awe->bKickFreeRun = 0;
                sem_wait(&awe->userFillSem);
            }
            isWaked = 0;
            inLen = 0;
            outLen = awe->outWorkBufLen;
            internal_aml_awe_process(awe, awe->hinWorkBuf, &inLen,
                            awe->houtWorkBuf, &outLen, &isWaked);
            for (i = 0; i < awe->outChannels; i++) {
                 virOut[i] = (char*)AML_MEM_GetVirtAddr(awe->houtWorkBuf[i]);
             }
             /*throw out data*/
             if (!awe->awe_data_handler_func[AWE_DATA_TYPE_ASR]) {
                 printf("Do not install ASR data handler callback\n");
                 exit(0);
             }
             awe->awe_data_handler_func[AWE_DATA_TYPE_ASR](awe, AWE_DATA_TYPE_ASR,
                                             virOut[0], outLen,
                                             awe->awe_data_handler_userdata[AWE_DATA_TYPE_ASR]);

             /*throw out voip data*/
             if (!awe->awe_data_handler_func[AWE_DATA_TYPE_VOIP]) {
                 printf("Do not install VOIP data handler callback\n");
                 exit(0);
             }
             awe->awe_data_handler_func[AWE_DATA_TYPE_VOIP](awe, AWE_DATA_TYPE_VOIP,
                                  virOut[1], outLen,
                                  awe->awe_data_handler_userdata[AWE_DATA_TYPE_VOIP]);

             /*throw out event*/
             if (isWaked) {
                 if (!awe->awe_event_handler_func[AWE_EVENT_TYPE_WAKE]) {
                     printf("Do not install wake up event handler callback\n");
                     exit(0);
                 }
                 awe->awe_event_handler_func[AWE_EVENT_TYPE_WAKE](awe, AWE_EVENT_TYPE_WAKE, 0,
                                                  NULL, awe->awe_event_handler_userdata[AWE_EVENT_TYPE_WAKE]);
             }
        } else {
            printf("Impossible, invalid input mode:%d\n", awe->inputMode);
        }
    }
    return NULL;
}

AWE_RET AML_AWE_Create(AWE **awe)
{
    AWE *pawe = NULL;
    AWE_CHECK_NULL(awe);
    if (*awe != NULL) {
        printf("Dangerous, *awe should be initialized as NULL, *awe=%p\n", *awe);
        return AWE_RET_ERR_NOT_SUPPORT;
    }
    pawe = calloc(1, sizeof(AWE));
    if (!pawe) {
        printf("Calloc AWE structure failed\n");
        return AWE_RET_ERR_NO_MEM;
    }
    memset(pawe, 0, sizeof(AWE));
    pawe->hInput = AML_MEM_Allocate(sizeof(aml_vsp_awe_process_param_in));
    pawe->input_size = sizeof(aml_vsp_awe_process_param_in);
    pawe->hOutput = AML_MEM_Allocate(sizeof(aml_vsp_awe_process_param_out));
    pawe->output_size = sizeof(aml_vsp_awe_process_param_out);
    pawe->hParam = AML_MEM_Allocate(sizeof(AWE_PARA));
    pawe->param_size = sizeof(AWE_PARA);
    pawe->hVsp = AML_VSP_Init("AML.VSP.AWE", NULL, 0);

    if (pawe->hParam && pawe->hInput && pawe->hOutput && pawe->hVsp) {
        *awe = pawe;
        printf("Create AWE success: hVsp:%p hParam:%p hInput:%p hOutput:%p\n",
            pawe->hVsp, pawe->hParam, pawe->hInput, pawe->hOutput);
        (*awe)->aweStatus = AWE_STATUS_IDLE;
        return AWE_RET_OK;
    } else {
        printf("Allocate AWE resource failed: hVsp:%p hParam:%p hInput:%p hOutput:%p\n",
            pawe->hVsp, pawe->hParam, pawe->hInput, pawe->hOutput);
        if (pawe->hParam)
            AML_MEM_Free(pawe->hParam);
        if (pawe->hInput)
            AML_MEM_Free(pawe->hInput);
        if (pawe->hOutput)
            AML_MEM_Free(pawe->hInput);
        if (pawe->hVsp)
            AML_MEM_Free(pawe->hVsp);
        return AWE_RET_ERR_NO_MEM;
    }
}

AWE_RET AML_AWE_Destroy(AWE *awe)
{
    AWE_CHECK_NULL(awe);
    if (awe->hOutput) {
        AML_MEM_Free(awe->hOutput);
        awe->hOutput = NULL;
    }
    if (awe->hInput) {
        AML_MEM_Free(awe->hInput);
        awe->hInput= NULL;
    }
    if (awe->hParam) {
        AML_MEM_Free(awe->hParam);
        awe->hParam= NULL;
    }
    if (awe->hVsp) {
        AML_VSP_Deinit(awe->hVsp);
        awe->hVsp = NULL;
    }
    if (awe)
        free(awe);
    return 0;
}

AWE_RET AML_AWE_Open(AWE *awe)
{
    int i;
    AWE_RET ret;
    AWE_CHECK_NULL(awe);
    AWE_CHECK_STATUS(awe->aweStatus, AWE_STATUS_IDLE)
    AWE_PARA* para = (AWE_PARA*)AML_MEM_GetVirtAddr(awe->hParam);

    AML_VSP_GetParam(awe->hVsp, AWE_PARA_MIC_IN_CHANNELS, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    awe->micInChannels = para->micInChannels;

    AML_VSP_GetParam(awe->hVsp, AWE_PARA_REF_IN_CHANNELS, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    awe->refInChannels = para->refInChannels;

    AML_VSP_GetParam(awe->hVsp, AWE_PARA_OUT_CHANNELS, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    awe->outChannels = para->outChannels;

    AML_VSP_GetParam(awe->hVsp, AWE_PARA_IN_SAMPLE_RATE, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    awe->inSampleRate = para->inSampRate;

    AML_VSP_GetParam(awe->hVsp, AWE_PARA_IN_SAMPLE_BITS, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    awe->inSampleBit = para->inSampBits;

    AML_VSP_GetParam(awe->hVsp, AWE_PARA_INPUT_MODE, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    awe->inputMode = para->inputMode;
    printf("Open AWE: SampleRate: %d, Bitdepth:%d, inputMode:%d\n"
           "channels num: mic:%d, ref:%d, out:%d\n",
            awe->inSampleRate, awe->inSampleBit, awe->inputMode,
            awe->micInChannels, awe->refInChannels, awe->outChannels);

    ret = AML_VSP_Open(awe->hVsp);
    if (ret != AWE_RET_OK) {
        printf("Open hifi awe failed:%d\n", ret);
        goto table_failure_handling;
    }


    awe->inWorkBufLenInSample = awe->inSampleRate*VOICE_CHUNK_MS/1000;
    awe->inWorkBufLen = (awe->inSampleBit >> 3)*awe->inWorkBufLenInSample;
    for (i = 0; i < (awe->micInChannels + awe->refInChannels); i++) {
        awe->hinWorkBuf[i] = AML_MEM_Allocate(awe->inWorkBufLen);
        if (awe->hinWorkBuf[i] == NULL) {
            printf("Failed to allocate input work buf, ch:%d", i);
            goto table_failure_handling;
        }
    }

    awe->outWorkBufLen = awe->inWorkBufLen*4;
    for (i = 0; i < awe->outChannels; i++) {
        awe->houtWorkBuf[i] = AML_MEM_Allocate(awe->outWorkBufLen);
        if (awe->houtWorkBuf[i] == NULL) {
            printf("Failed to allocate output work buf, ch:%d", i);
            goto table_failure_handling;
        }
    }

    awe->userFillBufSize = VOICE_CHUNK_NUM*(awe->micInChannels + awe->refInChannels)*awe->inWorkBufLen;
    /*one more chunk here is for handling wrap around*/
    awe->userFillBuf = malloc(awe->userFillBufSize + (awe->micInChannels + awe->refInChannels)*awe->inWorkBufLen);
    awe->userFillBufRd = awe->userFillBufWr = 0;
    if (awe->userFillBuf == NULL) {
        printf("Failed to allocate user fill ring buf\n");
        goto table_failure_handling;
    }

    ret = sem_init(&awe->userFillSem, 0, 0);
    if (ret != 0)
    {
        printf("create user filling sem error. %d: %s\n",ret,strerror(ret));
        goto table_failure_handling;
    }
    awe->work_thread_exit  = 0;
    awe->bKickFreeRun = 0;
    ret = pthread_create(&awe->work_thread, NULL, awe_thread_process_data, (void*)awe);
    if (ret != 0)
    {
        printf("create working thread error. %d: %s\n",ret,strerror(ret));
        goto table_failure_handling;
    }
    pthread_setname_np(awe->work_thread, "awe_thread_process_data");
    awe->aweStatus = AWE_STATUS_EXECUTE;
    goto table_end;
table_failure_handling:
    awe->work_thread_exit = 1;
    sem_post(&awe->userFillSem);
    pthread_join(awe->work_thread,NULL);
    sem_destroy(&awe->userFillSem);
    if (awe->userFillBuf)
        free(awe->userFillBuf);

    for (i = 0; i < (awe->micInChannels + awe->refInChannels); i++) {
        if (awe->hinWorkBuf[i])
            AML_MEM_Free(awe->hinWorkBuf[i]);
    }

    for (i = 0; i < awe->outChannels; i++) {
        if (awe->houtWorkBuf[i])
            AML_MEM_Free(awe->houtWorkBuf[i]);
    }
table_end:
	return ret;
}

AWE_RET AML_AWE_Close(AWE *awe)
{
    int i;
    AWE_RET ret;
    AWE_CHECK_NULL(awe);
    AWE_CHECK_STATUS(awe->aweStatus, AWE_STATUS_EXECUTE);
    awe->work_thread_exit = 1;
    sem_post(&awe->userFillSem);
    pthread_join(awe->work_thread,NULL);

    for (i = 0; i < (awe->micInChannels + awe->refInChannels); i++) {
        if (awe->hinWorkBuf[i])
            AML_MEM_Free(awe->hinWorkBuf[i]);
    }

    for (i = 0; i < awe->outChannels; i++) {
        if (awe->houtWorkBuf[i])
            AML_MEM_Free(awe->houtWorkBuf[i]);
    }

    if (awe->userFillBuf)
        free(awe->userFillBuf);
    if (awe->hVsp)
        ret = AML_VSP_Close(awe->hVsp);
    sem_destroy(&awe->userFillSem);
    return ret;
}

AWE_RET AML_AWE_SetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para)
{
    AWE_RET ret = AWE_RET_OK;
    AWE_CHECK_NULL(awe);
    AWE_CHECK_NULL(para);
    char* pParam = (char*)AML_MEM_GetVirtAddr(awe->hParam);
    memcpy(pParam, para, sizeof(AWE_PARA));
    AML_MEM_Clean(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);

    if (paraId == AWE_PARA_FREE_RUN_MODE) {
        if(para->freeRunMode == 1) {
            awe->bKickFreeRun = 1;
            while(awe->bKickFreeRun)
                NULL;
            ret = AML_VSP_SetParam(awe->hVsp, (int32_t)paraId, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
        } else {
            ret = AML_VSP_SetParam(awe->hVsp, (int32_t)paraId, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
            sem_post(&awe->userFillSem);
        }
    } else
        ret = AML_VSP_SetParam(awe->hVsp, (int32_t)paraId, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    return ret;
}

AWE_RET AML_AWE_GetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para)
{
    AWE_RET ret = AWE_RET_OK;
    AWE_CHECK_NULL(awe);
    AWE_CHECK_NULL(para);
    ret = AML_VSP_GetParam(awe->hVsp, (int32_t)paraId, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
    char* pParam = (char*)AML_MEM_GetVirtAddr(awe->hParam);
    memcpy(para, pParam, sizeof(AWE_PARA));
    return ret;
}

AWE_RET AML_AWE_Process(AWE *awe, void* in[],
                    int32_t *inLenInByte, void* out[],
                    int32_t *outLenInByte, uint32_t *isWaked)
{
    AWE_RET ret = AWE_RET_OK;
    AWE_CHECK_NULL(awe);
    AWE_CHECK_NULL(inLenInByte);
    AWE_CHECK_NULL(outLenInByte);
    AWE_CHECK_STATUS(awe->aweStatus, AWE_STATUS_EXECUTE);
    if (awe->inputMode == AWE_USER_INPUT_MODE) {
        unsigned int i;
        int32_t inLen = AMX_MIN(*inLenInByte, awe->inWorkBufLen);
        int32_t outLen = AMX_MIN(*outLenInByte, awe->outWorkBufLen);
        for (i = 0; i < (awe->refInChannels + awe->micInChannels); i++) {
            memcpy(AML_MEM_GetVirtAddr(awe->hinWorkBuf[i]), in[i], inLen);
            AML_MEM_Clean(AML_MEM_GetPhyAddr(awe->hinWorkBuf[i]), inLen);
        }
        ret = internal_aml_awe_process(awe, awe->hinWorkBuf, &inLen, awe->houtWorkBuf, &outLen, isWaked);
        for (i = 0; i < awe->outChannels; i++) {
            AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->houtWorkBuf[i]), outLen);
            memcpy(out[i], AML_MEM_GetVirtAddr(awe->houtWorkBuf[i]), outLen);
        }
        *inLenInByte = inLen;
        *outLenInByte = outLen;
    }
    else {
        printf("AWE is not worked at user input mode. current input mode:%d\n", awe->inputMode);
        ret = AWE_RET_ERR_NOT_SUPPORT;
    }
    return ret;
}

AWE_RET AML_AWE_PushBuf(AWE *awe, const char *data, size_t size)
{
    AWE_CHECK_NULL(awe);
    AWE_CHECK_NULL(data);
    AWE_CHECK_STATUS(awe->aweStatus, AWE_STATUS_EXECUTE);
    if (awe->inputMode == AWE_DSP_INPUT_MODE) {
        printf("Do not support this API when input is from dsp\n");
        return AWE_RET_ERR_NOT_SUPPORT;
    }
    if (awe->userFillBuf) {
        //printf("wr:%d rd:%d.size:%d\n\n", awe->userFillBufWr, awe->userFillBufRd, awe->userFillBufSize);
        if (aml_awe_ring_buf_space(awe->userFillBufSize, awe->userFillBufWr, awe->userFillBufRd) < size) {
            /*printf("No enough space to fill pcm: rd:%d wr:%d size:%d\n",
                awe->userFillBufRd, awe->userFillBufWr, awe->userFillBufSize);*/
            return AWE_RET_ERR_NO_MEM;
        }
        if (size <= (awe->userFillBufSize - awe->userFillBufWr)) {
            memcpy(awe->userFillBuf + awe->userFillBufWr, data, size);
        } else {
            uint32_t block1 = awe->userFillBufSize - awe->userFillBufWr;
            uint32_t block2 = size -block1;
            memcpy(awe->userFillBuf + awe->userFillBufWr, data, block1);
            memcpy(awe->userFillBuf, data + block1, block2);
        }
        awe->userFillBufWr = (awe->userFillBufWr + size) % awe->userFillBufSize;
        /*post semaphore here, tell work thread user fills new pcm*/
        sem_post(&awe->userFillSem);
        return AWE_RET_OK;
    } else {
        printf("Does not allocate ring buffer\n");
        return AWE_RET_ERR_NO_MEM;
    }
}

AWE_RET AML_AWE_AddDataHandler(AWE *awe, const AWE_DATA_TYPE type,
                                                 AML_AWE_DataHandler handler,
                                                 void *user_data)
{
    AWE_CHECK_NULL(awe);
    AWE_CHECK_NULL(handler);
    AWE_CHECK_STATUS(awe->aweStatus, AWE_STATUS_IDLE);
    if (type >= AWE_DATA_TYPE_MAX) {
        printf("Invalid data type\n");
        return AWE_RET_ERR_NOT_SUPPORT;
    }
    if (awe->awe_data_handler_func[type] != NULL)
        printf("New data handler override old one\n");
    awe->awe_data_handler_func[type] = handler;
    awe->awe_data_handler_userdata[type] = user_data;
    return AWE_RET_OK;

}

AWE_RET AML_AWE_AddEventHandler(AWE *awe, const AWE_EVENT_TYPE type,
                                                  AML_AWE_EventHandler handler,
                                                  void *user_data)
{
    AWE_CHECK_NULL(awe);
    AWE_CHECK_NULL(handler);
    AWE_CHECK_STATUS(awe->aweStatus, AWE_STATUS_IDLE);
    if (type >= AWE_EVENT_TYPE_MAX) {
        printf("Invalid event type\n");
        return AWE_RET_ERR_NOT_SUPPORT;
    }
    if (awe->awe_event_handler_func[type] != NULL)
        printf("New event handler override old one\n");
    awe->awe_event_handler_func[type] = handler;
    awe->awe_event_handler_userdata[type] = user_data;
    return AWE_RET_OK;
}

