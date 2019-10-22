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
 * hifi4 rpc client api sample codes
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
#include "rpc_client_pcm.h"
#include "rpc_client_vsp.h"
#include "aml_wakeup_api.h"
#include "aml_audio_util.h"

#define UNUSED(x) (void)(x)

#define MSECS_PER_SEC (1000L)
#define NSECS_PER_MSEC (1000000L)

void aprofiler_get_cur_timestamp(struct timespec* ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
    return;
}

uint32_t aprofiler_msec_duration(struct timespec* tsEnd, struct timespec* tsStart)
{
    uint32_t uEndMSec = (uint32_t)(tsEnd->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsEnd->tv_nsec/NSECS_PER_MSEC);
    uint32_t uStartMSec = (uint32_t)(tsStart->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsStart->tv_nsec/NSECS_PER_MSEC);
	//printf("uEndMSec:%d, uStartMSec:%d\n", uEndMSec, uStartMSec);
	return (uEndMSec - uStartMSec);
}


#define TIC              \
    struct timespec bgn, end; \
    aprofiler_get_cur_timestamp(&bgn)

#define TOC                            \
    aprofiler_get_cur_timestamp(&end); \
    uint32_t ms = aprofiler_msec_duration(&end, &bgn)

#if 1
uint32_t audio_play_data[] = {
//#include "sinewav_48k_24bits_stereo_l1k_r2k_1s.in"
#include "pyghlkn_48k_24bits_stereo_10s.in"
};
#else
uint8_t audio_play_data[] = {
#include "dtmf_48k_32bits_stereo.in"
};
#endif
uint32_t audio_play_data_len = sizeof(audio_play_data);

using namespace std;

static int pcm_play_buildin()
{
    rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
    pconfig->channels = 2;
    pconfig->rate = 48000;
    pconfig->format = PCM_FORMAT_S32_LE;
    pconfig->period_size = 1024;
    pconfig->period_count = 2;
    pconfig->start_threshold = 1024;
    pconfig->silence_threshold = 1024*2;
    pconfig->stop_threshold = 1024*2;
    tAmlPcmhdl p = pcm_client_open(0, DEVICE_TDMOUT_B, PCM_OUT, pconfig);
    AML_MEM_HANDLE hShmBuf;

    uint8_t *play_data = (uint8_t *)audio_play_data;
    int in_fr = pcm_client_bytes_to_frame(p, audio_play_data_len);
    int i, fr = 0;
    const int ms = 36;
    const int oneshot = 48 * ms; // 1728 samples
    uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    for (i = 0; i + oneshot <= in_fr; i += fr) {
        memcpy(buf, play_data + pcm_client_frame_to_bytes(p, i), size);
        AML_MEM_Clean(phybuf, size);
        fr = pcm_client_writei(p, phybuf, oneshot);
        //printf("%dms pcm_write i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //  ms, i, p, buf, oneshot, fr);
    }
    pcm_client_close(p);
    AML_MEM_Free(hShmBuf);
    free(pconfig);
    return 0;
}

static int pcm_play_test(int argc, char* argv[])
{
    rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
    pconfig->channels = 2;
    pconfig->rate = 48000;
    pconfig->format = PCM_FORMAT_S32_LE;
    pconfig->period_size = 1024;
    pconfig->period_count = 2;
    pconfig->start_threshold = 1024;
    pconfig->silence_threshold = 1024*2;
    pconfig->stop_threshold = 1024*2;
    tAmlPcmhdl p = pcm_client_open(0, DEVICE_TDMOUT_B, PCM_OUT, pconfig);
    AML_MEM_HANDLE hShmBuf;

    if (argc != 1) {
        printf("Invalid paremeter number, argc=%d\n", argc);
        return -1;
    }

    FILE *fileplay = fopen(argv[0], "rb");
    if (fileplay == NULL) {
        printf("failed to open played pcm file\n");
        return -1;
    }
    const int ms = 36;
    const int oneshot = 48 * ms; // 1728 samples
    uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    while ((size = fread(buf, 1, size, fileplay))) {
        AML_MEM_Clean(phybuf, size);
        pcm_client_writei(p, phybuf, oneshot);
        //printf("%dms pcm_write pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //   ms, p, buf, oneshot, fr);
    }
    pcm_client_close(p);
    AML_MEM_Free(hShmBuf);
    free(pconfig);
    fclose(fileplay);
    return 0;
}


