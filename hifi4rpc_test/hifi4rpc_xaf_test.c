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
 * hifi4 rpc client api, hifi4 tinyalsa
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#ifdef ANDROIDPLATFORM
#include "asoundlib.h"
#elif defined (C4A_HIFI4)
#include <alsa/asoundlib.h>
#endif
#include "generic_macro.h"
#include "aml_audio_util.h"
#include "aml_pcm_api.h"
#include "aml_flatbuf_api.h"
//#define PROFILE_RTT
long get_us() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
    long long us =  tp.tv_sec * 1000 * 1000 + tp.tv_nsec / (1000);
    // printf("cur us=%lld\n", us);
    return us;
}

int bcm_file_test(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Invalid argc:%d\n", argc);
        return -1;
    }
    printf("all arg begin\n");
    int i;
    for (i = 0; i != argc; i++) {
        printf("arg[%d]=%s\n", i, argv[i]);
    }
    printf("all arg end\n");
    int id = atoi(argv[0]);
    printf("Invoke Hifi%d\n", id);

    FILE *fileplay = fopen(argv[1], "rb");
    if (fileplay == NULL) {
        printf("failed to open played pcm file\n");
        return -1;
    }
    const int ms = CHUNK_MS;
    const int oneshot = SAMPLE_MS * ms; // 48KHz
    uint32_t size = oneshot * SAMPLE_CH * SAMPLE_BYTES; // 16channel, 32bit
    uint32_t r;
    struct flatbuffer_config config;
    config.size = 3*size;
    config.phy_ch = (id == 0)?FLATBUF_CH_ARM2DSPA:FLATBUF_CH_ARM2DSPB;
    AML_FLATBUF_HANDLE hFlat = AML_FLATBUF_Create("AML.XAF.CAPTURE", FLATBUF_FLAG_WR,
                                                &config);
    void *buf = malloc(size);
    int loop = 128 * 10; // test 10.24s
    long d0, d1, e;
    for (i = 0; i != loop; i++) {
        d0 = get_us();
        r = fread(buf, 1, size, fileplay);
        if (r != size) {
            rewind(fileplay);
            r = fread(buf, 1, size, fileplay);
        }
        AML_FLATBUF_Write(hFlat, buf, r, -1);
        d1 = get_us();
        e = 16 * 1000 - (d1 - d0);
        // if (i % (128 * 4) == 0) {
        //     printf("real=%ld wait=%ld\n", d1 - d0, e);
        // }
        if (e > 0) {
            usleep(e);
        }
        //printf("%dms pcm_write pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //   ms, p, buf, oneshot, fr);
    }
    fclose(fileplay);
    free(buf);
    AML_FLATBUF_Destroy(hFlat);
    return 0;
}

// !!! depend on libtinyalsa, only link in Android.mk !!!
int bcm_pcm_test(int argc, char* argv[])
{
#ifdef ANDROIDPLATFORM
    int i;
    if (argc != 1) {
        printf("Invalid argc:%d\n", argc);
        return -1;
    }
    for (i = 0; i != argc; i++) {
        printf("arg[%d]=%s\n", i, argv[i]);
    }
    int id = atoi(argv[0]);
    printf("Invoke Hifi%d\n", id);

    struct pcm_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channels = SAMPLE_CH;
    cfg.rate = 48000;
    cfg.period_size = 128;
    cfg.period_count = 4;
    // !!! linux's TINYALSA side's header file's, PCM_FORMAT_S32_LE is 1
    // !!! DSP's TINYALSA side's headefile, PCM_FORMAT_S32_LE is 7
    cfg.format = 1;
    cfg.start_threshold = 0;
    cfg.stop_threshold = 0;
    cfg.silence_threshold = 0;
    cfg.avail_min = 0;
    uint32_t card = 0, device = 8, flags = PCM_IN;
    printf("ch=%d rate=%d period=%dx%d format=%d thr=%d,%d,%d card=%d device=%d\n",
           cfg.channels, cfg.rate, cfg.period_size, cfg.period_count,
           cfg.format, cfg.start_threshold, cfg.stop_threshold,
           cfg.silence_threshold, card, device);
    struct pcm *pcm = pcm_open(card, device, flags, &cfg);
    if (pcm == NULL) {
        printf("failed to open pcm\n");
        return -1;
    }
    printf("pcm=%p\n", pcm);
    // if (!pcm_is_ready(pcm)) {
    //     printf("pcm isn't ready\n");
    //     return -2;
    // }
    const int ms = CHUNK_MS;
    const int oneshot = SAMPLE_MS * ms; // 48KHz
    uint32_t size = oneshot * SAMPLE_CH * SAMPLE_BYTES; // 16channel, 32bit
    uint32_t r;
    struct flatbuffer_config config;
    config.size = 3*size;
    config.phy_ch = (id == 0)?FLATBUF_CH_ARM2DSPA:FLATBUF_CH_ARM2DSPB;
    AML_FLATBUF_HANDLE hFlat = AML_FLATBUF_Create("AML.XAF.CAPTURE", FLATBUF_FLAG_WR,
                                                &config);
    void *buf = malloc(size);
    int sec = 10;
    int loop = 128 * sec;
    printf("capture pcm data, and send it to dsp side in %d seconds\n", sec);
    for (i = 0; i != loop; i++) {
        /** Linux tinyalsa's pcm_read, it return error code
         * When r=0, it means successfully get full filled buffer */
        r = pcm_read(pcm, buf, size);
        if (r != 0) {
            printf("expect size=%u, but get r=%u, quit\n", size, r);
            break;
        }
        // int32_t *sp = (char *)buf;
        // printf("%08x %08x %08x %08x\n", sp[0], sp[1], sp[2], sp[3]);
        AML_FLATBUF_Write(hFlat, buf, size, -1);
        //printf("%dms pcm_write pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //   ms, p, buf, oneshot, fr);
    }
    printf("i=%d loop=%d\n", i, loop);
    pcm_close(pcm);
    free(buf);
    AML_FLATBUF_Destroy(hFlat);
#else
    AMX_UNUSED(argc);
	AMX_UNUSED(argv);
    printf("Not support such case");
#endif
    return 0;
}

