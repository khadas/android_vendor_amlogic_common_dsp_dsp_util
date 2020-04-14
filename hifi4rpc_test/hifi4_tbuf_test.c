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


#define SAMPLE_MS 48
#define SAMPLE_BYTES 4
#define SAMPLE_CH 16
#define CHUNK_MS 16
#define CHUNK_BYTES (SAMPLE_MS*SAMPLE_BYTES*SAMPLE_CH*CHUNK_MS)
#define TBUF_SIZE (10*CHUNK_BYTES)

typedef struct {
    AML_TBUF_HANDLE tbuf;
    FILE* fp;
    int totalSize;
    int chunkNum;
} capturer_t;

typedef struct {
    AML_TBUF_HANDLE tbuf;
    TBUF_READER_T eType;
    int remaindSize;
    int chunkNum;
    int aipc;
    char name[64];
} hifi4_writer_t;

typedef struct {
    FILE* fp;
    int remaindSize;
    int aipc;
    char name[64];
} hifi4_reader_t;

typedef struct {
    int remaindSize;
    int aipc;
    char name[64];
} hifi4_loopback_t;

void* thread_capture_pcm(void * arg)
{
    capturer_t* pCapturerCtx = (capturer_t*)arg;
    int bExit = 0;
    int sleepCnt = 0;
    while(!bExit) {
        size_t szAvail = 0;
        int nRead = 0;
        void* phy = NULL;
        void* vir = NULL;
        /*Try to capture audio to T buffer align with chunk*/
        while (1) {
            AML_TBUF_GetSpace(pCapturerCtx->tbuf, &szAvail);
            if (szAvail < CHUNK_BYTES) {
                usleep(1000);
                sleepCnt++;
                if (sleepCnt > 1000) {
                    printf("T buffer overrun:%zu %d %d\n", szAvail, pCapturerCtx->totalSize, pCapturerCtx->chunkNum);
                    sleepCnt = 0;
                }
            }
            else {
                szAvail = CHUNK_BYTES;
                break;
            }
        }
        AML_TBUF_GetWrPtr(pCapturerCtx->tbuf, &phy, &vir);
        nRead = fread(vir, 1, szAvail, pCapturerCtx->fp);
        if (nRead != szAvail) {
            bExit = 1;
            printf("Capture thread reach EOF:%d\n", nRead);
        }
        AML_TBUF_UpdateWrOffset(pCapturerCtx->tbuf, nRead);
        pCapturerCtx->totalSize += nRead;
        pCapturerCtx->chunkNum++;
        TBUF_DEBUG("capture total:%d\n", pCapturerCtx->totalSize);
    }
    printf("Exit capture thread:%d,%d\n", pCapturerCtx->totalSize, pCapturerCtx->chunkNum);
    return NULL;
}

void* thread_write_dsp(void * arg)
{
    hifi4_writer_t* pWriterCtx = (hifi4_writer_t*)arg;
    int sleepCnt = 0;
    AML_TBUF_AddReader(pWriterCtx->tbuf, pWriterCtx->eType);
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
        AML_MEM_Clean(phy, szAvail);
        /*IPC to HiFi, block here till HiFi consume the buffer*/
        pcm_io_st io_arg;
        memset(&io_arg, 0, sizeof(io_arg));
        io_arg.data = (xpointer)phy;
        io_arg.count = szAvail;
        TBUF_DEBUG("Before IPC:%s %d\n", pWriterCtx->name, io_arg.count);
        xAIPC(pWriterCtx->aipc, MBX_CMD_IOBUF_ARM2DSP, &io_arg, sizeof(io_arg));
        TBUF_DEBUG("After IPC:%s %d\n", pWriterCtx->name, io_arg.count);
        AML_TBUF_UpdateRdOffset(pWriterCtx->tbuf, pWriterCtx->eType, szAvail);
        pWriterCtx->remaindSize -= io_arg.count;
        pWriterCtx->chunkNum++;
        TBUF_DEBUG("%d %s %d %d\n", pWriterCtx->remaindSize, pWriterCtx->name, szAvail, io_arg.count);
    }
    AML_TBUF_RemoveReader(pWriterCtx->tbuf, pWriterCtx->eType);
    printf("Exit write thread %s:%d\n", pWriterCtx->name, pWriterCtx->chunkNum);
    return NULL;
}

