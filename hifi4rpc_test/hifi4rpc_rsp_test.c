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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "aml_resampler.h"

#define VOICE_MS 48
int aml_rsp_unit_test(int argc, char* argv[]) {
    int ret = 0;
    size_t bytesRead = 0;
    size_t bytesWrite = 0;
    FILE* voicefile = NULL;
    FILE* outfile = NULL;
    void* inputBuf = NULL;
    void* outputBuf = NULL;
    int32_t inRate = 48000;
    int32_t outRate = 16000;
    resampler_handle* rsp = NULL;

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
    inputBuf = malloc(voiceChunkInByte);
    outputBuf = malloc(voiceChunkInByte*outRate/inRate);

    rsp = aml_resampler_init(inRate, outRate);
    if (!rsp) {
        printf("Initialize resampler failure\n");
        ret = -1;
        goto tab_end;
    }
    printf("Init resampler hdl=%p\n", rsp);

    while (1) {
        bytesRead = voiceChunkInByte;
        bytesRead = fread(inputBuf, 1, bytesRead, voicefile);
        if (!bytesRead) {
            printf("EOF\n");
            break;
        }

        bytesWrite = bytesRead*outRate/inRate;
        ret = aml_resampler(rsp, outputBuf, inputBuf,bytesRead);

        if (ret != 0) {
            printf("resampler encountered error:0x%x\n", ret);
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
    if (inputBuf)
        free(inputBuf);
    if (outputBuf)
        free(outputBuf);
    if (rsp)
        aml_resampler_destroy(rsp);

    return ret;
}

