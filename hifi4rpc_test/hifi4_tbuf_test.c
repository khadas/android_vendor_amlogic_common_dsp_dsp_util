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
 * amlogic T buffer API.
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "aipc_type.h"
#include "ipc_cmd_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_shm.h"
#include "aml_tbuf_api.h"
#include "aml_flatbuf_api.h"
#include "asoundlib.h"
#include "generic_macro.h"

#define TBUF_SIZE (10*CHUNK_BYTES)

typedef enum {
    CAPTURE_INPUT_INV = 0,
    CAPTURE_INPUT_FILE = 1,
    CAPTURE_INPUT_PCM = 2,
    CAPTURE_INPUT_MAX = 3,
} CAPTURE_INPUT_T;

typedef struct {
    AML_TBUF_HANDLE tbuf;
    FILE* fp;
    struct pcm *pcm;
    int remaindSize;
    int chunkNum;
    CAPTURE_INPUT_T inputType;
} capturer_t;

typedef struct {
    AML_TBUF_HANDLE tbuf;
    TBUF_READER_T eType;
    int remaindSize;
    int chunkNum;
    AML_FLATBUF_HANDLE hFlat;
    char name[64];
} hifi4_writer_t;

typedef struct {
    FILE* fp;
    int remaindSize;
    AML_FLATBUF_HANDLE hFlat;
    char name[64];
    int chunkNum;
} hifi4_reader_t;

typedef struct {
    int remaindSize;
    int aipc;
    char name[64];
} hifi4_loopback_t;

int capturer_input(capturer_t* pCapturerCtx, void* data, size_t* size)
{
    int ret = 0;
    if (pCapturerCtx->inputType == CAPTURE_INPUT_FILE) {
        size_t nRead = 0;
        nRead = fread(data, 1, *size, pCapturerCtx->fp);
        if (nRead != *size) {
            printf("Capture thread reach EOF:%d\n", nRead);
            *size = nRead;
            ret = -1;
        }
    }
    else if (pCapturerCtx->inputType == CAPTURE_INPUT_PCM) {
        ret = pcm_read(pCapturerCtx->pcm, data, *size);
        if (ret != 0) {
            printf("/n Errors appears in hw\n");
            *size = 0;
        }
    }
    return ret;
}

void* thread_capture_pcm(void * arg)
{
    capturer_t* pCapturerCtx = (capturer_t*)arg;
    int bExit = 0;
    int sleepCnt = 0;
    while(!bExit && pCapturerCtx->remaindSize) {
        size_t szAvail = 0;
        int ret = 0;
        void* phy = NULL;
        void* vir = NULL;
        /*Try to capture audio to T buffer align with chunk*/
        while (1) {
            AML_TBUF_GetSpace(pCapturerCtx->tbuf, &szAvail);
            if (szAvail < CHUNK_BYTES) {
                usleep(1000);
                sleepCnt++;
                if (sleepCnt > 1000) {
                    printf("T buffer overrun:%zu %d %d\n", szAvail, pCapturerCtx->remaindSize, pCapturerCtx->chunkNum);
                    sleepCnt = 0;
                }
            }
            else {
                szAvail = CHUNK_BYTES;
                break;
            }
        }
        AML_TBUF_GetWrPtr(pCapturerCtx->tbuf, &phy, &vir);
        szAvail = AMX_MIN((int)szAvail, pCapturerCtx->remaindSize);
        ret = capturer_input(pCapturerCtx, vir, &szAvail);
        if (ret != 0) {
            bExit = 1;
        }
        AML_TBUF_UpdateWrOffset(pCapturerCtx->tbuf, szAvail);
        pCapturerCtx->remaindSize -= szAvail;
        pCapturerCtx->chunkNum++;
        TBUF_DEBUG("capture total:%d\n", pCapturerCtx->remaindSize);
    }
    printf("Exit capture thread:%d,%d\n", pCapturerCtx->remaindSize, pCapturerCtx->chunkNum);
    return NULL;
}