void* thread_read_dsp(void * arg)
{
    hifi4_reader_t* pReaderCtx = (hifi4_reader_t*)arg;
    AML_MEM_HANDLE hShm = AML_MEM_Allocate(CHUNK_BYTES);
    void* phy = AML_MEM_GetPhyAddr(hShm);
    void* vir = AML_MEM_GetVirtAddr(hShm);
    while(pReaderCtx->remaindSize > 0) {
        /*IPC to HiFi, block here till HiFi write audio to the buffer*/
        pcm_io_st io_arg;
        memset(&io_arg, 0, sizeof(io_arg));
        io_arg.data = (xpointer)phy;
        io_arg.count = CHUNK_BYTES;
        TBUF_DEBUG("Before IPC: %s %d\n", pReaderCtx->name, io_arg.count);
        xAIPC(pReaderCtx->aipc, MBX_CMD_IOBUF_DSP2ARM, &io_arg, sizeof(io_arg));
        TBUF_DEBUG("After IPC: %s %d\n", pReaderCtx->name, io_arg.count);
        AML_MEM_Invalidate(phy, io_arg.count);
        fwrite(vir, 1, io_arg.count, pReaderCtx->fp);
        pReaderCtx->remaindSize -= io_arg.count;
        TBUF_DEBUG("%d %s %d %d\n", pReaderCtx->remaindSize, pReaderCtx->name, io_arg.count, io_arg.count);
    }
    printf("Exit read thread:%s\n", pReaderCtx->name);
    AML_MEM_Free(hShm);
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
    writerACtx.aipc = -1;
    memset(&writerBCtx, 0, sizeof(hifi4_writer_t));
    writerBCtx.aipc = -1;
    memset(&readerACtx, 0, sizeof(hifi4_reader_t));
    readerACtx.aipc = -1;
    memset(&readerBCtx, 0, sizeof(hifi4_reader_t));
    readerBCtx.aipc = -1;
    memset(&loopbackACtx, 0, sizeof(hifi4_loopback_t));
    loopbackACtx.aipc = -1;
    memset(&loopbackBCtx, 0, sizeof(hifi4_loopback_t));
    loopbackBCtx.aipc = -1;


    if (argc != 3) {
        printf("Invalid parameter:%d\n", argc);
        return -1;
    }
    FILE* fInput = fopen(argv[0], "rb");
    FILE* fOutputA = fopen(argv[1], "w+b");
    FILE* fOutputB = fopen(argv[2], "w+b");
    if (!fInput || !fOutputA || !fOutputB) {
        printf("Failed to open file:%p %p %p\n", fInput, fOutputA, fOutputB);
        goto recycle_resource;
    }

    tbuf = AML_TBUF_Create(TBUF_SIZE);
    if (!tbuf) {
        printf("Failed create T buf\n");
        goto recycle_resource;
    }

    captureCtx.fp = fInput;
    captureCtx.tbuf = tbuf;

    fseek(captureCtx.fp, 0, SEEK_END);
    fileSize = ftell(captureCtx.fp);
    fseek(captureCtx.fp, 0, SEEK_SET);

    writerACtx.eType = TBUF_READER_A;
    writerACtx.tbuf = tbuf;
    writerACtx.remaindSize = fileSize;
    writerACtx.aipc = xAudio_Ipc_Init(0);
    snprintf(writerACtx.name, sizeof(writerACtx.name), "WriterA");

    writerBCtx.eType = TBUF_READER_B;
    writerBCtx.tbuf = tbuf;
    writerBCtx.remaindSize = fileSize;
    writerBCtx.aipc = xAudio_Ipc_Init(1);
    snprintf(writerBCtx.name, sizeof(writerBCtx.name), "WriterB");

    readerACtx.fp = fOutputA;
    readerACtx.aipc = xAudio_Ipc_Init(0);
    readerACtx.remaindSize = fileSize;
    snprintf(readerACtx.name, sizeof(readerACtx.name), "ReaderA");

    readerBCtx.fp = fOutputB;
    readerBCtx.aipc = xAudio_Ipc_Init(1);
    readerBCtx.remaindSize = fileSize;
    snprintf(readerBCtx.name, sizeof(readerBCtx.name), "ReaderB");

    loopbackACtx.aipc = xAudio_Ipc_Init(0);
    loopbackACtx.remaindSize = fileSize;
    snprintf(loopbackACtx.name, sizeof(loopbackACtx.name), "LoopbackA");

    loopbackBCtx.aipc = xAudio_Ipc_Init(1);
    loopbackBCtx.remaindSize = fileSize;
    snprintf(loopbackBCtx.name, sizeof(loopbackBCtx.name), "LoopbackB");

    if ((writerACtx.aipc == -1) || (writerBCtx.aipc == -1) ||
        (readerACtx.aipc == -1) || (readerBCtx.aipc == -1) ||
        (loopbackACtx.aipc == -1) || (loopbackBCtx.aipc == -1)) {
        printf("Failed to init IPC handle:%d %d %d %d %d %d\n",
            writerACtx.aipc, writerBCtx.aipc, readerACtx.aipc, readerBCtx.aipc,
            loopbackACtx.aipc, loopbackBCtx.aipc);
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
    pthread_create(&readerAThread, NULL, thread_read_dsp, (void*)&readerACtx);
    pthread_create(&readerBThread, NULL, thread_read_dsp, (void*)&readerBCtx);
    pthread_create(&loopbackAThread, NULL, thread_loopback, (void*)&loopbackACtx);
    pthread_create(&loopbackBThread, NULL, thread_loopback, (void*)&loopbackBCtx);
    pthread_join(captureThread,NULL);
    pthread_join(writeAThread,NULL);
    pthread_join(writeBThread,NULL);
    pthread_join(readerAThread,NULL);
    pthread_join(readerBThread,NULL);
    pthread_join(loopbackAThread,NULL);
    pthread_join(loopbackBThread,NULL);

recycle_resource:
    if (writerACtx.aipc != -1)
        xAudio_Ipc_Deinit(writerACtx.aipc);
    if (writerBCtx.aipc != -1)
        xAudio_Ipc_Deinit(writerBCtx.aipc);
    if (readerACtx.aipc != -1)
        xAudio_Ipc_Deinit(readerACtx.aipc);
    if (readerBCtx.aipc != -1)
        xAudio_Ipc_Deinit(readerBCtx.aipc);
    if (loopbackACtx.aipc != -1)
        xAudio_Ipc_Deinit(loopbackACtx.aipc);
    if (loopbackBCtx.aipc != -1)
        xAudio_Ipc_Deinit(loopbackBCtx.aipc);
    if (captureCtx.fp != NULL)
        fclose(captureCtx.fp);
    if (readerACtx.fp != NULL)
        fclose(readerACtx.fp);
    if (readerBCtx.fp != NULL)
        fclose(readerBCtx.fp);
    AML_TBUF_Destroy(tbuf);
    return 0;
}

