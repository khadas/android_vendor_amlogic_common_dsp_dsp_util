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
 * amlogic wake up engine(AWE) API.
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

#include <stdint.h>
#include "rpc_client_shm.h"
#define AWE_MAX_IN_CHANS 4
#define AWE_MAX_OUT_CHANS 2

typedef struct _AWE AWE;

typedef enum {
    /*dsp feed input*/
    AWE_DSP_INPUT_MODE,
    /*user application feed input*/
    AWE_USER_INPUT_MODE
} AWE_INPUT_MODE;

typedef union {
    int32_t inSampRate;
    int32_t inSampBits;
    struct {
        int32_t numOfSupportSampRates;
        int32_t supportSampRates[4];
    };
    struct {
        int32_t numOfSupportSampBits;
        int32_t supportSampBits[4];
    };
    int32_t micInChannels;
    int32_t refInChannels;
    int32_t outChannels;
    uint32_t freeRunMode;
    AWE_INPUT_MODE inputMode;
} __attribute__((packed)) AWE_PARA;

/**
 *  wake event payload structure
 */
typedef struct {
    const char *word; /* Text of wake/hot word */
    const char *data; /* Samples of wake/hot word */
    size_t size;      /* Total bytes of samples */
    float angle;      /* DOA result */
    float score;      /* Wake confidence */
} AWE_WAKE;

/*
 */
