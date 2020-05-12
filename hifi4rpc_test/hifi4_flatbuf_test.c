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
#include "generic_macro.h"

#define FLAT_TEST_TYPE_UNIT 0
#define FLAT_TEST_TYPE_THROUGHPUT 1

char* strRdSamples = "flat buffer api test, this is test samples FlatBufCmdDsp2Arm";
void* flat_buffers_read_cmd(void* arg)
{
    AMX_UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;

    int strRdSamplesLen = strlen(strRdSamples) + 1;
    char* recBuf = (char*)malloc(strRdSamplesLen);

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));
    config.size = strRdSamplesLen/2;
    config.phy_ch = FLATBUF_CH_ARM2DSPA;

    hFbuf = AML_FLATBUF_Create("FlatBufCmdDsp2Arm", FLATBUF_FLAG_RD, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    int i = 0;
    srand(time(NULL));
    while (strRdSamplesLen > 0) {
        /*
         * read random number of chars every time
         */
        int size = rand() % config.size;
        size = AMX_MIN(size, strRdSamplesLen);
        /*
         * Read bytes with 1 ms time out
         */
        size = AML_FLATBUF_Read(hFbuf, &recBuf[i], size, 1);
        strRdSamplesLen -= size;
        i += size;
    }

    if (!strcmp(strRdSamples, recBuf)) {
        printf("arm_flat_buffers_read_cmd_success\n");
    } else {
        printf("arm_flat_buffers_read_cmd_fails\n");
    }
exit_capture:
    if (recBuf)
        free(recBuf);
    if (hFbuf)
        AML_FLATBUF_Destroy(hFbuf);
    return NULL;
}


char* strWrSamples = "flat buffer api test, this is test samples FlatBufCmdArm2Dsp";
void* flat_buffers_write_cmd(void* arg)
{
    AMX_UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;

    int strWrSamplesLen = strlen(strWrSamples) + 1;
    char* sendBuf = strWrSamples;

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));
    config.size = strWrSamplesLen/2;
    config.phy_ch = FLATBUF_CH_ARM2DSPA;

    hFbuf = AML_FLATBUF_Create("FlatBufCmdArm2Dsp", FLATBUF_FLAG_WR, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    int i = 0;
    srand(time(NULL));
    while (strWrSamplesLen > 0) {
        /*
         * write random number of chars every time
         */
        int size = rand() % config.size;
        size = AMX_MIN(size, strWrSamplesLen);
        /*
         * block here till all bytes are wrriten
         */
        size = AML_FLATBUF_Write(hFbuf, &sendBuf[i], size, -1);
        strWrSamplesLen -= size;
        i += size;
    }
exit_capture:
    if (hFbuf)
        AML_FLATBUF_Destroy(hFbuf);
    return NULL;
}

#define FLAT_TEST_SAMPLE_RATE 16000
#define FLAT_TEST_PERIOD_SEC 60
#define FLAT_TEST_CH_NUM 3
#define FLAT_TEST_SAMPLE_BYTE 2
void* flat_buffers_read_data(void* arg)
{
    AMX_UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;
    void* recBuf = NULL;

    int32_t rdLen = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PERIOD_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SAMPLE_BYTE;

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));

    /*
     * Config internal CC buffer size.
     */
    config.size = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_CH_NUM*FLAT_TEST_SAMPLE_BYTE;
    config.phy_ch = FLATBUF_CH_ARM2DSPA;

    hFbuf = AML_FLATBUF_Create("FlatBufDataDsp2Arm", FLATBUF_FLAG_RD, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    recBuf = malloc(config.size);
    while (rdLen > 0) {
        /*
         * read size should not be larger than config.size
         */
        int size = config.size/2;
        size = AMX_MIN(rdLen, size);
        /*
         * block here till all bytes are read
         */
        size = AML_FLATBUF_Read(hFbuf, recBuf, size, 1);
        rdLen -= size;
    }

    printf("arm_flat_buffers_read_data_finish_rdLen:%d\n",
        FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PERIOD_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SAMPLE_BYTE);
exit_capture:
    if (recBuf)
        free(recBuf);
    if (hFbuf)
        AML_FLATBUF_Destroy(hFbuf);
    return NULL;
}

