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
#define FLAT_TEST_TYPE_UNIT 0
#define FLAT_TEST_TYPE_THROUGHPUT 1

char* strRdSamples = "flat buffer api test, this is test samples FlatBufFifoDsp2Arm";
void* flat_buffers_read_string(void* arg)
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
    hFbuf = AML_FLATBUF_Create("FlatBufFifoDsp2Arm", FLATBUF_FLAG_RD | FLATBUF_FLAG_BLOCK, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    int i = 0;
    while (strRdSamplesLen > 0) {
        /*
         * read 2 chars every time, read size should not be larger than config.size
         */
        int size = 2;
        /*
         * flat buffers works in block mode, the call return only when all bytes are read
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
void* flat_buffers_write_string(void* arg)
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
    hFbuf = AML_FLATBUF_Create("FlatBufFifoArm2Dsp", FLATBUF_FLAG_WR | FLATBUF_FLAG_BLOCK, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    int i = 0;
    while (strWrSamplesLen > 0) {
        /*
         * write 2 chars every time, write size should not be larger than config.size
         */
        int size = 2;
        /*
         * flat buffers works in block mode, the call return only when all bytes are writen
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

#define FLAT_TEST_SAMPLE_RATE 16000
#define FLAT_TEST_PEROID_SEC 60
#define FLAT_TEST_CH_NUM 3
#define FLAT_TEST_SMAPLE_BYTE 2
void* flat_buffers_read_throughput(void* arg)
{
    UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;

    int32_t rdLen = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PEROID_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SMAPLE_BYTE;

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));

    /*
     * Config internal CC buffer size.
     */
    config.size = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_CH_NUM*FLAT_TEST_SMAPLE_BYTE;
    config.phy_addr = 0;

    /*
     * Config to work at block mode
     */
    hFbuf = AML_FLATBUF_Create("FlatBufFifoDsp2Arm", FLATBUF_FLAG_RD, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    void* recBuf = malloc(config.size);
    while (rdLen > 160) {
        /*
         * read size should not be larger than config.size
         */
        int size = config.size/2;
        /*
         * flat buffers works in block mode, the call return only when all bytes are read
         */
        size = AML_FLATBUF_Read(hFbuf, recBuf, size);
        rdLen -= size;        
        //printf("Arm_Read_len_%d_size_%d\n", rdLen, size);
        usleep(1);
    }

    printf("arm_flat_buffers_read_finish_rdLen:%d\n",
        FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PEROID_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SMAPLE_BYTE);
exit_capture:
    if (recBuf)
        free(recBuf);
    if (hFbuf)
        AML_FLATBUF_Destory(hFbuf);
    return NULL;
}

void* flat_buffers_write_throughput(void* arg)
{
    UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;

    int32_t wrLen = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PEROID_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SMAPLE_BYTE;

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));

    /*
     * Config internal CC buffer size.
     */
    config.size = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_CH_NUM*FLAT_TEST_SMAPLE_BYTE;
    config.phy_addr = 0;

    /*
     * Config to work at block mode
     */
    hFbuf = AML_FLATBUF_Create("FlatBufFifoArm2Dsp", FLATBUF_FLAG_WR, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    void* sendBuf = malloc(config.size);
    while (wrLen > 160) {
        /*
         * write size should not be larger than config.size
         */
        int size = config.size/2;
        /*
         * flat buffers works in block mode, the calls return only when all bytes are writen
         */
        size = AML_FLATBUF_Write(hFbuf, sendBuf, size);
        wrLen -= size;
        //printf("Arm_Write_len_%d_size_%d\n", wrLen, size);
        usleep(1);
    }
    
    printf("arm_flat_buffers_write_finish_wrLen:%d\n",
        FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PEROID_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SMAPLE_BYTE);
exit_capture:
    if (sendBuf)
        free(sendBuf);
    if (hFbuf)
        AML_FLATBUF_Destory(hFbuf);
    return NULL;
}


int flat_buf_test(int argc, char* argv[])
{
    UNUSED(argc);
    pthread_t wr_thread;
    pthread_t rd_thread;
    int32_t test_cmd = atoi(argv[0]);

    int handle = xAudio_Ipc_init();
    xAIPC(handle, MBX_CMD_FLATBUF_ARM2DSP, &test_cmd, sizeof(int32_t));
    xAIPC(handle, MBX_CMD_FLATBUF_DSP2ARM, &test_cmd, sizeof(int32_t));
    xAudio_Ipc_Deinit(handle);

    if (test_cmd == FLAT_TEST_TYPE_UNIT) {
        pthread_create(&wr_thread, NULL, flat_buffers_write_string, NULL);
        pthread_create(&rd_thread, NULL, flat_buffers_read_string, NULL);
    } else if (test_cmd == FLAT_TEST_TYPE_THROUGHPUT) {
        pthread_create(&wr_thread, NULL, flat_buffers_write_throughput, NULL);
        pthread_create(&rd_thread, NULL, flat_buffers_read_throughput, NULL);
    } else {
        printf("Invalid test type\n");
        return -1;
    }
    pthread_join(wr_thread,NULL);
    pthread_join(rd_thread,NULL);

    return 0;
}