#define PCM_CAPTURE_SAMPLES (48000*20)
static int pcm_capture_test(int argc, char* argv[])
{
    enum ALSA_DEVICE_IN device = DEVICE_TDMIN_B;
    if (argc >= 2)
        device = (enum ALSA_DEVICE_IN)atoi(argv[1]);
    rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
    pconfig->channels = (device == DEVICE_LOOPBACK)?4:2;
    pconfig->rate = 48000;
    pconfig->format = PCM_FORMAT_S32_LE;
    pconfig->period_size = 1024;
    pconfig->period_count = 4;
    pconfig->start_threshold = 1024;
    pconfig->silence_threshold = 1024*2;
    pconfig->stop_threshold = 1024*2;
    tAmlPcmhdl p = pcm_client_open(0, device, PCM_IN, pconfig);
    AML_MEM_HANDLE hShmBuf;

    FILE *filecap = fopen(argv[0], "w+b");
    if (filecap == NULL) {
        printf("failed to open captured pcm file\n");
        return -1;
    }

    int in_fr = PCM_CAPTURE_SAMPLES;
    int i, fr = 0;
    const int ms = 36;
    const int oneshot = 48 * ms; // 1728 samples
    uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    for (i = 0; i + oneshot <= in_fr; i += fr) {
        fr = pcm_client_readi(p, phybuf, oneshot);
        AML_MEM_Invalidate(phybuf, size);
        fwrite(buf, sizeof(char), size, filecap);
        //printf("%dms pcm_read i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //  ms, i, p, buf, oneshot, fr);
    }
    pcm_client_close(p);
    AML_MEM_Free(hShmBuf);
    free(pconfig);
    fclose(filecap);
    return 0;
}