typedef enum {
    /* static params */
    /* static params can only be set before AWE_Open */
    AWE_PARA_INPUT_MODE,
    AWE_PARA_IN_SAMPLE_RATE,
    AWE_PARA_IN_SAMPLE_BITS,
    AWE_PARA_MIC_IN_CHANNELS,
    AWE_PARA_REF_IN_CHANNELS,
    AWE_PARA_OUT_CHANNELS,
    /* dynamic params */
    AWE_PARA_FREE_RUN_MODE,
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

typedef enum {
    AWE_DATA_TYPE_INVALID,
    /* AWE optimized for ASR */
    AWE_DATA_TYPE_ASR,
    /* AWE optimized for VAD */
    AWE_DATA_TYPE_VAD,
    /* AWE optimized for VOIP */
    AWE_DATA_TYPE_VOIP,
    AWE_DATA_TYPE_MAX
} AWE_DATA_TYPE;


typedef enum {
    AWE_EVENT_TYPE_INVALID,
    /* AWE event for local VAD */
    AWE_EVENT_TYPE_VAD,
    /* AWE event for wake detected, payload is `AWE_WAKE` */
    AWE_EVENT_TYPE_WAKE,
    /* AWE event for hot word detected, payload is `AWE_WAKE` */
    AWE_EVENT_TYPE_HOTWORD,
    AWE_EVENT_TYPE_MAX
} AWE_EVENT_TYPE;

/**
 * Create and initialize AWE instance
 *
 * @param[out] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_Create(AWE **awe);

/**
 * Destory AWE instance and all resources allocated for AWE instance.
 *
 * @param[in] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_Destroy(AWE *awe);

/**
 * Enable AWE instance
 *
 * All static params should be configured through AML_AWE_SetParam
 * befor this call. AWE is moved to working status after this call
 *
 * @param[in] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_Open(AWE *awe);

/**
 * Disable AWE instance
 *
 * The API AML_AWE_Process can not working after this call.
 *
 * @param[in] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_Close(AWE *awe);

/**
 * Configure AWE parameter
 *
 * Static params is configured before AML_AWE_Open.
 * Dynamic params can be configured after AML_AWE_Open.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] identification of a parameter see AWE_PARA_ID
 *
 * @param[in] value of a parameter, associated with paraId,
 * see AWE_PARA
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_SetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para);

/**
 * Obtain AWE parameter
 *
 * @param[in] AWE instance handler
 *
 * @param[in] identification of a parameter see AWE_PARA_ID
 *
 * @param[out] value of a parameter, associated with paraId,
 * see AWE_PARA
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_GetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para);

/**
 * Synchronous AWE data processing entry.
 * Application feed pcm here, application get output
 * and wake up result here.
 * Note: This api only work when AWE input mode is configured
 * as AWE_USER_INPUT_MODE
 *
 * @param[in] AWE instance handler
 *
 * @param[in] Array of shared memory hanlders. A member of
 * this array represent a input pcm channel. The shared memory
 * is filled with mic or reference pcm by application.
 * The pcm is in none interleave format, mic0|mic1|ref0|ref1
 * Max supported input channels see AWE_MAX_IN_CHANS
 *
 * @param[in/out] Length in bytes per channel. Return
 * remained pcm in bytes after the call
 *
 * @param[out] Aarry of shared memory hanlders. A member of
 * this array represent a output pcm channel. The share memory
 * is filled with processed pcm by AWE. The pcm in none interleave
 * format, out0|out1
 * Max supported channel see AWE_MAX_OUT_CHANS
 *
 * @param[in/out] Space in bytes of a output channel. Return processed
 * pcm length in bytes
 *
 * @param[out] A flag indicates whether wake up words is spotted
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_Process(AWE *awe, AML_MEM_HANDLE in[], int32_t *inLenInByte, AML_MEM_HANDLE out[],
        int32_t *outLenInByte, uint32_t *isWaked);

/**
 * Asynchronous data processing entry
 *
 * Application feed pcm here.
 * Application obtain processed pcm through AML_AWE_DataHandler
 * Application process wake up word detect in AML_AWE_EventHandler
 * Note: This API is invalid when AWE input mode is configured as
 * AWE_DSP_INPUT_MODE.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] Pcm buffer, no interleaved format mic0|mic1|ref0|ref1
 *
 * @param[in] size in total bytes
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
AWE_RET AML_AWE_PushBuf(AWE *awe, const char *data, size_t size);


/**
 * AWE processed data handler
 *
 * AEC processed pcm can be obtained here. The implementation should copy
 * the data and return ASAP (< 1ms), otherwise the worker thread would be blocked.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE data processing type, see AWE_DATA_TYPE
 *
 * @param[in] Pcm processed data, mono channel
 *
 * @param[in] size in bytes per channel
 *
 * @param[in] user_data User-defined data
 */
typedef void (*AML_AWE_DataHandler)(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data);

/**
 * AWE event handler
 *
 * Notify application a certain event appear, for example, wakeup words detect.
 * The implementation should handle event and return ASAP (< 1ms),
 * otherwise the worker thread would be blocked.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE event type, see AWE_EVENT_TYPE
 *
 * @param[in] Event code, does not define so far
 *
 * @param[in] Event payload
 *
 * @param[in] user_data User-defined data
 */
typedef void (*AML_AWE_EventHandler)(AWE *awe, const AWE_EVENT_TYPE type, int32_t code,
                                             const void *payload, void *user_data);
/**
 * Add a data handler associated with data type
 *
 * Multiple data handlers could be added.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE data processing type, see AWE_DATA_TYPE
 *
 * @param[in] callback function to handle data
 *
 * @param[in] user_data User-defined data
 */
AWE_RET AML_AWE_AddDataHandler(AWE *awe, const AWE_DATA_TYPE type,
                                                 AML_AWE_DataHandler handler,
                                                 void *user_data);
/**
 * Add a event handler associated with event type
 *
 * Multiple v handlers could be added.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE event type, see AWE_EVENT_TYPE
 *
 * @param[in] callback function to handle event
 *
 * @param[in] user_data User-defined data
 */
AWE_RET AML_AWE_AddEventHandler(AWE *awe, const AWE_EVENT_TYPE type,
                                                  AML_AWE_EventHandler handler,
                                                  void *user_data);



#ifdef __cplusplus
}
#endif

#endif /* _AMLOGIC_WAKE_ENGINE_H_ */