typedef struct _io_thread_context_ {
    int id;
    FILE* fp;
    struct pcm *ppcm;
    size_t fileSize;
    struct timespec timeStamp[256];
    int chunkNum;
    void* ctx;
} io_thread_context;
int read_wrapper(io_thread_context* pWriteCtx, void* data, size_t* size)
{
    int ret = 0;
    if (pWriteCtx->fp) {
        size_t nRead = 0;
        nRead = fread(data, 1, *size, pWriteCtx->fp);
        if (nRead != *size) {
            printf("Capture thread reach EOF:%zu\n", nRead);
            *size = nRead;
            ret = -1;
        }
    }
#ifdef ANDROIDPLATFORM
    else if (pWriteCtx->ppcm) {
        ret = pcm_read(pWriteCtx->ppcm, data, *size);
        if (ret != 0) {
            printf("/n Errors appears in hw\n");
            *size = 0;
        }
    }
#endif
    return ret;
}

static void* thread_write_pcm(void * arg)
{
    io_thread_context* pWriteCtx = (io_thread_context*)arg;
    void* buf = malloc(CHUNK_BYTES);
    struct flatbuffer_config config;
    config.size = 2*CHUNK_BYTES;
    config.phy_ch = (pWriteCtx->id == 0)?FLATBUF_CH_ARM2DSPA:FLATBUF_CH_ARM2DSPB;
    AML_FLATBUF_HANDLE hFlat = AML_FLATBUF_Create("IOBUF_ARM2DSP",  FLATBUF_FLAG_WR, &config);
    int bExit = 0;
    while (!bExit && pWriteCtx->fileSize > 0) {
        int ret = 0;
        size_t nBytes = AMX_MIN(CHUNK_BYTES, pWriteCtx->fileSize);
        ret = read_wrapper(pWriteCtx, (void*)buf, &nBytes);
        if (pWriteCtx->chunkNum < 256)
            aprofiler_get_cur_timestamp(&pWriteCtx->timeStamp[pWriteCtx->chunkNum]);
        if (ret != 0) {
            bExit = 1;
        }
        nBytes = AML_FLATBUF_Write(hFlat, buf, nBytes, -1);
        pWriteCtx->chunkNum++;
        pWriteCtx->fileSize -= nBytes;
        printf("thread_write_pcm remained:%zu\n", pWriteCtx->fileSize);
    }
    free(buf);
    AML_FLATBUF_Destroy(hFlat);
    return NULL;
}