static int mp3_offload_dec(int argc, char* argv[]) {
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

static int aac_offload_dec(int argc, char* argv[]) {
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
    config.transportFmt = TT_MP4_ADTS;
    config.nrOfLayers = 1;
    hdlAac = AmlACodecInit_AacDec(&config);
    printf("Init aacdec hdl=%p\n", hdlAac);

    // Open the input file.
    FILE* aacfile = fopen(argv[0], "rb");
    if (!aacfile) {
        fprintf(stderr, "Encountered error reading %s\n", argv[0]);
        AmlACodecDeInit_AacDec(hdlAac);
        return EXIT_FAILURE;
    }
    printf("Open aac file\n");

    //AmlACodecSetParam(hdlAac, AAC_DRC_REFERENCE_LEVEL, 54);
    //AmlACodecSetParam(hdlAac, AAC_DRC_ATTENUATION_FACTOR, 32);
    //AmlACodecSetParam(hdlAac, AAC_PCM_LIMITER_ENABLE, 1);


    if (argc == 3)
        bUserAllocShm = atoi(argv[2]);
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
        pcmfile = fopen(argv[1], "w+b");
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
static int offload_vsp_rsp(int argc, char* argv[]) {
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

#define VOICE_CHUNK_LEN_MS 20
#define AWE_SAMPLE_RATE 16000
#define AWE_SAMPLE_BYTE 2
#define VOICE_CHUNK_LEN_BYTE (AWE_SAMPLE_BYTE*AWE_SAMPLE_RATE*VOICE_CHUNK_LEN_MS/1000)
static uint32_t uFeedChunk = 0;
static uint32_t uRecvChunk = 0;
static uint32_t uTotalBytesRead = 0;
static uint32_t uTotalBytesWrite = 0;

void aml_wake_engine_asr_data_handler(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data)
{
    UNUSED(awe);
    FILE* fout = (FILE*)user_data;
    if (AWE_DATA_TYPE_ASR == type) {
        fwrite(out, 1, size, fout);
        uRecvChunk++;
        uTotalBytesWrite += size;
    }
}

void aml_wake_engine_voip_data_handler(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data)
{
    UNUSED(awe);
    FILE* fout = (FILE*)user_data;
    if (AWE_DATA_TYPE_VOIP == type) {
        fwrite(out, 1, size, fout);
    }
}

void aml_wake_engine_event_handler(AWE *awe, const AWE_EVENT_TYPE type, int32_t code,
                                             const void *payload, void *user_data)
{
    UNUSED(awe);
    UNUSED(code);
    UNUSED(payload);
    UNUSED(user_data);
    if (type == AWE_EVENT_TYPE_WAKE)
        printf("wake word detected !!!! \n");
}

static AWE *gAwe = NULL;
void awe_test_sighandler(int signum)
{
    UNUSED(signum);
    if (gAwe)
        AML_AWE_Close(gAwe);
    if (gAwe)
        AML_AWE_Destroy(gAwe);
    gAwe = NULL;
    uFeedChunk = 0;
}

int aml_wake_engine_unit_test(int argc, char* argv[]) {
    uFeedChunk = 0;
    uRecvChunk = 0;
    uTotalBytesRead = 0;
    uTotalBytesWrite = 0;
    int syncMode = 1;
    AWE_PARA awe_para;
    int ret = 0;
    uint32_t isWakeUp = 0;
    AWE_RET awe_ret = AWE_RET_OK;
    int32_t inLen = 0, outLen = 0;
    void *in[4], *out[2];

    AML_MEM_HANDLE hMic0Buf = 0;
    void *vir_mic0_buf = NULL;
    void *phy_mic0_buf = NULL;

    AML_MEM_HANDLE hMic1Buf = 0;
    void *vir_mic1_buf = NULL;
    void *phy_mic1_buf = NULL;

    AML_MEM_HANDLE hRef0Buf = 0;
    void *vir_ref0_buf = NULL;
    void *phy_ref0_buf = NULL;

    void *vir_interleave_buf = NULL;

    AML_MEM_HANDLE hOutBuf0 = 0;
    void *vir_out0_buf = NULL;
    void *phy_out0_buf = NULL;

    AML_MEM_HANDLE hOutBuf1 = 0;
    void *vir_out1_buf = NULL;
    void *phy_out1_buf = NULL;

    signal(SIGINT, &awe_test_sighandler);
    if (argc == 6)
        syncMode = atoi(argv[5]);

    FILE *fmic0 = fopen(argv[0], "rb");
    FILE *fmic1 = fopen(argv[1], "rb");
    FILE *fref0 = fopen(argv[2], "rb");
    FILE *fout0 = fopen(argv[3], "w+b");
    FILE *fout1 = fopen(argv[4], "w+b");
    if ( !fmic0 || !fmic1|| !fref0 || !fout0 || !fout1) {
        printf("Can not open io file:%p %p %p %p %p\n",
        fmic0, fmic1, fref0, fout0, fout1);
        ret = -1;
        goto end_tab;
    }

    if (syncMode == 1) {
        vir_interleave_buf = malloc(3*VOICE_CHUNK_LEN_BYTE);
        if (!vir_interleave_buf) {
            printf("Can not allocate interleave buffer:%p\n", vir_interleave_buf);
            ret = -1;
            goto end_tab;
        }
        vir_mic0_buf = malloc(VOICE_CHUNK_LEN_BYTE);
        vir_mic1_buf = malloc(VOICE_CHUNK_LEN_BYTE);
        vir_ref0_buf = malloc(VOICE_CHUNK_LEN_BYTE);
    } else {
        hMic0Buf = AML_MEM_Allocate(VOICE_CHUNK_LEN_BYTE);
        vir_mic0_buf = AML_MEM_GetVirtAddr(hMic0Buf);
        phy_mic0_buf = AML_MEM_GetPhyAddr(hMic0Buf);

        hMic1Buf = AML_MEM_Allocate(VOICE_CHUNK_LEN_BYTE);
        vir_mic1_buf = AML_MEM_GetVirtAddr(hMic1Buf);
        phy_mic1_buf = AML_MEM_GetPhyAddr(hMic1Buf);

        hRef0Buf = AML_MEM_Allocate(VOICE_CHUNK_LEN_BYTE);
        vir_ref0_buf = AML_MEM_GetVirtAddr(hRef0Buf);
        phy_ref0_buf = AML_MEM_GetPhyAddr(hRef0Buf);
        if (!hMic0Buf|| !hMic1Buf || !hRef0Buf) {
            printf("Can not allocate none interleave buffer:%p %p %p\n",
            hMic0Buf, hMic1Buf, hRef0Buf);
            ret = -1;
            goto end_tab;
        } else {
            printf("mic0buf:%p, mic1buf:%p, ref0buf:%p\n",
            phy_mic0_buf, phy_mic1_buf, phy_ref0_buf);
        }
    }

    hOutBuf0 = AML_MEM_Allocate(2048);
    vir_out0_buf = AML_MEM_GetVirtAddr(hOutBuf0);
    phy_out0_buf = AML_MEM_GetPhyAddr(hOutBuf0);

    hOutBuf1 = AML_MEM_Allocate(2048);
    vir_out1_buf = AML_MEM_GetVirtAddr(hOutBuf1);
    phy_out1_buf = AML_MEM_GetPhyAddr(hOutBuf1);


    if (!hOutBuf0 || !hOutBuf1) {
        printf("Can not allocate output buffer:%p, %p\n", hOutBuf0, hOutBuf1);
        ret = -1;
        goto end_tab;
    } else {
        printf("outbuf:%p,%p\n", phy_out0_buf, phy_out1_buf);
    }

    awe_ret = AML_AWE_Create(&gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Can not create Hifi AWE service\n");
        ret = -1;
        goto end_tab;
    }

    if (syncMode == 1) {
        AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_ASR, aml_wake_engine_asr_data_handler,(void *)fout0);
        AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_VOIP, aml_wake_engine_voip_data_handler,(void *)fout1);
        AML_AWE_AddEventHandler(gAwe, AWE_EVENT_TYPE_WAKE, aml_wake_engine_event_handler, NULL);
    }

    awe_para.inputMode = AWE_USER_INPUT_MODE;
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_INPUT_MODE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set input mode fail:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_RATES, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Get supported samperate fail:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampRate = awe_para.supportSampRates[0];
    printf("awe_para.inSampRate:%d\n", awe_para.inSampRate);
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_RATE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set samperate fail:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to get sample bits:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampBits = awe_para.supportSampBits[0];
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to set sample bits:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_Open(gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to open AWE:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }
    printf("wake test start! !\n");
    uint32_t i;
    while (1) {
        uint32_t nbyteRead = VOICE_CHUNK_LEN_BYTE;
        uint32_t nbyteMic0 = 0;
        uint32_t nbyteMic1 = 0;
        uint32_t nbyteRef0 = 0;
        nbyteMic0 = fread(vir_mic0_buf, 1, nbyteRead, fmic0);
        nbyteMic1 = fread(vir_mic1_buf, 1, nbyteRead, fmic1);
        nbyteRef0 = fread(vir_ref0_buf, 1, nbyteRead, fref0);
        if (nbyteMic0 < nbyteRead || nbyteMic1 < nbyteRead || nbyteRef0 < nbyteRead) {
            printf("EOF\n");
            break;
        }

        if (syncMode == 0) {
            AML_MEM_Clean(phy_mic0_buf, nbyteRead);
            AML_MEM_Clean(phy_mic1_buf, nbyteRead);
            AML_MEM_Clean(phy_ref0_buf, nbyteRead);

            in[0] = hMic0Buf;
            in[1] = hMic1Buf;
            in[2] = hRef0Buf;
            inLen = nbyteRead;
            out[0] = hOutBuf0;
            out[1] = hOutBuf1;
            outLen = 2048;
            AML_AWE_Process(gAwe, in, &inLen, out, &outLen, &isWakeUp);
            if (isWakeUp) {
                printf("wake word detected ! \n");
            }
            if (!inLen) {
                fseek(fmic0, -((long)inLen), SEEK_CUR);
                fseek(fmic1, -((long)inLen), SEEK_CUR);
                fseek(fref0, -((long)inLen), SEEK_CUR);
            }
            AML_MEM_Invalidate(phy_out0_buf, outLen);
            AML_MEM_Invalidate(phy_out1_buf, outLen);
            fwrite(vir_out0_buf, 1, outLen, fout0);
            fwrite(vir_out1_buf, 1, outLen, fout1);
            uTotalBytesWrite += outLen;
            uTotalBytesRead += nbyteRead;
        } else if (syncMode == 1) {
            /*interleave mic0,mic1,ref0*/
            short* pMic0 = (short*)vir_mic0_buf;
            short* pMic1 = (short*)vir_mic1_buf;
            short* pRef0 = (short*)vir_ref0_buf;
            short* pInterleave = (short*)vir_interleave_buf;
            for (i = 0; i < (nbyteRead/2); i++) {
                *pInterleave = *pMic0++;
                pInterleave++;
                *pInterleave = *pMic1++;
                pInterleave++;
                *pInterleave = *pRef0++;
                pInterleave++;
            }
            ret = AML_AWE_PushBuf(gAwe, (const char*)vir_interleave_buf, nbyteRead*3);
            if (AWE_RET_ERR_NO_MEM == ret) {
                fseek(fmic0, -((long)nbyteRead), SEEK_CUR);
                fseek(fmic1, -((long)nbyteRead), SEEK_CUR);
                fseek(fref0, -((long)nbyteRead), SEEK_CUR);
                usleep(500);
            }
            else if (ret != AWE_RET_OK) {
                printf("Unknow error when execute AML_AWE_PushBuf, ret=%d\n", ret);
                break;
            } else {
                uFeedChunk++;
                uTotalBytesRead += nbyteRead;
            }
        } else {
            printf("Invalide sync mode:%d\n", syncMode);
            break;
        }
    }

    end_tab:
    while(uRecvChunk < uFeedChunk) {
        printf("uRecvChunk:%d, uFeedChunk:%d\n", uRecvChunk, uFeedChunk);
        usleep(5000);
    }
    printf("Read: %d Kbytes, Write %d Kbytes\n", uTotalBytesRead/1024, uTotalBytesWrite/1024);
    if (gAwe)
        AML_AWE_Close(gAwe);
    if (gAwe)
        AML_AWE_Destroy(gAwe);
    if (hOutBuf0)
        AML_MEM_Free(hOutBuf0);
    if (hOutBuf1)
        AML_MEM_Free(hOutBuf1);
    if (syncMode == 0) {
        if (hMic0Buf)
            AML_MEM_Free(hMic0Buf);
        if (hMic1Buf)
            AML_MEM_Free(hMic1Buf);
        if (hRef0Buf)
            AML_MEM_Free(hRef0Buf);
    }

    if (vir_interleave_buf) {
        free(vir_interleave_buf);
        free(vir_mic0_buf);
        free(vir_mic1_buf);
        free(vir_ref0_buf);
    }

    if (fmic0)
        fclose(fmic0);
    if (fmic1)
        fclose(fmic1);
    if (fref0)
        fclose(fref0);
    if (fout0)
        fclose(fout0);
    if (fout1)
        fclose(fout1);
    return ret;
}

#define VOIP_DATA_FIFO "/tmp/voip_data_fifo"
#define VOIP_CMD_FIFO "/tmp/voip_cmd_fifo"
#define VOIP_CMD_DATA_INVALID 0
#define VOIP_CMD_DATA_EOF 0xffffffff
#define VOIP_CMD_DATA_AVAIL 1

void aml_wake_engine_voip_data_receiver(char* strVoip)
{
    FILE* fout_voip = NULL;
    int fifo_reader_fd = -1;
    int fifo_cmdrev_fd = -1;
    uint32_t nread = 0;
    uint32_t cmd = 0;
    int nReadFail = 0;
    void* pReadBuf = NULL;
    void* pWriteBuf = NULL;
    void* hsrc = NULL;

    fifo_reader_fd = open(VOIP_DATA_FIFO ,O_RDONLY|O_NONBLOCK,0);
    if(fifo_reader_fd < 0)
    {
        printf("Voip data fifo open failed in receiver side\n");
        goto receiver_end;
    }

    fifo_cmdrev_fd = open(VOIP_CMD_FIFO ,O_RDONLY,0);
    if(fifo_cmdrev_fd < 0)
    {
        printf("Voip command fifo open failed in receiver side\n");
        goto receiver_end;
    }

    fout_voip = fopen(strVoip, "w+b");
    if (fout_voip == NULL) {
        printf("Voip dump file open failed in receiver side\n");
        goto receiver_end;
    }
    //16000:8000 = 2:1
    hsrc = AML_SRCS16LE_Init(2, 1, 1);
    pReadBuf = malloc(VOICE_CHUNK_LEN_BYTE);
    pWriteBuf = malloc(VOICE_CHUNK_LEN_BYTE/2);
    while(1) {
        read(fifo_cmdrev_fd, &cmd, sizeof(cmd));
        if (cmd == VOIP_CMD_DATA_EOF) {
            printf("Exit voip receiver process\n");
            break;
        } else if (cmd == VOIP_CMD_DATA_AVAIL) {
            nread = read(fifo_reader_fd, pReadBuf, VOICE_CHUNK_LEN_BYTE);
            if (nread != VOICE_CHUNK_LEN_BYTE) {
                nReadFail++;
                if (nReadFail > 50) {
                    printf("Too much data fifo read error, exit voip receiver process\n");
                    break;
                }
                continue;
            }
            nReadFail = 0;
            AML_SRCS16LE_Exec(hsrc, (int16_t*)pWriteBuf, VOICE_CHUNK_LEN_BYTE/(2*sizeof(int16_t)), (int16_t*)pReadBuf, VOICE_CHUNK_LEN_BYTE/sizeof(int16_t));
            fwrite(pWriteBuf, 1, VOICE_CHUNK_LEN_BYTE/2, fout_voip);
            usleep(20*1000);
        } else {
            printf("Invalid voip command, exit voip receiver process\n");
            break;
        }
    }
    AML_SRCS16LE_DeInit(hsrc);
    free(pWriteBuf);
    free(pReadBuf);

receiver_end:
    if (fout_voip)
        fclose(fout_voip);
    if (fifo_reader_fd > 0)
        close(fifo_reader_fd);
}

void aml_wake_engine_voip_data_transfer(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data)
{
    UNUSED(awe);
    int* pfifo = (int*)user_data;
    int fifo_writer_fd = pfifo[0];
    int fifo_cmdsend_fd = pfifo[1];
    if (AWE_DATA_TYPE_VOIP == type) {
        uint32_t cmd = VOIP_CMD_DATA_AVAIL;
        write(fifo_writer_fd, out, size);
        write(fifo_cmdsend_fd, &cmd, sizeof(cmd));
    }
}

int aml_wake_engine_dspin_test(int argc, char* argv[]) {
    UNUSED(argc);
    AWE_PARA awe_para;
    int ret = 0;
    AWE_RET awe_ret = AWE_RET_OK;
    pid_t pid = -1;
    FILE *fout_asr = NULL;
    int fifo_writer_fd = -1;
    int fifo_cmdsend_fd = -1;
    int fifo_array[2] = {0};
    uint32_t cmd = VOIP_CMD_DATA_INVALID;

    if((mkfifo(VOIP_DATA_FIFO ,O_CREAT|O_EXCL|O_RDWR)<0)&&(errno!=EEXIST)) {
        printf("Can not create voip data fifo\n");
        goto end_tab;
    }

    if((mkfifo(VOIP_CMD_FIFO ,O_CREAT|O_EXCL|O_RDWR)<0)&&(errno!=EEXIST)) {
        printf("Can not create voip command fifo\n");
        goto end_tab;
    }

    /* fork a child process */
    pid = fork();
    if (pid < 0) { /* error occurred */\
        printf("Fork Failed");
        return 1;
    }
    else if (pid == 0) { /* child process */
        printf("Start voip receiver process, pid=%d\n", getpid());
        sleep(1);
        aml_wake_engine_voip_data_receiver(argv[1]);
        return 0;
    }
    else { /* parent process */
        printf("Continue parent process\n");
    }

    signal(SIGINT, &awe_test_sighandler);
    fifo_writer_fd = open(VOIP_DATA_FIFO, O_RDWR | O_NONBLOCK, 0);
    if (fifo_writer_fd < 0) {
        printf("Can not open voip data fifo writer fd:%d\n", fifo_writer_fd);
        goto end_tab;
    }

    fifo_cmdsend_fd = open(VOIP_CMD_FIFO, O_RDWR | O_NONBLOCK, 0);
    if (fifo_cmdsend_fd < 0) {
        printf("Can not open voip command fifo writer fd:%d\n", fifo_writer_fd);
        goto end_tab;
    }
    fifo_array[0] = fifo_writer_fd;
    fifo_array[1] = fifo_cmdsend_fd;

    fout_asr = fopen(argv[0], "w+b");
    if (!fout_asr) {
        printf("Can not open asr output file:%p\n", fout_asr);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_Create(&gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Can not create Hifi AWE service\n");
        ret = -1;
        goto end_tab;
    }

    AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_ASR, aml_wake_engine_asr_data_handler,(void *)fout_asr);
    AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_VOIP, aml_wake_engine_voip_data_transfer,(void *)fifo_array);
    AML_AWE_AddEventHandler(gAwe, AWE_EVENT_TYPE_WAKE, aml_wake_engine_event_handler, NULL);

    awe_para.inputMode = AWE_DSP_INPUT_MODE;
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_INPUT_MODE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set input mode fail\n");
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_RATES, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Get supported samperate fail\n");
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampRate = awe_para.supportSampRates[0];
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_RATE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set samperate fail\n");
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to get sample bits\n");
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampBits = awe_para.supportSampBits[0];
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to set sample bits\n");
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_Open(gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to open AWE\n");
        ret = -1;
        goto end_tab;
    }
    printf("wake test start in dsp input mode\n");
    char user_cmd[16];
    while (1) {
        printf("Command Guide:\n");
        printf("suspend - suspend and resume from voice wake up\n");
        printf("exit - quit voice capture\n");
        scanf("%s", user_cmd);
        if(!strcmp(user_cmd, "suspend")) {
            system("amixer cset numid=7 1");
            system("amixer cset numid=21 1");
            system("arecord -Dhw:0,2 -c 2 -r 16000 -f S32_LE /tmp/aa.wav &");
            sleep(1);
            printf("Start awe freeRun mode\n");
            awe_para.freeRunMode = 1;
            awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_FREE_RUN_MODE, &awe_para);
            system("echo "" > /sys/kernel/config/usb_gadget/amlogic/UDC");
            system("echo mem > /sys/power/state");
            awe_para.freeRunMode = 0;
            awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_FREE_RUN_MODE, &awe_para);
            printf("Stop awe freeRun mode\n");
        } else if(!strcmp(user_cmd, "exit")) {
            break;
        } else {
            printf("Invalid command!\n");
        }
    }

    cmd = VOIP_CMD_DATA_EOF;
    write(fifo_cmdsend_fd, &cmd, sizeof(cmd));

