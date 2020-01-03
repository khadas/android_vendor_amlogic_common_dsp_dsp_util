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
 * hifi4 rpc client api sample codes, offload codecs
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
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "mp3reader.h"
#include "sndfile.h"
#include "aipc_type.h"
#include "rpc_client_aac.h"
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"

int mp3_offload_dec(int argc, char* argv[]) {
    int bUserAllocShm = 0;
    tAmlMp3DecHdl hdlmp3 = 0;
    AML_MEM_HANDLE hShmInput =0;
    AML_MEM_HANDLE hShmOutput = 0;
    uint8_t *inputBuf = 0;
    int16_t *outputBuf = 0;
    void* inputphy = 0;
    void* outputphy = 0;

    // Initialize the config.
    tAmlACodecConfig_Mp3DecExternal config;
    memset(&config, 0, sizeof(tAmlACodecConfig_Mp3DecExternal));
    config.equalizerType = flat;
    config.crcEnabled = false;

    // Initialize the decoder.
    hdlmp3 = AmlACodecInit_Mp3Dec(&config);
    printf("Init mp3dec hdl=%p\n", hdlmp3);

    // Open the input file.
    Mp3Reader mp3Reader;
    printf("Read mp3 file:%s\n", argv[0]);
    uint32_t success = mp3Reader.init(argv[0]);
    if (!success) {
        fprintf(stderr, "Encountered error reading %s\n", argv[0]);
        AmlACodecDeInit_Mp3Dec(hdlmp3);
        return EXIT_FAILURE;
    }
    printf("Init mp3reader\n");

    // Open the output file.
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(SF_INFO));
    sfInfo.channels = mp3Reader.getNumChannels();
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfInfo.samplerate = mp3Reader.getSampleRate();
    SNDFILE *handle = sf_open(argv[1], SFM_WRITE, &sfInfo);
    if (handle == NULL) {
        fprintf(stderr, "Encountered error writing %s\n", argv[1]);
        mp3Reader.close();
        AmlACodecDeInit_Mp3Dec(hdlmp3);
        return EXIT_FAILURE;
    }
    printf("Init outfile=%s\n", argv[1]);

    if (argc == 3)
        bUserAllocShm = atoi(argv[2]);
    if (bUserAllocShm) {
        // Allocate input buffer.
        hShmInput = AML_MEM_Allocate(kInputBufferSize);
        inputBuf = (uint8_t*)AML_MEM_GetVirtAddr(hShmInput);
        inputphy = AML_MEM_GetPhyAddr(hShmInput);

        // Allocate output buffer.
        hShmOutput = AML_MEM_Allocate(kOutputBufferSize);
        outputBuf = (int16_t*)AML_MEM_GetVirtAddr(hShmOutput);
        outputphy = AML_MEM_GetPhyAddr(hShmOutput);
        printf("Init in vir:%p phy:%p,  out vir:%p phy:%p\n",
        inputBuf, inputphy, outputBuf, outputphy);
    } else {
        // Allocate input buffer.
        inputBuf = (uint8_t*)malloc(kInputBufferSize);

        // Allocate output buffer.
        outputBuf = (int16_t*)malloc(kOutputBufferSize);
        printf("Init input %p, output buffer %p\n", inputBuf, outputBuf);
    }

    // Decode loop.
    int retVal = EXIT_SUCCESS;
    while (1) {
    // Read input from the file.
    uint32_t bytesRead;
    success = mp3Reader.getFrame(inputBuf, &bytesRead);
    if (!success) {
        printf("EOF\n");
        break;
    }

    // Set the input config.
    config.inputBufferCurrentLength = bytesRead;
    config.inputBufferMaxLength = 0;
    config.inputBufferUsedLength = 0;
    if (bUserAllocShm) {
        config.pInputBuffer = (uint8_t*)inputphy;
        config.pOutputBuffer = (int16_t*)outputphy;
    } else {
        config.pInputBuffer = (uint8_t*)inputBuf;
        config.pOutputBuffer = (int16_t*)outputBuf;
    }
    config.outputFrameSize = kOutputBufferSize / sizeof(int16_t);
    /*INFO("inputBufferCurrentLength:0x%x, inputBufferMaxLength:0x%x, inputBufferUsedLength:0x%x,"
    "pInputBuffer:0x%lx, pOutputBuffer:0x%lx, outputFrameSize:0x%x\n",
    config.inputBufferCurrentLength, config.inputBufferMaxLength, config.inputBufferUsedLength,
    config.pInputBuffer, config.pOutputBuffer, config.outputFrameSize);*/
    /*INFO("\n=================================\n");
    mp3_show_hex((char*)config.pInputBuffer, bytesRead);
    INFO("\n=================================\n");*/
    ERROR_CODE decoderErr;
    //printf("config.outputFrameSize:%d\n", config.outputFrameSize);
    if (bUserAllocShm) {
        AML_MEM_Clean(inputphy, bytesRead);
        decoderErr = AmlACodecExec_UserAllocIoShm_Mp3Dec(hdlmp3, &config);
        AML_MEM_Invalidate(outputphy, config.outputFrameSize*sizeof(int16_t));
    } else
        decoderErr = AmlACodecExec_Mp3Dec(hdlmp3, &config);

    if (decoderErr != NO_DECODING_ERROR) {
        fprintf(stderr, "Decoder encountered error:0x%x\n", decoderErr);
        retVal = EXIT_FAILURE;
        break;
    }
    /*INFO("\n*************arm:%p**********************\n", outputBuf);
    mp3_show_hex((char*)outputBuf, config.outputFrameSize*sizeof(int16_t));
    INFO("\n***********************************\n");*/
    //INFO("config.outputFrameSize:%d\n", config.outputFrameSize);
    sf_writef_short(handle, outputBuf,
            config.outputFrameSize / sfInfo.channels);
    }
    printf("mp3 decoder done\n");

    // Close input reader and output writer.
    mp3Reader.close();
    printf("reader close\n");
    sf_close(handle);
    printf("write clonse\n");

    // Free allocated memory.
    if (bUserAllocShm) {
        AML_MEM_Free((AML_MEM_HANDLE)hShmInput);
        AML_MEM_Free((AML_MEM_HANDLE)hShmOutput);
    } else {
        free(inputBuf);
        free(outputBuf);
    }
    AmlACodecDeInit_Mp3Dec(hdlmp3);
    return retVal;
}

