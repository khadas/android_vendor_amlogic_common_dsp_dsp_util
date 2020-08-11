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
    const int ms = 36;
    const int oneshot = 48 * ms; // 1728 samples
    rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
    pconfig->channels = 2;
    pconfig->rate = 48000;
    pconfig->format = PCM_FORMAT_S32_LE;
    pconfig->period_size = oneshot;
    pconfig->period_count = 8;
    pconfig->start_threshold = oneshot*4;
    pconfig->silence_threshold = oneshot * 2;
    pconfig->stop_threshold = oneshot * 2;
    tAmlPcmhdl p = pcm_client_open(0, DEVICE_TDMOUT_B, PCM_OUT, pconfig);
    AML_MEM_HANDLE hShmBuf;

    uint8_t *play_data = (uint8_t *)audio_play_data;
    int in_fr = pcm_client_bytes_to_frame(p, audio_play_data_len);
    int i, fr = 0;
    uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    for (i = 0; i + oneshot <= in_fr; i += fr) {
        memcpy(buf, play_data + pcm_client_frame_to_bytes(p, i), size);
        AML_MEM_Clean(phybuf, size);
        fr = pcm_client_writei(p, phybuf, oneshot);
        //printf("%dms pcm_write i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
          //ms, i, p, buf, oneshot, fr);
    }
    pcm_client_close(p);
    AML_MEM_Free(hShmBuf);
    free(pconfig);
    return 0;
}

int pcm_play_test(int argc, char *argv[])
{
    const int ms = 36;
    const int oneshot = 48 * ms; // 1728 samples
    rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
    pconfig->channels = 2;
    pconfig->rate = 48000;
    pconfig->format = PCM_FORMAT_S32_LE;
    pconfig->period_size = oneshot;
    pconfig->period_count = 8;
    pconfig->start_threshold = oneshot*4;
    pconfig->silence_threshold = oneshot * 2;
    pconfig->stop_threshold = oneshot * 2;
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

int pcm_capture_test(int argc, char *argv[])
{
    if (argc != 7 && argc != 8) {
        printf("Invalide param number:%d\n", argc);
        return -1;
    }
    int seconds = atoi(argv[0]);
    int chunkMs = atoi(argv[1]);
    int chn = atoi(argv[2]);
    int rate = atoi(argv[3]);
    int format = atoi(argv[4]);
    if (format != 0 && format != 1) {
        printf("Not supported format:%d\n", format);
        return -1;
    }
    int device = atoi(argv[5]);
    if (device != 1 && device != 3 && device != 4) {
        printf("Not supported device:%d\n", device);
        return -1;
    }
    FILE *filecap = fopen(argv[6], "w+b");
    if (filecap == NULL) {
        printf("failed to open captured pcm file\n");
        return -1;
    }
    int flags = PCM_IN;
    if (argc >= 8) {
        if (strcasecmp(argv[7], "block") == 0) {
            // default to block
        } else if (strcasecmp(argv[7], "nonblock") == 0) {
            flags |= PCM_NONBLOCK;
        } else {
            printf("fail to parse option: [block,nonblock]\n");
            return -1;
        }
    }

    rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
    pconfig->channels = chn;
    pconfig->rate = rate;
    pconfig->format = (format == 0) ? PCM_FORMAT_S32_LE : PCM_FORMAT_S16_LE;
    pconfig->period_size = chunkMs * (rate/1000);
    pconfig->period_count = 4;
    pconfig->start_threshold = 1024;
    pconfig->silence_threshold = 1024 * 2;
    pconfig->stop_threshold = 1024 * 2;
    tAmlPcmhdl p = pcm_client_open(0, device, flags, pconfig);
    AML_MEM_HANDLE hShmBuf;

    int in_fr = rate * seconds;
    int i, fr = 0;
    const int oneshot = chunkMs * (rate/1000);
    uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
    hShmBuf = AML_MEM_Allocate(size);
    void *buf = AML_MEM_GetVirtAddr(hShmBuf);
    void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
    for (i = 0; i + oneshot <= in_fr; i += fr) {
        fr = pcm_client_readi(p, phybuf, oneshot);
        AML_MEM_Invalidate(phybuf, size);
        fwrite(buf, sizeof(char), size, filecap);
        //printf("i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n", i, p, buf, oneshot, fr);
    }
    pcm_client_close(p);
    AML_MEM_Free(hShmBuf);
    free(pconfig);
    fclose(filecap);
    return 0;
}
