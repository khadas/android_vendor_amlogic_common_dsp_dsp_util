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
 * hifi4 rpc client api sample codes, vsp
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "rpc_client_vsp.h"

typedef struct {
    int32_t	  Fs_Hz_in;
    int32_t	  Fs_Hz_out;
} __attribute__((packed)) aml_vsp_st_param;

/*Just an example show how to apply meta data*/
typedef struct {
    int32_t	  Fs;
    int32_t   Bitdepth;
} __attribute__((packed)) aml_vsp_meta_param;

#define VOICE_MS 48
int offload_vsp_rsp(int argc, char* argv[]) {
    int ret = 0;
    int Vsp_Err = 0;
    AML_VSP_HANDLE hdlvsp = 0;
    AML_MEM_HANDLE hParam = 0;
    AML_MEM_HANDLE hShmInput =0;
    AML_MEM_HANDLE hShmOutput = 0;
    uint8_t *paramBuf = 0;
    uint8_t *inputBuf = 0;
    uint8_t *outputBuf = 0;
    void* paramphy = 0;
    void* inputphy = 0;
    void* outputphy = 0;
    size_t bytesRead = 0;
    size_t bytesWrite = 0;
    aml_vsp_st_param* st_param = NULL;
    aml_vsp_meta_param* meta_param = NULL;
    FILE* voicefile = NULL;
    FILE* outfile = NULL;
    int32_t inRate = 48000;
    int32_t outRate = 16000;
    if (argc == 4) {
        inRate = atoi(argv[2]);
        outRate = atoi(argv[3]);
    }
    int32_t samplePerMs = inRate/1000;
    int32_t voiceChunkInByte = (2*samplePerMs*VOICE_MS);

    // Open the input voice file.
    voicefile = fopen(argv[0], "rb");
    if (!voicefile) {
        printf("Open input file failure %s\n", argv[0]);
        ret = -1;
        goto tab_end;
    }
    printf("Open voice file\n");

    outfile = fopen(argv[1], "w+b");
    if (!outfile) {
        printf("Open output file failure %s\n", argv[1]);
        ret = -1;
        goto tab_end;
    }
    printf("Open output file\n");



    // Allocate input buffer.
    hShmInput = AML_MEM_Allocate(sizeof(aml_vsp_meta_param) + voiceChunkInByte);
    inputBuf = (uint8_t*)AML_MEM_GetVirtAddr(hShmInput);
    inputphy = AML_MEM_GetPhyAddr(hShmInput);

    // Allocate output buffer.
    hShmOutput = AML_MEM_Allocate(voiceChunkInByte*outRate/inRate);
    outputBuf = (uint8_t*)AML_MEM_GetVirtAddr(hShmOutput);
    outputphy = AML_MEM_GetPhyAddr(hShmOutput);

    // Allocate static param buffer, the param is used to initialize vsp
    hParam = AML_MEM_Allocate(sizeof(aml_vsp_st_param));
    paramBuf = (uint8_t*)AML_MEM_GetVirtAddr(hParam);
    paramphy = AML_MEM_GetPhyAddr(hParam);

    // Initialize the vsp-rsp.
    st_param = (aml_vsp_st_param*)paramBuf;
    st_param->Fs_Hz_in = inRate;
    st_param->Fs_Hz_out = outRate;
    AML_MEM_Clean(paramphy, sizeof(aml_vsp_st_param));
    hdlvsp = AML_VSP_Init(AML_VSP_RESAMPLER, (void*)paramphy, sizeof(aml_vsp_st_param));
    if (!hdlvsp) {
        printf("Initialize vsp failure\n");
        ret = -1;
        goto tab_end;
    }
    printf("Init vsp-rsp hdl=%p\n", hdlvsp);

    while (1) {
        //example to show apply meta data associate with the buffer.
        meta_param = (aml_vsp_meta_param*)inputBuf;
        meta_param->Bitdepth = 16;
        meta_param->Fs = inRate;
        bytesRead = voiceChunkInByte;
        bytesRead = fread(inputBuf + sizeof(aml_vsp_meta_param), 1, bytesRead, voicefile);
        if (!bytesRead) {
            printf("EOF\n");
            break;
        }

        bytesWrite = bytesRead*outRate/inRate;
        AML_MEM_Clean(inputphy, bytesRead + sizeof(aml_vsp_meta_param));
        Vsp_Err = AML_VSP_Process(hdlvsp, inputphy, bytesRead + sizeof(aml_vsp_meta_param), outputphy, &bytesWrite);
        AML_MEM_Invalidate(outputphy, bytesWrite);

        if (Vsp_Err != 0) {
            printf("Decoder encountered error:0x%x\n", Vsp_Err);
            ret = -1;
            break;
        }
        if (outfile)
            fwrite(outputBuf, 1, bytesWrite, outfile);
    }
    printf("voice signal resampling done\n");

    tab_end:
    // Close input reader and output writer.
    if (voicefile)
        fclose(voicefile);
    if (outfile)
        fclose(outfile);

    // Free allocated memory.
    if (hShmInput)
        AML_MEM_Free((AML_MEM_HANDLE)hShmInput);
    if (hShmOutput)
        AML_MEM_Free((AML_MEM_HANDLE)hShmOutput);
    if (hParam)
        AML_MEM_Free((AML_MEM_HANDLE)hParam);
    if (hdlvsp)
        AML_VSP_Deinit(hdlvsp);

    return ret;
}

