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
 * amlogic audio resampler api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rpc_client_shm.h"
#include "rpc_client_vsp.h"
#include "aml_resampler.h"
#include "generic_macro.h"

typedef struct {
    int32_t Fs_Hz_in;
    int32_t Fs_Hz_out;
} __attribute__((packed)) aml_vsp_st_param;

/*Just an example show how to apply meta data*/
typedef struct {
    int32_t Fs;
    int32_t Bitdepth;
} __attribute__((packed)) aml_vsp_meta_param;

struct resampler {
    AML_VSP_HANDLE hdlvsp;
    AML_MEM_HANDLE hShmIn;
    AML_MEM_HANDLE hShmOut;
    int32_t voiceChunkInByte;
    int32_t inRate;
    int32_t outRate;
};

#define VOICE_MS 48
resampler_handle *aml_resampler_init(
        int Fs_Hz_in,                   /* I    Input sampling rate (Hz)                                    */
        int Fs_Hz_out                   /* I    Output sampling rate (Hz)                                   */
)
{
    AML_VSP_HANDLE hdlvsp = 0;
    AML_MEM_HANDLE hParam = 0;
    uint8_t *paramBuf = 0;
    void* paramphy = 0;
    aml_vsp_st_param* st_param = NULL;
    struct resampler* rsp = malloc(sizeof(struct resampler));

    int32_t samplePerMs = Fs_Hz_in/1000;
    int32_t voiceChunkInByte = (2*samplePerMs*VOICE_MS);

    // Allocate static param buffer, the param is used to initialize vsp
    hParam = AML_MEM_Allocate(sizeof(aml_vsp_st_param));
    paramBuf = (uint8_t*)AML_MEM_GetVirtAddr(hParam);
    paramphy = AML_MEM_GetPhyAddr(hParam);

    // Initialize the vsp-rsp.
    st_param = (aml_vsp_st_param*)paramBuf;
    st_param->Fs_Hz_in = Fs_Hz_in;
    st_param->Fs_Hz_out = Fs_Hz_out;
    AML_MEM_Clean(paramphy, sizeof(aml_vsp_st_param));
    hdlvsp = AML_VSP_Init("AML.VSP.RSP", (void*)paramphy, sizeof(aml_vsp_st_param));
    if (!hdlvsp) {
        printf("Initialize vsp failure\n");
        goto tab_end;
    } else {
        printf("Init vsp-rsp hdl=%p\n", hdlvsp);
        rsp->hdlvsp = hdlvsp;
        rsp->hShmIn = AML_MEM_Allocate(sizeof(aml_vsp_meta_param) + voiceChunkInByte);
        rsp->hShmOut = AML_MEM_Allocate(voiceChunkInByte*Fs_Hz_out/Fs_Hz_in);
        rsp->voiceChunkInByte = voiceChunkInByte;
        rsp->inRate = Fs_Hz_in;
        rsp->outRate = Fs_Hz_out;
    }
tab_end:
    AML_MEM_Free((AML_MEM_HANDLE)hParam);
    return rsp;
}

int aml_resampler(
        resampler_handle *p_resampler,  /* I/O  Resampler state                                             */
        short out[],                    /* O    Output signal                                               */
        const short in[],               /* I    Input signal                                                */
        int inLen                       /* I    Number of input samples                                     */
)
{
    int ret = 0;
    aml_vsp_meta_param* meta_param;
    struct resampler* rsp = (struct resampler*)p_resampler;
    uint8_t* inputBuf = (uint8_t*)AML_MEM_GetVirtAddr(rsp->hShmIn);
    void* inputphy = (void*)AML_MEM_GetPhyAddr(rsp->hShmIn);
    uint8_t* outputBuf = (uint8_t*)AML_MEM_GetVirtAddr(rsp->hShmOut);
    void* outputphy = (void*)AML_MEM_GetPhyAddr(rsp->hShmOut);
    int inidx = 0;
    int outidx = 0;

    meta_param = (aml_vsp_meta_param*)inputBuf;
    meta_param->Bitdepth = 16;
    meta_param->Fs = rsp->inRate;
    while (inLen > 0) {
        size_t bytesRead = 0;
        bytesRead = AMX_MIN(inLen, rsp->voiceChunkInByte);
        memcpy(inputBuf + sizeof(aml_vsp_meta_param), &in[inidx], bytesRead);
        inLen -= bytesRead;

        size_t bytesWrite = bytesRead*rsp->outRate/rsp->inRate;
        AML_MEM_Clean(inputphy, bytesRead + sizeof(aml_vsp_meta_param));
        ret = AML_VSP_Process(rsp->hdlvsp, inputphy, bytesRead + sizeof(aml_vsp_meta_param), outputphy, &bytesWrite);
        AML_MEM_Invalidate(outputphy, bytesWrite);

        if (ret != 0) {
            printf("Resampler encountered error:0x%x\n", ret);
            ret = -1;
            break;
        }
        memcpy(&out[outidx], outputBuf, bytesWrite);
        inidx += bytesRead;
        outidx += bytesWrite;
    }
    return ret;
}

int aml_resampler_destroy(
        resampler_handle *p_resampler   /* I    Input sampling rate (Hz)                                    */
)
{
    struct resampler* rsp = (struct resampler*)p_resampler;
    AML_VSP_Deinit(rsp->hdlvsp);
    AML_MEM_Free(rsp->hShmIn);
    AML_MEM_Free(rsp->hShmOut);
    free(rsp);
    return 0;
}