int aac_offload_dec(int argc, char* argv[]) {
    SNDFILE *handle = NULL;
    FILE* pcmfile = NULL;
    int bUserAllocShm = 0;
    tAmlAacDecHdl hdlAac = 0;
    AML_MEM_HANDLE hShmInput =0;
    AML_MEM_HANDLE hShmOutput = 0;
    uint8_t *inputBuf = 0;
    int16_t *outputBuf = 0;
    void* inputphy = 0;
    void* outputphy = 0;

    // Initialize the decoder.
    tAmlAacInitCtx config;
    config.transportFmt = (uint32_t)atoi(argv[0]);
    config.nrOfLayers = 1;
    hdlAac = AmlACodecInit_AacDec(&config);
    if (!hdlAac) {
        printf("Failed to allocate aac handler\n");
        return -1;
    }
    printf("Init aacdec hdl=%p\n", hdlAac);

    // Open the input file.
    FILE* aacfile = fopen(argv[1], "rb");
    if (!aacfile) {
        fprintf(stderr, "Encountered error reading %s\n", argv[1]);
        AmlACodecDeInit_AacDec(hdlAac);
        return EXIT_FAILURE;
    }
    printf("Open aac file\n");

    //AmlACodecSetParam(hdlAac, AAC_DRC_REFERENCE_LEVEL, 54);
    //AmlACodecSetParam(hdlAac, AAC_DRC_ATTENUATION_FACTOR, 32);
    //AmlACodecSetParam(hdlAac, AAC_PCM_LIMITER_ENABLE, 1);


    if (argc == 4)
        bUserAllocShm = atoi(argv[3]);
    if (bUserAllocShm) {
        // Allocate input buffer.
        hShmInput = AML_MEM_Allocate(AAC_INPUT_SIZE);
        printf("hShmInput:%p\n", hShmInput);
        inputBuf = (uint8_t*)AML_MEM_GetVirtAddr(hShmInput);
        inputphy = AML_MEM_GetPhyAddr(hShmInput);

        // Allocate output buffer.
        hShmOutput = AML_MEM_Allocate(PCM_OUTPUT_SIZE);
        outputBuf = (int16_t*)AML_MEM_GetVirtAddr(hShmOutput);
        printf("hShmOutput:%p\n", hShmOutput);
        outputphy = AML_MEM_GetPhyAddr(hShmOutput);
        printf("===Init in vir:%p phy:%p,  out vir:%p phy:%p====\n",
        inputBuf, inputphy, outputBuf, outputphy);
        if (!hShmOutput || !hShmInput){
            fprintf(stderr, "Encountered memory allocation issue\n");
            fclose(aacfile);
            AmlACodecDeInit_AacDec(hdlAac);
            return EXIT_FAILURE;
        }
    } else {
        // Allocate input buffer.
        inputBuf = (uint8_t*)malloc(AAC_INPUT_SIZE);

        // Allocate output buffer.
        outputBuf = (int16_t*)malloc(PCM_OUTPUT_SIZE);
        printf("Init input %p, output buffer %p\n", inputBuf, outputBuf);
        if (!inputBuf || !outputBuf) {
            fprintf(stderr, "Encountered memory allocation issue\n");
            fclose(aacfile);
            AmlACodecDeInit_AacDec(hdlAac);
            return EXIT_FAILURE;
        }
    }

    // Decode loop.
    int retVal = EXIT_SUCCESS;
    while (1) {
    // Read input from the file.
    uint32_t bytesRead = AAC_INPUT_SIZE;
    uint32_t aac_input_size, pcm_out_size, aac_input_left;
    tAmlAacOutputCtx out_ctx;
    bytesRead = fread(inputBuf, 1, bytesRead, aacfile);
    if (!bytesRead) {
        printf("EOF\n");
        break;
    }

    aac_input_size = bytesRead;
    pcm_out_size = PCM_OUTPUT_SIZE;
    AAC_DECODER_ERROR decoderErr;
    if (bUserAllocShm) {
        AML_MEM_Clean(inputphy, bytesRead);
        decoderErr = AmlACodecExec_UserAllocIoShm_AacDec(hdlAac, inputphy, aac_input_size,
                            outputphy, &pcm_out_size,
                            &aac_input_left, &out_ctx);
        //printf("aac_input_left:%dn", aac_input_left);
        AML_MEM_Invalidate(outputphy, pcm_out_size);
        fseek(aacfile, -((long)aac_input_left), SEEK_CUR);
    } else {
        decoderErr = AmlACodecExec_AacDec(hdlAac, inputBuf, aac_input_size,
                            outputBuf, &pcm_out_size,
                            &aac_input_left, &out_ctx);
        fseek(aacfile, -((long)aac_input_left), SEEK_CUR);
    }
    if (decoderErr != AAC_DEC_OK) {
        fprintf(stderr, "Decoder encountered error:0x%x\n", decoderErr);
        retVal = EXIT_FAILURE;
        break;
    }

    if (pcmfile == NULL) {
        // Open the output file.
        printf("=======Decode first aac frame:======\n");
        printf("channels: %d, sampleRate:%d, frameSize:%d\n"
        "channal mask - front:%d side:%d back:%d lfe:%d top:%d\n",
        out_ctx.channelNum, out_ctx.sampleRate, out_ctx.frameSize,
        out_ctx.chmask[ACT_FRONT], out_ctx.chmask[ACT_SIDE],
        out_ctx.chmask[ACT_BACK],  out_ctx.chmask[ACT_LFE],
        out_ctx.chmask[ACT_FRONT_TOP] + out_ctx.chmask[ACT_SIDE_TOP] +
        out_ctx.chmask[ACT_BACK_TOP]  + out_ctx.chmask[ACT_TOP]);
        printf("Init outfile=%s\n", argv[1]);
        SF_INFO sfInfo;
        memset(&sfInfo, 0, sizeof(SF_INFO));
        sfInfo.channels = out_ctx.channelNum;
        sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;//FDK limited to 16bit per samples
        sfInfo.samplerate = out_ctx.sampleRate;
        //handle = sf_open(argv[1], SFM_WRITE, &sfInfo);
        pcmfile = fopen(argv[2], "w+b");
        if (pcmfile == NULL) {
            fprintf(stderr, "Encountered error writing %s\n", argv[1]);
            goto tab_end;
        }
    }

    /*INFO("\n*************arm:%p**********************\n", outputBuf);
    mp3_show_hex((char*)outputBuf, config.outputFrameSize*sizeof(int16_t));
    INFO("\n***********************************\n");*/
    //INFO("config.outputFrameSize:%d\n", config.outputFrameSize);
    //if (handle)
    //  sf_writef_short(handle, outputBuf,pcm_out_size/SF_FORMAT_PCM_16);
    if (pcmfile)
        fwrite(outputBuf, 1, out_ctx.frameSize*out_ctx.channelNum*SF_FORMAT_PCM_16, pcmfile);
    }

    // Close input reader and output writer.
    printf("aac decoder done\n");

    tab_end:
    if (aacfile) {
        fclose(aacfile);
        printf("aac file close\n");
    }
    if (handle) {
        sf_close(handle);
        printf("write close\n");
    }
    fclose(pcmfile);
    // Free allocated memory.
    if (bUserAllocShm) {
        if (hShmInput)
            AML_MEM_Free((AML_MEM_HANDLE)hShmInput);
        if (hShmOutput)
            AML_MEM_Free((AML_MEM_HANDLE)hShmOutput);
    } else {
        if (inputBuf)
            free(inputBuf);
        if (outputBuf)
            free(outputBuf);
    }
    if (hdlAac)
        AmlACodecDeInit_AacDec(hdlAac);
    return retVal;
}

