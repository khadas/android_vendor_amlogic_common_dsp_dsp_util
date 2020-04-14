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
 * XAF STREAM API
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "aipc_type.h"
#include "ipc_cmd_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_shm.h"
#include "aml_tbuf_api.h"
#include "aml_pcm_api.h"
#include "asoundlib.h"
#include "generic_macro.h"

#define SAMPLE_MS 48
#define SAMPLE_BYTES 4
#define SAMPLE_CH 16
#define CHUNK_MS 16
#define CHUNK_BYTES (SAMPLE_MS*SAMPLE_BYTES*SAMPLE_CH*CHUNK_MS)
#define TBUF_SIZE (10*CHUNK_BYTES)

typedef struct {
    AML_TBUF_HANDLE tbuf;
    struct pcm* ppcm;
    FILE* fp;
    bool bExit;
    pthread_t hThread;
    int strmPos;
    int chunkNum;
} alsa_capturer_t;

typedef struct {
    AML_TBUF_HANDLE tbuf;
    TBUF_READER_T eType;
    int strmPos;
    int chunkNum;
    int aipc;
    bool bExit;
    pthread_t hThread;
    char name[64];
} dsp_writer_t;

typedef struct {
    AML_MEM_HANDLE hShm;
    void* phy;
    void* vir;
    int aipc;
    char name[64];
    int strmPos;
} dsp_reader_t;

typedef struct {
    int aipc;
    pthread_t hThread;
    char name[64];
    int strmPos;
} dsp_loopback_t;


typedef struct AML_ALSA_T_ {
    AML_TBUF_HANDLE tbuf;
    alsa_capturer_t capture;
    bool isAUsed;
    bool isBUsed;
} AML_ALSA_T;
AML_ALSA_T* pgAlsa = NULL;

typedef struct AML_PCM_T_ {
    AML_ALSA_T* pAlsa;
    dsp_writer_t writer;
    dsp_reader_t reader;
    dsp_loopback_t loopback;
} AML_PCM_T;

void* thread_aml_pcm_capture(void * arg)
{
    alsa_capturer_t* pCapturerCtx = (alsa_capturer_t*)arg;
    int sleepCnt = 0;
    printf("Enter capture thread:%d,%d, %d\n", pCapturerCtx->strmPos, pCapturerCtx->chunkNum, pCapturerCtx->bExit);
    while(!pCapturerCtx->bExit) {
        size_t szAvail = 0;
        int nRead = 0;
        void* phy = NULL;
        void* vir = NULL;
        /*Try to capture audio to T buffer align with chunk*/
        while (1) {
            AML_TBUF_GetSpace(pCapturerCtx->tbuf, &szAvail);
            if (szAvail < CHUNK_BYTES) {
                if (pCapturerCtx->bExit)
                   return NULL;
                usleep(1000);
                sleepCnt++;
                if (sleepCnt > 1000) {
                    printf("T buffer overrun:%zu %d %d\n", szAvail, pCapturerCtx->strmPos, pCapturerCtx->chunkNum);
                    sleepCnt = 0;
                }
            }
            else {
                szAvail = CHUNK_BYTES;
                break;
            }
        }
        AML_TBUF_GetWrPtr(pCapturerCtx->tbuf, &phy, &vir);
        nRead = pcm_read(pCapturerCtx->ppcm, vir, szAvail);
        if (nRead != 0) {
            pCapturerCtx->bExit = 1;
            printf("\npcm devices meet error %d\n", nRead);
        }
        AML_TBUF_UpdateWrOffset(pCapturerCtx->tbuf, szAvail);
        pCapturerCtx->strmPos += szAvail;
        pCapturerCtx->chunkNum++;
        AM_PCM_DEBUG("Capture total:%d\n", pCapturerCtx->strmPos);
    }
    printf("Exit capture thread:%d,%d\n", pCapturerCtx->strmPos, pCapturerCtx->chunkNum);
    return NULL;
}

