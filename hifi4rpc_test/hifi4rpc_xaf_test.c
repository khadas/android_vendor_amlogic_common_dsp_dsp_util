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

#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "asoundlib.h"
#include "generic_macro.h"

long get_us() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
    long long us =  tp.tv_sec * 1000 * 1000 + tp.tv_nsec / (1000);
    // printf("cur us=%lld\n", us);
    return us;
}

int bcm_client_write(int aipchdl, const void *data, unsigned int count) {
    pcm_io_st arg;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)NULL;
    arg.data = (xpointer)data;
    arg.count = count;
    arg.out_ret = 0;
    xAIPC(aipchdl, MBX_CMD_IOBUF_ARM2DSP, &arg, sizeof(arg));
    return arg.out_ret;
}

int bcm_client_read(int aipchdl, const void *data, unsigned int count) {
    pcm_io_st arg;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)NULL;
    arg.data = (xpointer)data;
    arg.count = count;
    arg.out_ret = 0;
    xAIPC(aipchdl, MBX_CMD_IOBUF_DSP2ARM, &arg, sizeof(arg));
    return arg.out_ret;
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
    int hdl = xAudio_Ipc_Init(id);
    AML_MEM_HANDLE hShmBuf;

    FILE *fileplay = fopen(argv[1], "rb");
    if (fileplay == NULL) {
        printf("failed to open played pcm file\n");
        return -1;
    }
    const int ms = 16;
    const int oneshot = 48 * ms; // 48KHz
    uint32_t size = oneshot * 16 * 4; // 16channel, 32bit
    uint32_t r;
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    int loop = 128 * 10; // test 10.24s
    long d0, d1, e;
    for (i = 0; i != loop; i++) {
        d0 = get_us();
        r = fread(buf, 1, size, fileplay);
        if (r != size) {
            rewind(fileplay);
            r = fread(buf, 1, size, fileplay);
        }
        AML_MEM_Clean(phybuf, r);
        bcm_client_write(hdl, phybuf, r);
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
    AML_MEM_Free(hShmBuf);
    fclose(fileplay);
    xAudio_Ipc_Deinit(hdl);
    return 0;
}

// !!! depend on libtinyalsa, only link in Android.mk !!!
int bcm_pcm_test(int argc, char* argv[])
{
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
    int hdl = xAudio_Ipc_Init(id);
    AML_MEM_HANDLE hShmBuf;

    struct pcm_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channels = 16;
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
    const int ms = 16;
    const int oneshot = 48 * ms; // 48KHz
    uint32_t size = oneshot * 16 * 4; // 16channel, 32bit
    uint32_t r;
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    int loop = 128 * 10;
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
        AML_MEM_Clean(phybuf, size);
        bcm_client_write(hdl, phybuf, size);
        //printf("%dms pcm_write pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //   ms, p, buf, oneshot, fr);
    }
    printf("i=%d loop=%d\n", i, loop);
    AML_MEM_Free(hShmBuf);
    pcm_close(pcm);
    xAudio_Ipc_Deinit(hdl);
    return 0;
}

#define SAMPLE_MS 48
#define SAMPLE_BYTES 4
#define SAMPLE_CH 16
#define CHUNK_MS 16
#define CHUNK_BYTES (SAMPLE_MS * SAMPLE_BYTES * SAMPLE_CH * CHUNK_MS)
typedef struct _io_thread_context_ {
    int id;
    FILE *fp;
    size_t fileSize;
} io_thread_context;
static void *thread_write_pcm(void *arg)
{

    struct pcm_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channels = 16;
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
        return NULL;
    }
    if (!pcm_is_ready(pcm)) {
        printf("pcm is not ready:%s\n", pcm_get_error(pcm));
        return NULL;
    }

    void *pVir = NULL;
    void *pPhy = NULL;
    io_thread_context *pWriteCtx = (io_thread_context *)arg;
    AML_MEM_HANDLE hShm = AML_MEM_Allocate(CHUNK_BYTES);
    int aipc = xAudio_Ipc_Init(pWriteCtx->id);
    pVir = AML_MEM_GetVirtAddr(hShm);
    pPhy = AML_MEM_GetPhyAddr(hShm);
    int bExit = 0;
    size_t cnt = 0;
    while (!bExit) {
        int nRead = 0;
        nRead = pcm_read(pcm, (void *)pVir, CHUNK_BYTES);
        if (nRead != 0) {
            printf("pcm read fail:%s\n", pcm_get_error(pcm));
            bExit = 1;
            continue;
        }
        AML_MEM_Clean(pPhy, CHUNK_BYTES);
        pcm_io_st io_arg;
        io_arg.data = (xpointer)pPhy;
        io_arg.count = CHUNK_BYTES;
        xAIPC(aipc, MBX_CMD_IOBUF_ARM2DSP, &io_arg, sizeof(io_arg));
        cnt += CHUNK_BYTES;
        if (cnt >= pWriteCtx->fileSize) {
            break;
        }
    }
    printf("Write thread exit\n");
    xAudio_Ipc_Deinit(aipc);
    AML_MEM_Free(hShm);
    pcm_close(pcm);

    return NULL;
}