end_tab:
    if (gAwe)
        AML_AWE_Close(gAwe);
    if (gAwe)
        AML_AWE_Destroy(gAwe);

    if (fout_asr)
        fclose(fout_asr);

    if (fifo_writer_fd > 0)
        close(fifo_writer_fd);

    unlink(VOIP_CMD_FIFO);
    unlink(VOIP_DATA_FIFO);
    return ret;
}


#define IPC_UNIT_TEST_REPEAT 50
static int ipc_uint_tset(void) {
    unsigned int i;
    int num_repeat = IPC_UNIT_TEST_REPEAT;
    const uint8_t send_samples[16] = {
        0x1, 0xf, 0x2, 0xe, 0x3, 0xd, 0x4, 0xc, 0x5, 0xb, 0x6, 0xa, 0x7, 0x9, 0x0, 0x8
    };
    const uint8_t recv_samples[16] = {
        0x11, 0xf1, 0x21, 0x1e, 0x13, 0x1d, 0x14, 0xc2, 0x52, 0xb2, 0x62, 0x2a, 0x27, 0x29, 0x20, 0x38
    };
    char ipc_data[16];
    int arpchdl = xAudio_Ipc_init();
    while(num_repeat--) {
        memcpy(ipc_data, send_samples, sizeof(ipc_data));
        xAIPC(arpchdl, MBX_CMD_IPC_TEST, (void*)ipc_data, sizeof(ipc_data));
        for (i = 0; i < sizeof(recv_samples); i++) {
            if(recv_samples[i] != ipc_data[i])
            break;
        }
        if (i < sizeof(recv_samples)) {
            printf("arm ack: ipc unit test fail:%d, ipc data:\n", IPC_UNIT_TEST_REPEAT - num_repeat);
            for(i = 0; i < sizeof(recv_samples); i++)
                printf("0x%x ", ipc_data[i]);
            printf("\n");
            break;
        }
    }
    if(num_repeat <= 0)
        printf("ipc unit test pass, repeat: %d\n", IPC_UNIT_TEST_REPEAT);
    xAudio_Ipc_Deinit(arpchdl);
    return 0;
}