static void* thread_read_pcm(void * arg)
{
    io_thread_context* pReadCtx = (io_thread_context*)arg;
    void* buf = malloc(CHUNK_BYTES);
    struct flatbuffer_config config;
    config.size = 2*CHUNK_BYTES;
    config.phy_ch = (pReadCtx->id == 0)?FLATBUF_CH_ARM2DSPA:FLATBUF_CH_ARM2DSPB;;
    AML_FLATBUF_HANDLE hFlat = AML_FLATBUF_Create("IOBUF_DSP2ARM",  FLATBUF_FLAG_RD, &config);
    while (pReadCtx->fileSize) {
        int nRead = AMX_MIN(CHUNK_BYTES, pReadCtx->fileSize);
        nRead = AML_FLATBUF_Read(hFlat, buf, nRead, CHUNK_MS);
        if (pReadCtx->chunkNum < 256) {
            aprofiler_get_cur_timestamp(&pReadCtx->timeStamp[pReadCtx->chunkNum]);
            //io_thread_context* pWriteCtx = (io_thread_context*)pReadCtx->ctx;
            //printf("%d:%d\n", pReadCtx->chunkNum,  aprofiler_msec_duration(&pReadCtx->timeStamp[pReadCtx->chunkNum], &pWriteCtx->timeStamp[pReadCtx->chunkNum]));
        }
        #ifndef PROFILE_RTT
        if (nRead)
            fwrite((void*)buf, 1, nRead, pReadCtx->fp);
        #endif
        pReadCtx->fileSize -= nRead;
        pReadCtx->chunkNum++;
        printf("thread_read_pcm remained:%zu\n", pReadCtx->fileSize);
    }
    free(buf);
    AML_FLATBUF_Destroy(hFlat);
    return NULL;
}

int pcm_loopback_test(int argc, char* argv[])
{
    int aipc = -1;
    int fileSize = 0;
    io_thread_context writeCtx;
    memset(&writeCtx, 0, sizeof(io_thread_context));
    io_thread_context readCtx;
    memset(&readCtx, 0, sizeof(io_thread_context));

    if (argc != 4) {
        printf("pcm_loopback_test: invalid parameter number %d\n", argc);
        return -1;
    }
    int id = atoi(argv[0]);

    if (!strcmp("file", argv[1])) {
        FILE* fInput = fopen(argv[2], "rb");
        if (!fInput) {
            printf("Failed to open files:%s\n", argv[2]);
            goto recycle_resource;
        }
        writeCtx.fp = fInput;
        fseek(writeCtx.fp, 0, SEEK_END);
        fileSize = ftell(writeCtx.fp);
        fseek(writeCtx.fp, 0, SEEK_SET);
    } else if (!strcmp("pcm", argv[1])) {
#ifdef ANDROIDPLATFORM
        struct pcm_config cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.channels = SAMPLE_CH;
        cfg.rate = SAMPLE_MS*1000;
        cfg.period_size = 128;
        cfg.period_count = 4;
        cfg.format = 1;
        cfg.start_threshold = 0;
        cfg.stop_threshold = 0;
        cfg.silence_threshold = 0;
        cfg.avail_min = 0;
        writeCtx.ppcm = pcm_open(0, 8, PCM_IN, &cfg);
        if (writeCtx.ppcm == NULL) {
            printf("failed to open pcm\n");
            goto recycle_resource;
        }
        if (!pcm_is_ready(writeCtx.ppcm)) {
            printf("pcm is not ready:%s\n", pcm_get_error(writeCtx.ppcm));
            goto recycle_resource;
        }
        int seconds = atoi(argv[2]);
        fileSize = seconds*1000*SAMPLE_MS*SAMPLE_CH*SAMPLE_BYTES;
        printf("pcm open:%p\n", writeCtx.ppcm);
#else
        printf("Not support such case\n");
        goto recycle_resource;
#endif
    }

    writeCtx.fileSize = fileSize;
    writeCtx.id = id;

    readCtx.fileSize = fileSize;
    readCtx.id = id;
    readCtx.ctx = (void*)&writeCtx;
    readCtx.fp = fopen(argv[3], "w+b");
    if (readCtx.fp == NULL) {
        printf("Failed to open %s\n", argv[3]);
        goto recycle_resource;
    }

    pthread_t readThread;
    pthread_t writeThread;
    pthread_create(&readThread, NULL, thread_read_pcm, (void*)&readCtx);
    pthread_create(&writeThread, NULL, thread_write_pcm, (void*)&writeCtx);

    aipc = xAudio_Ipc_Init(id);
    pcm_io_st io_arg;
    io_arg.count = fileSize;
    xAIPC(aipc, MBX_CMD_IOBUF_DEMO, &io_arg, sizeof(io_arg));
    xAudio_Ipc_Deinit(aipc);

    printf("Invoke loopback task done\n");
    pthread_join(writeThread,NULL);
    pthread_join(readThread,NULL);
#ifdef PROFILE_RTT
    uint32_t i;
    float averageRTT = 0;
    uint32_t largestRTT = 0;
    uint32_t numRTT = AMX_MIN(256, readCtx.chunkNum);
    for (i = 0; i < numRTT; i++) {
        uint32_t rtt = aprofiler_msec_duration(&readCtx.timeStamp[i], &writeCtx.timeStamp[i]);
        if (largestRTT < rtt)
            largestRTT = rtt;
        averageRTT += (float)rtt;
    }
    printf("largestRTT: %dms\n", largestRTT);
    printf("averageRTT: %fms\n", averageRTT/(float)numRTT);
    printf("num of round: %d\n", numRTT);
#endif
recycle_resource:
    if (writeCtx.fp)
        fclose(writeCtx.fp);
#ifdef ANDROIDPLATFORM
    if (writeCtx.ppcm)
        pcm_close(writeCtx.ppcm);
#endif
    if (readCtx.fp)
        fclose(readCtx.fp);
    return 0;
}