void* thread_write_dsp(void * arg)
{
    hifi4_writer_t* pWriterCtx = (hifi4_writer_t*)arg;
    int sleepCnt = 0;
    while(pWriterCtx->remaindSize > 0) {
        size_t szAvail = 0;
        void* phy = NULL;
        void* vir = NULL;
        /*Try to feed audio from T buffer to dsp align with chunk*/
        while (1) {
            AML_TBUF_GetFullness(pWriterCtx->tbuf, pWriterCtx->eType, &szAvail);
            if (szAvail < CHUNK_BYTES) {
                if (pWriterCtx->remaindSize < CHUNK_BYTES) {
                    printf("Write thread reach EOF:%zu\n", szAvail);
                    break;
                }
                usleep(1000);
                sleepCnt++;
                if (sleepCnt > 1000) {
                    printf("T buffer underrun %s :%zu %d %d\n",
                         pWriterCtx->name, szAvail, pWriterCtx->remaindSize,pWriterCtx->chunkNum);
                    sleepCnt = 0;
                }
            }
            else {
                szAvail = CHUNK_BYTES;
                break;
            }
        }
        AML_TBUF_GetRdPtr(pWriterCtx->tbuf, pWriterCtx->eType, &phy, &vir);
        /*IPC to HiFi, block here till HiFi consume the buffer*/
        TBUF_DEBUG("Before IPC:%s %d\n", pWriterCtx->name, szAvail);
        szAvail = AML_FLATBUF_Write(pWriterCtx->hFlat, vir, szAvail, -1);
        TBUF_DEBUG("After IPC:%s %d\n", pWriterCtx->name, szAvail);
        AML_TBUF_UpdateRdOffset(pWriterCtx->tbuf, pWriterCtx->eType, szAvail);
        pWriterCtx->remaindSize -= szAvail;
        if (szAvail)
            pWriterCtx->chunkNum++;
        TBUF_DEBUG("%s %d %d %d\n", pWriterCtx->name, pWriterCtx->remaindSize, szAvail, pWriterCtx->chunkNum);        
    }
    printf("Exit write thread %s:%d\n", pWriterCtx->name, pWriterCtx->chunkNum);
    return NULL;
}

void* thread_read_dsp(void * arg)
{
    hifi4_reader_t* pReaderCtx = (hifi4_reader_t*)arg;
    void* buf = malloc(CHUNK_BYTES);
    while(pReaderCtx->remaindSize > 0) {
        /*IPC to HiFi, block here till HiFi write audio to the buffer*/
        int nRead = CHUNK_BYTES;
        TBUF_DEBUG("Before IPC: %s %d\n", pReaderCtx->name, nRead);
        nRead = AML_FLATBUF_Read(pReaderCtx->hFlat, buf, nRead, CHUNK_MS);
        TBUF_DEBUG("After IPC: %s %d\n", pReaderCtx->name, nRead);
        fwrite(buf, 1, nRead, pReaderCtx->fp);
        pReaderCtx->remaindSize -= nRead;
        if (nRead)
            pReaderCtx->chunkNum++;
        TBUF_DEBUG(" %s %d %d %d\n",  pReaderCtx->name, pReaderCtx->remaindSize, nRead, pReaderCtx->chunkNum);
    }
    free(buf);
    printf("Exit read thread:%s\n", pReaderCtx->name);
    return NULL;
}

void* thread_loopback(void * arg)
{
    hifi4_loopback_t* pLoopbackCtx = (hifi4_loopback_t*)arg;
    /*IPC to HiFi, start a demo task in HiFi, to looback audio*/
    pcm_io_st io_arg;
    memset(&io_arg, 0, sizeof(io_arg));
    io_arg.count = pLoopbackCtx->remaindSize;
    TBUF_DEBUG("Before IPC: %s\n", pLoopbackCtx->name);
    xAIPC(pLoopbackCtx->aipc, MBX_CMD_IOBUF_DEMO, &io_arg, sizeof(io_arg));
    TBUF_DEBUG("After IPC: %s\n", pLoopbackCtx->name);
    return NULL;
}