#define RPC_FUNC_DUMMY 0x1
#define RPC_FUNC_SQUARE 0x2
static int rpc_unit_tset(int argc, char* argv[]) {
    tAmlDummyRpc dummy_rpc_param;
    int ret = 0;
    int arpchdl = 0;
    if (argc != 3) {
        printf("invalid param number:%d\n", argc);
        return -1;
    }
    dummy_rpc_param.func_code = atoi(argv[0]);
    dummy_rpc_param.input_param = atoi(argv[1]);
    dummy_rpc_param.task_sleep_ms = atoi(argv[2]);
    dummy_rpc_param.task_id = getpid();
    arpchdl = xAudio_Ipc_init();
    xAIPC(arpchdl, MBX_CMD_RPC_TEST, &dummy_rpc_param, sizeof(dummy_rpc_param));
    xAudio_Ipc_Deinit(arpchdl);
    if (dummy_rpc_param.func_code == RPC_FUNC_SQUARE
        && (dummy_rpc_param.input_param*dummy_rpc_param.input_param) != dummy_rpc_param.output_param) {
        ret = -1;
    }
    printf("RPC unit test %s, func code:%d, input param:%d, output param:%d, task id:%d, sleep: %d ms\n",
            (ret==0)?"SUCESS":"FAILE",
    dummy_rpc_param.func_code, dummy_rpc_param.input_param,
    dummy_rpc_param.output_param, dummy_rpc_param.task_id,
    dummy_rpc_param.task_sleep_ms);
    return 0;
}