static void *thread_read_pcm(void *arg)
{
    void *pVir = NULL;
    void *pPhy = NULL;
    io_thread_context *pReadCtx = (io_thread_context *)arg;
    AML_MEM_HANDLE hShm = AML_MEM_Allocate(CHUNK_BYTES);
    int aipc = xAudio_Ipc_Init(pReadCtx->id);
    pVir = AML_MEM_GetVirtAddr(hShm);
    pPhy = AML_MEM_GetPhyAddr(hShm);
    while (pReadCtx->fileSize) {
        pcm_io_st io_arg;
        io_arg.data = (xpointer)pPhy;
        io_arg.count = CHUNK_BYTES;
        xAIPC(aipc, MBX_CMD_IOBUF_DSP2ARM, &io_arg, sizeof(io_arg));
        AML_MEM_Invalidate(pPhy, io_arg.count);

        fwrite((void *)pVir, 1, io_arg.count, pReadCtx->fp);
        pReadCtx->fileSize -= io_arg.count;
    }
    printf("Read thread exit\n");
    xAudio_Ipc_Deinit(aipc);
    AML_MEM_Free(hShm);
    return NULL;
}

int pcm_loopback_test(int argc, char *argv[])
{
    int aipc = -1;
    io_thread_context writeCtx;
    io_thread_context readCtx;
    if (argc != 3) {
        printf("Invalid parameter\n");
        return -1;
    }
    int id = atoi(argv[0]);
    int chunkNum = atoi(argv[1]);

    FILE *fpOut = fopen(argv[2], "w+b");
    if (fpOut == NULL) {
        printf("Failed to open %s\n", argv[2]);
        fclose(fpOut);
        return -1;
    }

    int fileSize = CHUNK_BYTES * chunkNum;
    printf("chunk=%d num=%d size=%d\n", CHUNK_BYTES, chunkNum, fileSize);

    writeCtx.fileSize = fileSize;
    writeCtx.id = id;

    readCtx.fileSize = fileSize;
    readCtx.fp = fpOut;
    readCtx.id = id;

    pthread_t writeThread;
    pthread_t readThread;
    pthread_create(&writeThread, NULL, thread_write_pcm, (void *)&writeCtx);
    pthread_create(&readThread, NULL, thread_read_pcm, (void *)&readCtx);

    aipc = xAudio_Ipc_Init(id);
    pcm_io_st io_arg;
    io_arg.count = fileSize;
    xAIPC(aipc, MBX_CMD_IOBUF_DEMO, &io_arg, sizeof(io_arg));
    xAudio_Ipc_Deinit(aipc);
    printf("HiFi4 loopback done\n");
    pthread_join(writeThread, NULL);
    pthread_join(readThread, NULL);

    fclose(fpOut);
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
    int hdl = xAudio_Ipc_Init(id);
    FILE* fpOut = fopen(argv[1], "wb");
    const int ms = 16;
    const int oneshot = 48 * ms; // 48KHz
    uint32_t size = oneshot * 16 * 4; // 16channel, 32bit
    AML_MEM_HANDLE hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    int loop = 4; // test 10.24s
    int32_t remained = size*loop;
    while (remained > 0) {
        uint32_t r = AMX_MIN(size, remained);
        bcm_client_read(hdl, phybuf, r);
        AML_MEM_Invalidate(phybuf, r);
        fwrite(buf, 1, r, fpOut);
        remained -= r;
        printf("recv data remained=%d\n", remained);
    }
    AML_MEM_Free(hShmBuf);
    fclose(fpOut);
    xAudio_Ipc_Deinit(hdl);
    return 0;
}