void* flat_buffers_write_data(void* arg)
{
    AMX_UNUSED(arg);
    AML_FLATBUF_HANDLE hFbuf = NULL;
    void* sendBuf = NULL;

    int32_t wrLen = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_PERIOD_SEC*FLAT_TEST_CH_NUM*FLAT_TEST_SAMPLE_BYTE;

    struct flatbuffer_config config;
    memset(&config, 0, sizeof(config));

    /*
     * Config internal CC buffer size.
     */
    config.size = FLAT_TEST_SAMPLE_RATE*FLAT_TEST_CH_NUM*FLAT_TEST_SAMPLE_BYTE;
    config.phy_ch = FLATBUF_CH_ARM2DSPA;

    hFbuf = AML_FLATBUF_Create("FlatBufDataArm2Dsp", FLATBUF_FLAG_WR, &config);
    if (hFbuf == NULL) {
        printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
        goto exit_capture;
    }

    sendBuf = malloc(config.size);
    while (wrLen > 0) {
        /*
         * write size should not be larger than config.size
         */
        int size = config.size/2;
        size = AMX_MIN(wrLen, size);
        /*
         * block here till all bytes are written
         */
        size = AML_FLATBUF_Write(hFbuf, sendBuf, size, -1);
        wrLen -= size;
    }
exit_capture:
    if (sendBuf)
        free(sendBuf);
    if (hFbuf)
        AML_FLATBUF_Destroy(hFbuf);
    return NULL;
}


int flat_buf_test(int argc, char* argv[])
{
    AMX_UNUSED(argc);
    AMX_UNUSED(argv);

    pthread_t wr_thread_cmd;
    pthread_t rd_thread_cmd;
    pthread_t wr_thread_data;
    pthread_t rd_thread_data;

    int handle = xAudio_Ipc_init();
    uint32_t cmd = 0;

    if (argc == 0) {
        xAIPC(handle, MBX_CMD_FLATBUF_ARM2DSP, &cmd, sizeof(uint32_t));
        xAIPC(handle, MBX_CMD_FLATBUF_DSP2ARM, &cmd, sizeof(uint32_t));
        pthread_create(&wr_thread_cmd, NULL, flat_buffers_write_cmd, NULL);
        pthread_create(&rd_thread_cmd, NULL, flat_buffers_read_cmd, NULL);
        pthread_create(&wr_thread_data, NULL, flat_buffers_write_data, NULL);
        pthread_create(&rd_thread_data, NULL, flat_buffers_read_data, NULL);
        pthread_join(wr_thread_cmd,NULL);
        pthread_join(rd_thread_cmd,NULL);
        pthread_join(wr_thread_data,NULL);
        pthread_join(rd_thread_data,NULL);
    } else if (argc == 1){
        if (!strcmp(argv[0], "arm2dsp_write")) {
            pthread_create(&wr_thread_cmd, NULL, flat_buffers_write_cmd, NULL);
            pthread_join(wr_thread_cmd,NULL);
        } else if (!strcmp(argv[0], "arm2dsp_read")) {
            cmd = 1;
            xAIPC(handle, MBX_CMD_FLATBUF_ARM2DSP, &cmd, sizeof(uint32_t));
        } else if (!strcmp(argv[0], "dsp2arm_write")) {
            cmd = 1;
            xAIPC(handle, MBX_CMD_FLATBUF_DSP2ARM, &cmd, sizeof(uint32_t));
        } else if (!strcmp(argv[0], "dsp2arm_read")) {
            pthread_create(&rd_thread_cmd, NULL, flat_buffers_read_cmd, NULL);
            pthread_join(rd_thread_cmd,NULL);
        } else
            printf("Invalid argv: %s\n", argv[0]);
    } else {
        printf("Invalid argc=%d\n", argc);
    }
    xAudio_Ipc_Deinit(handle);
    return 0;
}