#define SHM_UNIT_TEST_REPEAT 50
static int shm_uint_tset(void)
{
    int num_repeat = SHM_UNIT_TEST_REPEAT;
    char samples[16] = {0};
    void* pVirSrc = NULL;
    void* pVirDst = NULL;
    while(num_repeat--) {
        AML_MEM_HANDLE hDst, hSrc;
        hDst = AML_MEM_Allocate(sizeof(samples));
        hSrc = AML_MEM_Allocate(sizeof(samples));
        memset((void*)samples, num_repeat, sizeof(samples));
        pVirSrc = AML_MEM_GetVirtAddr(hSrc);
        memcpy((void*)pVirSrc, samples, sizeof(samples));
        AML_MEM_Clean(hSrc, sizeof(samples));
        Aml_ACodecMemory_Transfer(hDst, hSrc, sizeof(samples));
        AML_MEM_Invalidate(hDst, sizeof(samples));
        pVirDst = AML_MEM_GetVirtAddr(hSrc);
            if (memcmp((void*)pVirDst, samples, sizeof(samples))) {
            printf("shm unit test fail, repeat:%d\n",
            SHM_UNIT_TEST_REPEAT - num_repeat);
            break;
        } else {
            /*char* k = (char*)pVirDst;
            for(i = 0; i < sizeof(samples); i++)
            printf("0x%x ", k[i]);
            printf("\n");*/
        }
        AML_MEM_Free(hSrc);
        AML_MEM_Free(hDst);
    }
    if(num_repeat <= 0)
        printf("shm unit test pass, repeat %d\n", SHM_UNIT_TEST_REPEAT);

    num_repeat = SHM_UNIT_TEST_REPEAT;
    while(num_repeat--) {
        AML_MEM_HANDLE hShm = AML_MEM_Allocate(sizeof(samples));
        printf("Allocate shm:%p\n", hShm);
    }
    sleep(1);
    AML_MEM_Recycle((int)getpid());

    return 0;
}

