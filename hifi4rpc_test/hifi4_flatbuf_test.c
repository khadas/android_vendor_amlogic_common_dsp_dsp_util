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
 * hifi4 flat buffers test
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "aml_flatbuf_api.h"
#define UNUSED(x) (void)(x)

char* strRdSamples = "flat buffer api test, this is test samples FlatBufFifoDsp2Arm";
void* flat_buffers_read(void* arg)
{
    UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;
    AML_MEM_HANDLE hShm = 0;

    int strRdSamplesLen = strlen(strRdSamples) + 1;
    char* recBuf = (char*)malloc(strRdSamplesLen);

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));

    /*
     * Pre-allocate a block of shared memory as internal CC buffer.
     * The config is ignored if id FlatBufFifoDsp2Arm already exists.
     */
    config.size = strRdSamplesLen/2;
    hShm = AML_MEM_Allocate(config.size);
    config.phy_addr = AML_MEM_GetPhyAddr(hShm);

    /*
     * Config to work at block mode
     */
    hFbuf = AML_FLATBUF_Create("FlatBufFifoDsp2Arm", FLATBUF_FLAG_RD|FLATBUF_FLAG_BLOCK, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    int i = 0;
    while (strRdSamplesLen > 0) {
        /*
         * read 2 chars every time
         */
        int size = 2;
        /*
         * flat buffers works in unblock mode, partial write
         */
        size = AML_FLATBUF_Read(hFbuf, &recBuf[i], size);
        //printf("Arm read %d char:%c %c\n", size, recBuf[i], recBuf[i+1]);
        strRdSamplesLen -= size;
        i += size;
    }

    if (!strcmp(strRdSamples, recBuf)) {
        printf("arm_flat_buffers_read success\n");
    } else {
        printf("arm_flat_buffers_read fails\n");
    }
exit_capture:
    if (recBuf)
        free(recBuf);
    if (hShm)
        AML_MEM_Free(hShm);
    if (hFbuf)
        AML_FLATBUF_Destory(hFbuf);
    return NULL;
}


char* strWrSamples = "flat buffer api test, this is test samples FlatBufFifoArm2Dsp";
void* flat_buffers_write(void* arg)
{
    UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;
    AML_MEM_HANDLE hShm = 0;

    int strWrSamplesLen = strlen(strWrSamples) + 1;
    char* sendBuf = strWrSamples;

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));

    /*
     * Pre-allocate a block of shared memory as internal CC buffer.
     * The config is ignored if id FlatBufFifoArm2Dsp already exists.
     */
    config.size = strWrSamplesLen/2;
    hShm = AML_MEM_Allocate(config.size);
    config.phy_addr = AML_MEM_GetPhyAddr(hShm);

    /*
     * Config to work at block mode
     */
    hFbuf = AML_FLATBUF_Create("FlatBufFifoArm2Dsp", FLATBUF_FLAG_WR|FLATBUF_FLAG_BLOCK, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    int i = 0;
    while (strWrSamplesLen > 0) {
        /*
         * write 2 chars every time
         */
        int size = 2;
        /*
         * flat buffers works in unblock mode, partial write
         */
        //printf("Arm Write %d char\n", size);
        size = AML_FLATBUF_Write(hFbuf, &sendBuf[i], size);
        strWrSamplesLen -= size;
        i += size;
    }
exit_capture:
    if (hShm)
        AML_MEM_Free(hShm);
    if (hFbuf)
        AML_FLATBUF_Destory(hFbuf);
    return NULL;
}

int flat_buf_unit_test()
{
    pthread_t wr_thread;
    pthread_t rd_thread;

    int handle = xAudio_Ipc_init();
    xAIPC(handle, MBX_CMD_FLATBUF_ARM2DSP, NULL, 0);
    xAIPC(handle, MBX_CMD_FLATBUF_DSP2ARM, NULL, 0);
    xAudio_Ipc_Deinit(handle);

    pthread_create(&wr_thread, NULL, flat_buffers_write, NULL);
    pthread_create(&rd_thread, NULL, flat_buffers_read, NULL);
    pthread_join(wr_thread,NULL);
    pthread_join(rd_thread,NULL);

    return 0;
}


