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
 * hifi4 rpc gain api sample codes
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "aml_pcm_gain_api.h"

#define CHUNK_MS 10
int aml_pcm_gain_unit_test(int argc, char* argv[]) {
    int i = 0;
    PCMGAIN *pcmgain = NULL;
    PCMGAIN_PARA pcmgain_para;
    PCMGAIN_RET pcmgain_ret = PCMGAIN_RET_OK;
    void *in = NULL;
    void *out = NULL;
    int32_t sampleRate = 0;
    int32_t sampleBytes = 0;
    int32_t chnNum = 0;
    int32_t chunkBytes = 0;
    int32_t chunkFrames = 0;
    int32_t gain = 0;

    if (argc != 6) {
        printf("Please input sampleRate, bitdepth, channels, gain, input, output\n");
        return -1;
    }

    /*Initilaize input, output here*/
    sampleRate = atoi(argv[0]);
    sampleBytes = (atoi(argv[1]) >> 3);
    chnNum = atoi(argv[2]);
    gain = atoi(argv[3]);
    chunkBytes = CHUNK_MS*sampleRate*sampleBytes*chnNum/1000;
    chunkFrames = CHUNK_MS*sampleRate/1000;
    in = malloc(chunkBytes);
    out = malloc(chunkBytes);
    if (!in || !out) {
        printf("Can not allocate buffer:%p %p\n", in, out);
        pcmgain_ret = -1;
        goto end_tab;
    }

    FILE *fIn = fopen(argv[4], "rb");
    FILE *fOut = fopen(argv[5], "w+b");
    if ( !fIn || !fOut) {
        printf("Can not open io file:%p %p\n", fIn, fOut);
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_ret = AML_PCMGAIN_Create(&pcmgain);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Can not create Hifi PCMGAIN service\n");
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_ret = AML_PCMGAIN_GetParam(pcmgain, PCMGAIN_PARA_SUPPORT_SAMPLE_RATES, &pcmgain_para);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Get supported samperate fail:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }

    for (i = 0; i < pcmgain_para.numOfSupportSampRates; i++) {
        if (sampleRate == pcmgain_para.supportSampRates[i])
            break;
    }
    if (i == pcmgain_para.numOfSupportSampRates) {
        printf("Invalid sample rate\n");
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_para.SampRate = sampleRate;
    pcmgain_ret = AML_PCMGAIN_SetParam(pcmgain, PCMGAIN_PARA_SAMPLE_RATE, &pcmgain_para);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Set samperate fail:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }


    pcmgain_ret = AML_PCMGAIN_GetParam(pcmgain, PCMGAIN_PARA_SUPPORT_SAMPLE_BITS, &pcmgain_para);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Failed to get sample bits:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }
    for (i = 0; i < pcmgain_para.numOfSupportSampBits; i++) {
        if ((sampleBytes << 3) == pcmgain_para.supportSampBits[i])
            break;
    }
    if (i == pcmgain_para.numOfSupportSampBits) {
        printf("Invalid bit depth\n");
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_para.SampBits = (sampleBytes << 3);
    pcmgain_ret = AML_PCMGAIN_SetParam(pcmgain, PCMGAIN_PARA_SAMPLE_BITS, &pcmgain_para);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Failed to set sample bits:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_para.ChNum = chnNum;
    pcmgain_ret = AML_PCMGAIN_SetParam(pcmgain, PCMGAIN_PARA_CH_NUM, &pcmgain_para);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Failed to set channels:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_ret = AML_PCMGAIN_Open(pcmgain);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Failed to open PCMGAIN:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }

    pcmgain_para.Gain = gain;
    pcmgain_ret = AML_PCMGAIN_SetParam(pcmgain, PCMGAIN_PARA_GAIN, &pcmgain_para);
    if (pcmgain_ret != PCMGAIN_RET_OK) {
        printf("Failed to set gain:%d\n", pcmgain_ret);
        pcmgain_ret = -1;
        goto end_tab;
    }

    while (1) {
        uint32_t nbyteRead = chunkBytes;
        nbyteRead = fread(in, 1, nbyteRead, fIn);
        if (nbyteRead < chunkBytes) {
            printf("EOF\n");
            break;
        }
        AML_PCMGAIN_Process(pcmgain, in, out, chunkFrames);
        fwrite(out, 1, chunkBytes, fOut);
    }

end_tab:
    if (pcmgain)
        AML_PCMGAIN_Close(pcmgain);
    if (pcmgain)
        AML_PCMGAIN_Destroy(pcmgain);
    if (in)
        free(in);
    if (out)
        free(out);
    if (fIn)
        fclose(fIn);
    if (fOut)
        fclose(fOut);
    return pcmgain_ret;
}