void aml_hifi_inside_wakeup()
{
    int handle = xAudio_Ipc_init();
    xAIPC(handle, MBX_WAKE_ENGINE_DEMO, NULL, 0);
    xAudio_Ipc_Deinit(handle);
}

static void usage()
{
    printf ("\033[1mipc unit test usage:\033[m hifi4rpc_client_test --ipc\n");

    printf ("\033[1mrpc unit test usage:\033[m hifi4rpc_client_test --rpc $func_code[1:dummy, 2:square] $input_param $sleep_time[ms]\n");

    printf ("\033[1mshared memory unit test usage:\033[m hifi4rpc_client_test --shm\n");

    printf ("\033[1mmp3dec Usage:\033[m hifi4rpc_client_test --mp3dec $input_file $output_file\n");

    printf ("\033[1maacdec Usage:\033[m hifi4rpc_client_test --aacdec $input_file $output_file\n");

    printf ("\033[1mpcmplay Usage:\033[m hifi4rpc_client_test --pcmplay $pcm_file\n");

    printf ("\033[1mpcmcap Usage:\033[m hifi4rpc_client_test --pcmcap  $pcm_file $input_mode[1:tdmin,3:tdmin&loopback, 4:pdmin]\n");

    printf ("\033[1mpcmplay-buildin Usage:\033[m hifi4rpc_client_test --pcmplay-buildin\n");

    printf ("\033[1mhifi inside wakeup test Usage:\033[m hifi4rpc_client_test --hifiwake\n");

    printf ("\033[1mipc unit test usage:\033[m hifi4rpc_client_test --ipc\n");

    printf ("\033[1mvsp-rsp Usage:\033[m hifi4rpc_client_test --vsp-rsp $input_file $output_file $in_rate $out_rate\n");

    printf ("\033[1mvsp-awe-unit Usage:\033[m hifi4rpc_client_test --vsp-awe-unit $mic0.pcm $mic1.pcm $ref.pcm $out_asr.pcm $out_voip.pcm\n");

    printf ("\033[1mvsp-awe-dspin Usage:\033[m hifi4rpc_client_test --vsp-awe-dspin $out_asr.pcm $out_voip.pcm\n");

}