int xaf_test(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid argc:%d\n", argc);
        return -1;
    }
    int id = atoi(argv[0]);
    int caseId = atoi(argv[1]);
    printf("Invoke HiFi%d case=%d\n", id, caseId);
    int hdl = xAudio_Ipc_Init(id);
    xAIPC(hdl, MBX_CMD_XAF_TEST, &caseId, sizeof(caseId));
    xAudio_Ipc_Deinit(hdl);
    return 0;
}

int xaf_dump(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid argc:%d\n", argc);
        return -1;
    }
    int id = atoi(argv[0]);
    printf("Invoke HiFi%d\n", id);
    FILE* fpOut = fopen(argv[1], "wb");
    const int ms = CHUNK_MS;
    const int oneshot = SAMPLE_MS * ms; // 48KHz
    uint32_t size = oneshot * SAMPLE_CH * SAMPLE_BYTES; // 16channel, 32bit
    struct flatbuffer_config config;
    config.size = 2*size;
    config.phy_ch = (id == 0)?FLATBUF_CH_ARM2DSPA:FLATBUF_CH_ARM2DSPB;;
    AML_FLATBUF_HANDLE hFlat = AML_FLATBUF_Create("AML.XAF.RENDER",  FLATBUF_FLAG_RD,
                                                  &config);
    void *buf = malloc(size);
    int loop = 4; // test 10.24s
    int32_t remained = size*loop;
    while (remained > 0) {
        uint32_t r = AMX_MIN(size, (uint32_t)remained);
        r = AML_FLATBUF_Read(hFlat, buf, r, -1);
        fwrite(buf, 1, r, fpOut);
        remained -= r;
        printf("recv data remained=%d\n", remained);
    }
    AML_FLATBUF_Destroy(hFlat);
    free(buf);
    fclose(fpOut);
    return 0;
}

typedef struct _aml_pcm_test_context_t_ {
    int card;
    int device;
    FILE *fp;
    int chNum;
    int sampleRate;
    int sampleBytes;
    int chunkMs;
    int sec;
    struct timespec timeStamp[512];
    int frameCnt;
} aml_pcm_test_context;

void* thread_aml_pcm_test(void *arg)
{
    aml_pcm_test_context* pContext = (aml_pcm_test_context*)arg;
    /*printf("Card-%d, capture %d seconds, %dms chunk, chn:%d, sampleRate:%d, sampleBytes:%d\n",
            pContext->card, pContext->sec, pContext->chunkMs, pContext->chNum, pContext->sampleRate,
            pContext->sampleBytes);*/

    struct aml_pcm_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channels = pContext->chNum;
    cfg.rate = pContext->sampleRate;
    cfg.period_size = pContext->chunkMs * (pContext->sampleRate/1000);
    cfg.period_count = 4;
    cfg.format = AML_PCM_FORMAT_S32_LE;
    AML_PCM_HANDLE hPcm = AML_PCM_Open(pContext->card, pContext->device, 0, &cfg);

    /*16 ms per chunk*/
    size_t szChunk = pContext->chunkMs*pContext->chNum*pContext->sampleBytes*(pContext->sampleRate/1000);
    void* buf = malloc(szChunk);

    int remained = pContext->sec*pContext->sampleRate*pContext->chNum*pContext->sampleBytes;
    int timeStampIdx = 0;
    while (remained) {
        size_t bytes = AMX_MIN((int)szChunk, remained);
        int nRead = AML_PCM_Read(hPcm, buf, bytes);
        if (timeStampIdx < 512)
            aprofiler_get_cur_timestamp(&pContext->timeStamp[timeStampIdx++]);
        if (nRead != (int)bytes) {
            printf("AML_PCM_Read something wrong:%d\n", nRead);
            break;
        }
        fwrite(buf, 1, nRead, pContext->fp);
        remained -= nRead;
        pContext->frameCnt++;
    }
    free(buf);
    AML_PCM_Close(hPcm);
    printf("Exit thread_aml_pcm_test\n");
    return NULL;
}