void* thread_aml_pcm_dsp_loopback(void * arg)
{
    dsp_loopback_t* pLoopbackCtx = (dsp_loopback_t*)arg;

    /*IPC to HiFi, start a loopback task in HiFi, to looback audio*/
    pcm_io_st io_arg;
    io_arg.count = -1;
    xAIPC(pLoopbackCtx->aipc, MBX_CMD_IOBUF_DEMO, &io_arg, sizeof(io_arg));
    printf("Loopback task exit:%s\n", pLoopbackCtx->name);
    return NULL;
}

void* thread_aml_pcm_dsp_write(void * arg)
{
    pcm_io_st io_arg;
    dsp_writer_t* pWriterCtx = (dsp_writer_t*)arg;
    int sleepCnt = 0;
    AML_TBUF_AddConsumer(pWriterCtx->tbuf, pWriterCtx->eType);
    while(!pWriterCtx->bExit) {
        size_t szAvail = 0;
        void* phy = NULL;
        void* vir = NULL;
        /*Try to feed audio from T buffer to dsp align with chunk*/
        while (1) {
            AML_TBUF_GetFullness(pWriterCtx->tbuf, pWriterCtx->eType, &szAvail);
            if (szAvail < CHUNK_BYTES) {
                if (pWriterCtx->bExit)
                    goto exit_aml_pcm_dsp_write;
                usleep(1000);
                sleepCnt++;
                if (sleepCnt > 1000) {
                    printf("T buffer underrun %s :%zu %d %d\n",
                         pWriterCtx->name, szAvail, pWriterCtx->strmPos, pWriterCtx->chunkNum);
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
        memset(&io_arg, 0, sizeof(io_arg));
        io_arg.data = (xpointer)phy;
        io_arg.count = szAvail;
        xAIPC(pWriterCtx->aipc, MBX_CMD_IOBUF_ARM2DSP, &io_arg, sizeof(io_arg));
        AML_TBUF_UpdateRdOffset(pWriterCtx->tbuf, pWriterCtx->eType, szAvail);
        pWriterCtx->strmPos += io_arg.count;
        pWriterCtx->chunkNum++;
        AM_PCM_DEBUG(" %s: %d %d %d\n", pWriterCtx->name,  pWriterCtx->strmPos, szAvail, io_arg.count);
    }
exit_aml_pcm_dsp_write:
    /*Close loopback task*/
    io_arg.data = (xpointer)NULL;
    io_arg.count = 0;
    xAIPC(pWriterCtx->aipc, MBX_CMD_IOBUF_ARM2DSP, &io_arg, sizeof(io_arg));
    AML_TBUF_RemoveConsumer(pWriterCtx->tbuf, pWriterCtx->eType);
    printf("Exit write thread %s:%d\n", pWriterCtx->name, pWriterCtx->chunkNum);
    return NULL;
}

int aml_pcm_alsa_init(AML_ALSA_T** ppAlsa, unsigned int flags,
                            struct pcm_config *config)
{
    AMX_UNUSED(flags);
    AML_ALSA_T* pAlsa = NULL;

    pAlsa = malloc(sizeof(AML_ALSA_T));
    if (!pAlsa) {
        printf("Failed to allocate memory for AML_ALSA_T structure\n");
        goto recycle_of_aml_pcm_alsa_init;
    }
    memset(pAlsa, 0, sizeof(AML_ALSA_T));

    pAlsa->tbuf = AML_TBUF_Create(TBUF_SIZE);
    if (!pAlsa->tbuf) {
        printf("Failed to create tbuf\n");
        goto recycle_of_aml_pcm_alsa_init;
    }
    pAlsa->capture.tbuf = pAlsa->tbuf;
    pAlsa->capture.ppcm = pcm_open(0, 8, PCM_IN, config);
    if (!pAlsa->capture.ppcm) {
        printf("Failed to create pcm handle\n");
        goto recycle_of_aml_pcm_alsa_init;
    }
    printf("Open tinyalsa handle:%p\n", pAlsa->capture.ppcm);
    pthread_create(&pAlsa->capture.hThread, NULL, thread_aml_pcm_capture, (void*)&pAlsa->capture);
    *ppAlsa = pAlsa;

    return 0;
recycle_of_aml_pcm_alsa_init:
    if (pAlsa) {
        if (pAlsa->capture.ppcm)
            pcm_close(pAlsa->capture.ppcm);
        if (pAlsa->tbuf)
            AML_TBUF_Destroy(pAlsa->tbuf);
        free(pAlsa);
    }
    return -1;
}

void aml_pcm_alsa_deinit(AML_ALSA_T** ppAlsa)
{
    AML_ALSA_T* pAlsa = *ppAlsa;
    if (!pAlsa)
        return;
    if (pAlsa->isAUsed == false && pAlsa->isBUsed == false) {
        pAlsa->capture.bExit = 1;
        pthread_join(pAlsa->capture.hThread, NULL);
        pcm_close(pAlsa->capture.ppcm);
        AML_TBUF_Destroy(pAlsa->tbuf);
        free(pAlsa);
        *ppAlsa = NULL;
    }
}

AML_PCM_T* aml_pcm_instance_init(unsigned int card, AML_ALSA_T* pAlsa)
{
    AML_PCM_T* pPcm = NULL;
    pPcm = malloc(sizeof(AML_PCM_T));
    if (!pPcm) {
        printf("Failed to allocate memory for AML_PCM_T structure\n");
        goto recycle_of_aml_pcm_instance_init;
    }
    memset(pPcm, 0, sizeof(AML_PCM_T));

    pPcm->writer.aipc = xAudio_Ipc_Init(card);
    if (pPcm->writer.aipc == -1) {
        printf("Failed to init write thread ipc handle\n");
        goto recycle_of_aml_pcm_instance_init;
    }
    pPcm->writer.eType = (card == 0)?TBUF_READER_A:TBUF_READER_B;
    pPcm->writer.tbuf = pAlsa->tbuf;
    snprintf(pPcm->writer.name, sizeof(pPcm->writer.name), (card == 0)?"WriterA":"WriterB");
    if ( 0 != pthread_create(&pPcm->writer.hThread, NULL, thread_aml_pcm_dsp_write, (void*)&pPcm->writer)) {
        printf("Failed to create write thread\n");
        goto recycle_of_aml_pcm_instance_init;
    }

    pPcm->loopback.aipc = xAudio_Ipc_Init(card);
    if (pPcm->loopback.aipc == -1) {
        printf("Failed to init loopback thread ipc handle\n");
        goto recycle_of_aml_pcm_instance_init;
    }

    snprintf(pPcm->loopback.name, sizeof(pPcm->loopback.name), (card == 0)?"LoopbackA":"LoopbackB");
    if ( 0 != pthread_create(&pPcm->loopback.hThread, NULL, thread_aml_pcm_dsp_loopback, (void*)&pPcm->loopback)) {
        printf("Failed to create loopback thread\n");
        goto recycle_of_aml_pcm_instance_init;
    }

    snprintf(pPcm->reader.name, sizeof(pPcm->reader.name), (card == 0)?"ReaderA":"ReaderB");
    pPcm->reader.hShm = AML_MEM_Allocate(CHUNK_BYTES);
    if (!pPcm->reader.hShm) {
        printf("Failed to allocate inter buffer for read api\n");
        goto recycle_of_aml_pcm_instance_init;
    }
    pPcm->reader.phy = AML_MEM_GetPhyAddr(pPcm->reader.hShm);
    pPcm->reader.vir = AML_MEM_GetVirtAddr(pPcm->reader.hShm);
    pPcm->reader.aipc = xAudio_Ipc_Init(card);
    if (pPcm->reader.aipc == -1) {
        printf("Failed to init ipc handler for read api");
        goto recycle_of_aml_pcm_instance_init;
    }

    pPcm->pAlsa = pAlsa;
    return pPcm;
recycle_of_aml_pcm_instance_init:
    if (pPcm) {
        if (pPcm->writer.hThread) {
            pPcm->writer.bExit = 1;
            pthread_join(pPcm->writer.hThread, NULL);
        }
        if (pPcm->writer.aipc != -1)
            xAudio_Ipc_Deinit(pPcm->writer.aipc);
        if (pPcm->loopback.hThread) {
            printf("Wait loopback thread exit:%s\n", pPcm->loopback.name);
            pthread_join(pPcm->loopback.hThread, NULL);
            printf("Loopback thread exit done:%s\n", pPcm->loopback.name);
        }
        if (pPcm->loopback.aipc != -1)
            xAudio_Ipc_Deinit(pPcm->loopback.aipc);
        if (!pPcm->reader.hShm)
            AML_MEM_Free(pPcm->reader.hShm);
        if (pPcm->loopback.aipc != -1)
            xAudio_Ipc_Deinit(pPcm->reader.aipc);
        free(pPcm);
    }
    return NULL;
}

void aml_pcm_instance_deinit(AML_PCM_T* pPcm)
{
    printf("aml_pcm_instance_deinit:%s\n", pPcm->reader.name);
    pPcm->writer.bExit = 1;
    pthread_join(pPcm->writer.hThread, NULL);
    xAudio_Ipc_Deinit(pPcm->writer.aipc);

    pthread_join(pPcm->loopback.hThread, NULL);
    xAudio_Ipc_Deinit(pPcm->loopback.aipc);

    AML_MEM_Free(pPcm->reader.hShm);
    xAudio_Ipc_Deinit(pPcm->reader.aipc);

    if (pPcm->writer.eType == TBUF_READER_A)
        pPcm->pAlsa->isAUsed = false;
    else if (pPcm->writer.eType == TBUF_READER_B)
        pPcm->pAlsa->isBUsed = false;
    printf("aml_pcm_instance_deinit done:%s\n", pPcm->reader.name);
    free(pPcm);
}

static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
AML_PCM_HANDLE AML_PCM_Open(unsigned int card, unsigned int device, unsigned int flags,
                                        const struct aml_pcm_config *config)
{
    AML_PCM_T* pPcm = NULL;
    AMX_UNUSED(device);
    AMX_UNUSED(config);
    if (card != 0 && card !=1) {
        printf("Invalid Card:%d\n", card);
        goto end_of_aml_pcm_open;
    }

    pthread_mutex_lock(&gMutex);
    if (pgAlsa == NULL) {
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
        if (0 != aml_pcm_alsa_init(&pgAlsa, flags, &cfg)) {
            printf("Initial global context fail:%d", card);
            goto end_of_aml_pcm_open;
        }
    }

    if (card == 0 && pgAlsa->isAUsed) {
        printf("card-%d is alread used\n", card);
        goto end_of_aml_pcm_open;
    } else if (card == 1 && pgAlsa->isBUsed) {
        printf("card-%d is alread used\n", card);
        goto end_of_aml_pcm_open;
    } else {
        pPcm = aml_pcm_instance_init(card, pgAlsa);
        if (!pPcm) {
            printf("Initial pcm instance fail\n");
            aml_pcm_alsa_deinit(&pgAlsa);
            goto end_of_aml_pcm_open;
        }
    }

    pthread_mutex_unlock(&gMutex);
end_of_aml_pcm_open:
    return pPcm;
}

void AML_PCM_Close(AML_PCM_HANDLE Handle)
{
    pthread_mutex_lock(&gMutex);
    aml_pcm_instance_deinit((AML_PCM_T*)Handle);
    aml_pcm_alsa_deinit(&pgAlsa);
    pthread_mutex_unlock(&gMutex);
}

int AML_PCM_Read(AML_PCM_HANDLE Handle, void *data, unsigned int count)
{
    AML_PCM_T* pPcm = (AML_PCM_T*)Handle;
    dsp_reader_t* pReaderCtx = &pPcm->reader;
    uint8_t* p = data;

    /*IPC to HiFi, block here till HiFi write audio to the buffer*/
    int remained = count;
    while(remained) {
        pcm_io_st io_arg;
        memset(&io_arg, 0, sizeof(io_arg));
        io_arg.data = (xpointer)pReaderCtx->phy;
        io_arg.count = AMX_MIN(remained, CHUNK_BYTES);
        xAIPC(pReaderCtx->aipc, MBX_CMD_IOBUF_DSP2ARM, &io_arg, sizeof(io_arg));
        AML_MEM_Invalidate(pReaderCtx->phy, io_arg.count);
        memcpy(p, pReaderCtx->vir, io_arg.count);
        p += io_arg.count;
        remained -= io_arg.count;
        pReaderCtx->strmPos += io_arg.count;
    }
    return count;
}
