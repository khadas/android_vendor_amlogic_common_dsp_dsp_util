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
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "rpc_client_pcm.h"


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

int pcm_play_buildin()
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

int pcm_play_test(int argc, char* argv[])
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
        printf("Invalid parameter number, argc=%d\n", argc);
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

long get_us() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
    long long us =  tp.tv_sec * 1000 * 1000 + tp.tv_nsec / (1000);
    // printf("cur us=%lld\n", us);
    return us;
}

int bcm_file_test(int argc, char* argv[])
{
    printf("all arg begin\n");
    int i;
    for (i = 0; i != argc; i++) {
        printf("arg[%d]=%s\n", i, argv[i]);
    }
    printf("all arg end\n");
    int hdl = xAudio_Ipc_init();
    AML_MEM_HANDLE hShmBuf;

    FILE *fileplay = fopen("/data/out_lb.wav.raw", "rb");
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
    for (i = 0; i != argc; i++) {
        printf("arg[%d]=%s\n", i, argv[i]);
    }
#if 1
    int hdl = xAudio_Ipc_init();
    AML_MEM_HANDLE hShmBuf;

    struct pcm_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channels = 16;
    cfg.rate = 48000;
    cfg.period_size = 256;
    cfg.period_count = 4;
    // !!! linux's TINYALSA side's header file's, PCM_FORMAT_S32_LE is 1
    // !!! DSP's TINYALSA side's headefile, PCM_FORMAT_S32_LE is 7
    cfg.format = 1;
    cfg.start_threshold = 0;
    cfg.stop_threshold = 0;
    cfg.silence_threshold = 0;
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
    if (!pcm_is_ready(pcm)) {
        printf("pcm isn't ready\n");
        return -2;
    }
    const int ms = 16;
    const int oneshot = 48 * ms; // 48KHz
    uint32_t size = oneshot * 16 * 4; // 16channel, 32bit
    uint32_t r;
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    int loop = 128 * 10;
    for (i = 0; i != loop; i++) {
        r = pcm_read(pcm, buf, size);
        if (r != size) {
            printf("expect size=%u, but get r=%u, quit\n", size, r);
            break;
        }
        AML_MEM_Clean(phybuf, r);
        bcm_client_write(hdl, phybuf, r);
        //printf("%dms pcm_write pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
        //   ms, p, buf, oneshot, fr);
    }
    printf("i=%d loop=%d\n", i, loop);
    AML_MEM_Free(hShmBuf);
    pcm_close(pcm);
    xAudio_Ipc_Deinit(hdl);
#endif
    return 0;
}

#include "aipc_type.h"
int xaf_test(int argc, char **argv) {
    int h = xAudio_Ipc_init();
    xAIPC(h, MBX_CMD_XAF_TEST, NULL, 0);
    xAudio_Ipc_Deinit(h);
    return 0;
}

#define PCM_CAPTURE_SAMPLES (48000*20)
int pcm_capture_test(int argc, char* argv[])
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