int aml_pcm_test(int argc, char **argv) {
    int sec = 0;
    int chunkMs = 0;
    int chNum = 0;
    int sampleRate = 0;
    int sampleBytes = 0;
    int device = 0;
    pthread_t pcmReadThreadA;
    pthread_t pcmReadThreadB;
    char* pFileNameA = NULL;
    char* pFileNameB = NULL;
    aml_pcm_test_context contextA;
    memset(&contextA, 0, sizeof(aml_pcm_test_context));
    aml_pcm_test_context contextB;
    memset(&contextB, 0, sizeof(aml_pcm_test_context));

    if (argc < 1) {
        printf("Invalid argc %d\n", argc);
        goto aml_pcm_test_recycle;
    }

    if (!strcmp("dual", argv[0]) && argc == 9) {
        sec = atoi(argv[1]);
        chunkMs = atoi(argv[2]);
        chNum = atoi(argv[3]);
        sampleRate = atoi(argv[4]);
        sampleBytes = atoi(argv[5]);
        device = atoi(argv[6]);
        pFileNameA = argv[7];
        pFileNameB = argv[8];
    } else if (!strcmp("single", argv[0]) && argc == 9) {
        sec = atoi(argv[1]);
        chunkMs = atoi(argv[2]);
        chNum = atoi(argv[3]);
        sampleRate = atoi(argv[4]);
        sampleBytes = atoi(argv[5]);
        device = atoi(argv[6]);
        int card = atoi(argv[7]);
        if (0 == card)
            pFileNameA = argv[8];
        else if (1 == card)
            pFileNameB = argv[8];
        else {
            printf("Invalid card:%d\n", card);
            goto aml_pcm_test_recycle;
        }
    } else {
        printf("Invalid parameter %s %d\n", argv[0], argc);
        goto aml_pcm_test_recycle;
    }
    

    if (pFileNameA) {
        contextA.sec = sec;
        contextA.chunkMs = chunkMs;
        contextA.chNum = chNum;
        contextA.sampleRate = sampleRate;
        contextA.sampleBytes = sampleBytes;
        contextA.card = 0;
        contextA.device = device;
        contextA.fp = fopen(pFileNameA, "w+b");
        pthread_create(&pcmReadThreadA, NULL, thread_aml_pcm_test, (void *)&contextA);
        if (contextA.fp == NULL) {
            printf("Failed to open file %s\n",pFileNameA);
            goto aml_pcm_test_recycle;
        }
    }

    if (pFileNameB) {
        contextB.sec = sec;
        contextB.chunkMs = chunkMs;
        contextB.chNum = chNum;
        contextB.sampleRate = sampleRate;
        contextB.sampleBytes = sampleBytes;
        contextB.card = 1;
        contextB.device = device;
        contextB.fp = fopen(pFileNameB, "w+b");
        pthread_create(&pcmReadThreadB, NULL, thread_aml_pcm_test, (void *)&contextB);
        if (contextB.fp == NULL) {
            printf("Failed to open file %s\n", pFileNameB);
            goto aml_pcm_test_recycle;
        }
    }

    if (pFileNameA)
        pthread_join(pcmReadThreadA, NULL);
    if (pFileNameB)
        pthread_join(pcmReadThreadB, NULL);

aml_pcm_test_recycle:
    if (contextA.fp)
        fclose(contextA.fp);
    if (contextB.fp)
        fclose(contextB.fp);
    int maxDelay = 0;
    int maxDelayIdx = 0;
    int timeStampIdx = 0;
    int frameCnt = AMX_MIN(512, contextA.frameCnt);
    while (timeStampIdx < frameCnt) {
        int32_t delay = aprofiler_msec_duration(&contextA.timeStamp[timeStampIdx], &contextB.timeStamp[timeStampIdx]);
        //printf("frame_dix:%d, delay=%dms\n", timeStampIdx, delay);
        if (abs(delay) > maxDelay) {
            maxDelay = abs(delay);
            maxDelayIdx = timeStampIdx;
        }
        timeStampIdx++;
    }
    printf("total frame number:%d, max delay:%d, the frame index of max delay:%d\n",
            frameCnt, maxDelay, maxDelayIdx);
    return 0;
}