int hifi4_tbuf_test(int argc, char* argv[])
{
    struct flatbuffer_config flatCfg;
    flatCfg.size = 2*CHUNK_BYTES;
    AML_TBUF_HANDLE tbuf = NULL;
    /*T buffer can store 1 seconds audio*/
    size_t fileSize = 0;
    capturer_t captureCtx;
    hifi4_writer_t writerACtx;
    hifi4_writer_t writerBCtx;
    hifi4_reader_t readerACtx;
    hifi4_reader_t readerBCtx;
    hifi4_loopback_t loopbackACtx;
    hifi4_loopback_t loopbackBCtx;
    memset(&captureCtx, 0, sizeof(capturer_t));
    memset(&writerACtx, 0, sizeof(hifi4_writer_t));
    memset(&writerBCtx, 0, sizeof(hifi4_writer_t));
    memset(&readerACtx, 0, sizeof(hifi4_reader_t));
    memset(&readerBCtx, 0, sizeof(hifi4_reader_t));
    memset(&loopbackACtx, 0, sizeof(hifi4_loopback_t));
    loopbackACtx.aipc = -1;
    memset(&loopbackBCtx, 0, sizeof(hifi4_loopback_t));
    loopbackBCtx.aipc = -1;

    if (argc != 4) {
        printf("Invalid parameter:%d\n", argc);
        return -1;
    }

    if (!strcmp("file", argv[0])) {
        captureCtx.inputType = CAPTURE_INPUT_FILE;
        FILE* fInput = fopen(argv[1], "rb");
        if (!fInput) {
            printf("Failed to open files:%s\n", argv[1]);
            goto recycle_resource;
        }
        captureCtx.fp = fInput;
        fseek(captureCtx.fp, 0, SEEK_END);
        fileSize = ftell(captureCtx.fp);
        fseek(captureCtx.fp, 0, SEEK_SET);
    } else if (!strcmp("pcm", argv[0])) {
        captureCtx.inputType = CAPTURE_INPUT_PCM;
        struct pcm_config cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.channels = SAMPLE_CH;
        cfg.rate = SAMPLE_MS*1000;
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
        captureCtx.pcm = pcm_open(card, device, flags, &cfg);
        if (captureCtx.pcm == NULL) {
            printf("failed to open pcm\n");
            goto recycle_resource;
        }
        if (!pcm_is_ready(captureCtx.pcm)) {
            printf("pcm is not ready:%s\n", pcm_get_error(captureCtx.pcm));
            goto recycle_resource;
        }
        int seconds = atoi(argv[1]);
        fileSize = seconds*1000*SAMPLE_MS*SAMPLE_CH*SAMPLE_BYTES;
    }

    FILE* fOutputA = fopen(argv[2], "w+b");
    FILE* fOutputB = fopen(argv[3], "w+b");
    if (!fOutputA || !fOutputB) {
        printf("Failed to open files:%s or %s\n", argv[2], argv[3]);
        goto recycle_resource;
    }

    tbuf = AML_TBUF_Create(TBUF_SIZE);
    if (!tbuf) {
        printf("Failed create T buf\n");
        goto recycle_resource;
    }
    captureCtx.tbuf = tbuf;
    captureCtx.remaindSize = fileSize;

    writerACtx.eType = TBUF_READER_A;
    writerACtx.tbuf = tbuf;
    writerACtx.remaindSize = fileSize;
    flatCfg.phy_ch = FLATBUF_CH_ARM2DSPA;
    writerACtx.hFlat = AML_FLATBUF_Create("IOBUF_ARM2DSP", FLATBUF_FLAG_WR, &flatCfg);
    snprintf(writerACtx.name, sizeof(writerACtx.name), "WriterA");

    writerBCtx.eType = TBUF_READER_B;
    writerBCtx.tbuf = tbuf;
    writerBCtx.remaindSize = fileSize;
    flatCfg.phy_ch = FLATBUF_CH_ARM2DSPB;
    writerBCtx.hFlat = AML_FLATBUF_Create("IOBUF_ARM2DSP", FLATBUF_FLAG_WR, &flatCfg);
    snprintf(writerBCtx.name, sizeof(writerBCtx.name), "WriterB");

    readerACtx.fp = fOutputA;
    flatCfg.phy_ch = FLATBUF_CH_ARM2DSPA;
    readerACtx.hFlat = AML_FLATBUF_Create("IOBUF_DSP2ARM",  FLATBUF_FLAG_RD, &flatCfg);
    readerACtx.remaindSize = fileSize;
    snprintf(readerACtx.name, sizeof(readerACtx.name), "ReaderA");

    readerBCtx.fp = fOutputB;
    flatCfg.phy_ch = FLATBUF_CH_ARM2DSPB;
    readerBCtx.hFlat = AML_FLATBUF_Create("IOBUF_DSP2ARM",  FLATBUF_FLAG_RD, &flatCfg);
    readerBCtx.remaindSize = fileSize;
    snprintf(readerBCtx.name, sizeof(readerBCtx.name), "ReaderB");

    AML_TBUF_AddConsumer(tbuf, TBUF_READER_A);
    AML_TBUF_AddConsumer(tbuf, TBUF_READER_B);

    /*Start Dsp loopback task*/
    loopbackACtx.aipc = xAudio_Ipc_Init(0);
    loopbackACtx.remaindSize = fileSize;
    snprintf(loopbackACtx.name, sizeof(loopbackACtx.name), "LoopbackA");

    loopbackBCtx.aipc = xAudio_Ipc_Init(1);
    loopbackBCtx.remaindSize = fileSize;
    snprintf(loopbackBCtx.name, sizeof(loopbackBCtx.name), "LoopbackB");

    if ((loopbackACtx.aipc == -1) || (loopbackBCtx.aipc == -1)) {
       printf("Failed to init IPC handle:%d %d\n", loopbackACtx.aipc, loopbackBCtx.aipc);
       goto recycle_resource;
    }

    pthread_t captureThread;
    pthread_t writeAThread;
    pthread_t writeBThread;
    pthread_t readerAThread;
    pthread_t readerBThread;
    pthread_t loopbackAThread;
    pthread_t loopbackBThread;
    pthread_create(&captureThread, NULL, thread_capture_pcm, (void*)&captureCtx);
    pthread_create(&writeAThread, NULL, thread_write_dsp, (void*)&writerACtx);
    pthread_create(&writeBThread, NULL, thread_write_dsp, (void*)&writerBCtx);
    pthread_create(&readerBThread, NULL, thread_read_dsp, (void*)&readerBCtx);
    pthread_create(&readerAThread, NULL, thread_read_dsp, (void*)&readerACtx);
    pthread_create(&loopbackAThread, NULL, thread_loopback, (void*)&loopbackACtx);
    pthread_create(&loopbackBThread, NULL, thread_loopback, (void*)&loopbackBCtx);
    pthread_join(captureThread,NULL);
    pthread_join(writeAThread,NULL);
    pthread_join(writeBThread,NULL);
    pthread_join(readerAThread,NULL);
    pthread_join(readerBThread,NULL);
    pthread_join(loopbackAThread,NULL);
    pthread_join(loopbackBThread,NULL);


    AML_TBUF_RemoveConsumer(tbuf, TBUF_READER_A);
    AML_TBUF_RemoveConsumer(tbuf, TBUF_READER_B);

recycle_resource:
    if (writerACtx.hFlat != NULL)
        AML_FLATBUF_Destroy(writerACtx.hFlat);
    if (writerBCtx.hFlat != NULL)
        AML_FLATBUF_Destroy(writerBCtx.hFlat);
    if (readerACtx.hFlat != NULL)
        AML_FLATBUF_Destroy(readerACtx.hFlat);
    if (readerBCtx.hFlat != NULL)
        AML_FLATBUF_Destroy(readerBCtx.hFlat);
    if (loopbackACtx.aipc != -1)
        xAudio_Ipc_Deinit(loopbackACtx.aipc);
    if (loopbackBCtx.aipc != -1)
        xAudio_Ipc_Deinit(loopbackBCtx.aipc);
    if (captureCtx.fp != NULL && captureCtx.inputType == CAPTURE_INPUT_FILE)
        fclose(captureCtx.fp);
    if (captureCtx.pcm != NULL && captureCtx.inputType == CAPTURE_INPUT_PCM)
        pcm_close(captureCtx.pcm);
    if (readerACtx.fp != NULL)
        fclose(readerACtx.fp);
    if (readerBCtx.fp != NULL)
        fclose(readerBCtx.fp);
    AML_TBUF_Destroy(tbuf);
    return 0;
}