int main(int argc, char* argv[]) {
    int c = -1;
    int option_index = 0;
    struct option long_options[] =
    {
        {"help", no_argument, NULL, 0},
        {"ipc", no_argument, NULL, 1},
        {"shm", no_argument, NULL, 2},
        {"mp3dec", no_argument, NULL, 3},
        {"pcmplay", no_argument, NULL, 4},
        {"pcmcap", no_argument, NULL, 5},
        {"pcmplay-buildin", no_argument, NULL, 6},
        {"aacdec", no_argument, NULL, 7},
        {"vsp-rsp", no_argument, NULL, 8},
        {"rpc", no_argument, NULL, 9},
        {"vsp-awe-unit", no_argument, NULL, 10},
        {"vsp-awe-dspin", no_argument, NULL, 11},
        {"hifiwake", no_argument, NULL, 12},
        {0, 0, 0, 0}
    };
    c = getopt_long (argc, argv, "hvV", long_options, &option_index);
    if(-1 == c) {
        printf("error options\n");
    }
    switch(c)
    {
        case 0:
            usage();
            break;
        case 1:
            {
                TIC;
                ipc_uint_tset();
                TOC;
                printf("ipc unit test use:%u ms\n", ms);
            }
            break;
        case 2:
            {
                TIC;
                shm_uint_tset();
                TOC;
                printf("shm unit test use %u ms\n", ms);
            }
            break;
        case 3:
            if (2 == argc - optind || 3 == argc - optind) {
                TIC;
                mp3_offload_dec(argc - optind, &argv[optind]);
                TOC;
                printf("mp3 offload decoder use:%u ms\n", ms);
            }
            else {
                usage();
                exit(1);
            }
            break;
        case 4:
            if (1 == argc - optind)
                pcm_play_test(argc - optind, &argv[optind]);
                else {
                usage();
                exit(1);
            }
            break;
        case 5:
            if (1 == argc - optind || 2 ==  argc - optind)
                pcm_capture_test(argc - optind, &argv[optind]);
            else {
                usage();
                exit(1);
            }
            break;
        case 6:
            pcm_play_buildin();
        case 7:
            if (2 == argc - optind || 3 == argc - optind) {
                TIC;
                aac_offload_dec(argc - optind, &argv[optind]);
                TOC;
                printf("aac offload decoder use:%u ms\n", ms);
            }
            else {
                usage();
                exit(1);
            }
            break;
        case 8:
            if (2 == argc - optind || 4 == argc - optind) {
                TIC;
                offload_vsp_rsp(argc - optind, &argv[optind]);
                TOC;
                printf("offload voice signal resampler use:%u ms\n", ms);
            }
            else {
                usage();
                exit(1);
            }
            break;
        case 9:
            if (3 == argc - optind){
                TIC;
                rpc_unit_tset(argc - optind, &argv[optind]);
                TOC;
                printf("rpc unit test use:%u ms\n", ms);
            } else {
                usage();
                exit(1);
            }
            break;
        case 10:
            if (5 == argc - optind || 6 == argc - optind){
                TIC;
                aml_wake_engine_unit_test(argc - optind, &argv[optind]);
                TOC;
                printf("awe unit test use:%u ms\n", ms);
            } else {
                usage();
                exit(1);
            }
            break;
        case 11:
            if (2 == argc - optind){
                aml_wake_engine_dspin_test(argc - optind, &argv[optind]);
            } else {
                usage();
                exit(1);
            }
            break;
        case 12:
            aml_hifi_inside_wakeup();
            break;
        case '?':
            usage();
            exit(1);
            break;
    }
    return 0;
}
