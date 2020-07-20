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
 * hifi4 basic test
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
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "aml_audio_util.h"
#include "generic_macro.h"

#define VOICE_CHUNK_LEN_MS 20
#define IPC_UNIT_TEST_REPEAT 50
int ipc_uint_test(int id) {
    unsigned int i;
    int num_repeat = IPC_UNIT_TEST_REPEAT;
    const uint8_t send_samples[16] = {
        0x1, 0xf, 0x2, 0xe, 0x3, 0xd, 0x4, 0xc, 0x5, 0xb, 0x6, 0xa, 0x7, 0x9, 0x0, 0x8
    };
    const uint8_t recv_samples[16] = {
        0x11, 0xf1, 0x21, 0x1e, 0x13, 0x1d, 0x14, 0xc2, 0x52, 0xb2, 0x62, 0x2a, 0x27, 0x29, 0x20, 0x38
    };
    char ipc_data[16];
    int arpchdl = xAudio_Ipc_Init(id);
    while(num_repeat--) {
        memcpy(ipc_data, send_samples, sizeof(ipc_data));
        xAIPC(arpchdl, MBX_CMD_IPC_TEST, (void*)ipc_data, sizeof(ipc_data));
        for (i = 0; i < sizeof(recv_samples); i++) {
            if(recv_samples[i] != ipc_data[i])
            break;
        }
        if (i < sizeof(recv_samples)) {
            printf("arm ack: ipc unit test fail:%d, ipc data:\n", IPC_UNIT_TEST_REPEAT - num_repeat);
            for(i = 0; i < sizeof(recv_samples); i++)
                printf("0x%x ", ipc_data[i]);
            printf("\n");
            break;
        }
    }
    if(num_repeat <= 0)
        printf("ipc unit test pass, repeat: %d\n", IPC_UNIT_TEST_REPEAT);
    xAudio_Ipc_Deinit(arpchdl);
    return 0;
}


typedef struct {
    char element[64];
    int32_t sum;
} __attribute__((packed)) ipc_data_st;


int ipc_uint_test1(int id) {
    ipc_data_st arg;
    int arpchdl = xAudio_Ipc_Init(id);
    int repeat = 1000;
    while(repeat--) {
        memset(&arg, 0xff, sizeof(arg));
        arg.sum = 0;
        {
            int i = 0;
            for (i = 0; i < 64; i++)
                arg.sum += arg.element[i];
        }
        xAIPC(arpchdl, MBX_CMD_IPC_TEST1, &arg, sizeof(arg));
    }

    xAudio_Ipc_Deinit(arpchdl);
    return 0;
}


#define RPC_FUNC_DUMMY 0x1
#define RPC_FUNC_SQUARE 0x2
int rpc_unit_test(int argc, char* argv[]) {
    pid_t pid = -1;
    tAmlDummyRpc dummy_rpc_param;
    int ret = 0;
    int arpchdl = 0;
    if (argc != 3) {
        printf("invalid param number:%d\n", argc);
        return -1;
    }
    dummy_rpc_param.func_code = atoi(argv[0]);
    dummy_rpc_param.input_param = atoi(argv[1]);
    dummy_rpc_param.task_sleep_ms = atoi(argv[2]);

    /* fork a child process */
    pid = fork();
    if (pid < 0) { /* error occurred */\
        printf("Fork Failed");
        return 1;
    }
    else if (pid == 0) { /* child process */
        dummy_rpc_param.task_id = getpid();
        dummy_rpc_param.func_code = (dummy_rpc_param.func_code == RPC_FUNC_DUMMY)?RPC_FUNC_SQUARE:RPC_FUNC_DUMMY;
        dummy_rpc_param.task_sleep_ms += 100;
        printf("task id:%d command issued\n", dummy_rpc_param.task_id);
        arpchdl = xAudio_Ipc_init();
        xAIPC(arpchdl, MBX_CMD_RPC_TEST, &dummy_rpc_param, sizeof(dummy_rpc_param));
        xAudio_Ipc_Deinit(arpchdl);
        if (dummy_rpc_param.func_code == RPC_FUNC_SQUARE
            && (dummy_rpc_param.input_param*dummy_rpc_param.input_param) != dummy_rpc_param.output_param) {
            ret = -1;
        }
        printf("RPC_Unit_Test_%s, func code:%d, input param:%d, output param:%d, task id:%d, sleep: %d ms\n",
                (ret==0)?"SUCCESS":"FAIL",
        dummy_rpc_param.func_code, dummy_rpc_param.input_param,
        dummy_rpc_param.output_param, dummy_rpc_param.task_id,
        dummy_rpc_param.task_sleep_ms);
        return 0;
    }
    else { /* parent process */
        printf("Continue parent process\n");
    }

    dummy_rpc_param.task_id = getpid();
    usleep(20*1000);
    printf("task id:%d command issued\n", dummy_rpc_param.task_id);
    arpchdl = xAudio_Ipc_init();
    xAIPC(arpchdl, MBX_CMD_RPC_TEST, &dummy_rpc_param, sizeof(dummy_rpc_param));
    xAudio_Ipc_Deinit(arpchdl);
    if (dummy_rpc_param.func_code == RPC_FUNC_SQUARE
        && (dummy_rpc_param.input_param*dummy_rpc_param.input_param) != dummy_rpc_param.output_param) {
        ret = -1;
    }
    printf("RPC_Unit_Test_%s, func code:%d, input param:%d, output param:%d, task id:%d, sleep: %d ms\n",
            (ret==0)?"SUCCESS":"FAIL",
    dummy_rpc_param.func_code, dummy_rpc_param.input_param,
    dummy_rpc_param.output_param, dummy_rpc_param.task_id,
    dummy_rpc_param.task_sleep_ms);
    return 0;
}

#define SHM_UNIT_TEST_REPEAT 50
int shm_uint_test(void)
{
    int num_repeat = SHM_UNIT_TEST_REPEAT;
    char samples[16] = {0};
    void* pVirSrc = NULL;
    void* pVirDst = NULL;
    while(num_repeat--) {
        AML_MEM_HANDLE hDst, hSrc;
        hDst = AML_MEM_Allocate(sizeof(samples));
        hSrc = AML_MEM_Allocate(sizeof(samples));
        memset((void*)samples, num_repeat, sizeof(samples));
        pVirSrc = AML_MEM_GetVirtAddr(hSrc);
        memcpy((void*)pVirSrc, samples, sizeof(samples));
        AML_MEM_Clean(hSrc, sizeof(samples));
        AML_MEM_Transfer(hDst, hSrc, sizeof(samples));
        AML_MEM_Invalidate(hDst, sizeof(samples));
        pVirDst = AML_MEM_GetVirtAddr(hDst);
        if (memcmp((void*)pVirDst, samples, sizeof(samples))) {
            printf("shm unit test fail, repeat:%d\n",
            SHM_UNIT_TEST_REPEAT - num_repeat);
            break;
        } else {
            /*char* k = (char*)pVirDst;
            for(i = 0; i < sizeof(samples); i++)
            printf("0x%x ", k[i]);
            printf("\n");*/
        }
        AML_MEM_Free(hSrc);
        AML_MEM_Free(hDst);
    }
    if(num_repeat <= 0)
        printf("shm unit test pass, repeat %d\n", SHM_UNIT_TEST_REPEAT);

    num_repeat = SHM_UNIT_TEST_REPEAT;
    while(num_repeat--) {
        AML_MEM_HANDLE hShm = AML_MEM_Allocate(sizeof(samples));
        printf("Allocate shm:%p\n", hShm);
    }
    sleep(1);
    AML_MEM_Recycle((int)getpid());

    return 0;
}

#define CHUNK_SAMPLES 1024
void aml_s16leresampler(int argc, char* argv[])
{
    FILE* fin = NULL;
    FILE* fout = NULL;
    int inRate, outRate;
    int32_t inLen, outLen;
    int16_t* inBuf;
    int16_t* outBuf;
    void* hsrc;
    int nread;
    AMX_UNUSED(argc);

    inRate = atoi(argv[0]);
    outRate = atoi(argv[1]);
    fin = fopen(argv[2], "rb");
    if (fin == NULL) {
        printf("input file open failed\n");
        goto resampler_end;
    }

    fout = fopen(argv[3], "w+b");
    if (fout == NULL) {
        printf("Output file open failed\n");
        goto resampler_end;
    }
    hsrc = AML_SRCS16LE_Init(inRate, outRate, 1);

    inLen = sizeof(int16_t)*CHUNK_SAMPLES;
    outLen = (inLen*outRate)/inRate;
    inBuf = (int16_t*)malloc(inLen);
    outBuf = (int16_t*)malloc(outLen);
    while(1) {
        nread = fread(inBuf, 1, inLen, fin);
        if (nread != inLen) {
            printf("EOF\n");
            break;
        } else{
            AML_SRCS16LE_Exec(hsrc, outBuf, outLen/sizeof(int16_t), inBuf, inLen/sizeof(int16_t));
            fwrite(outBuf, 1, outLen, fout);
        }
    }
    free(inBuf);
    free(outBuf);
    AML_SRCS16LE_DeInit(hsrc);

resampler_end:
    if (fout)
        fclose(fout);
    if (fin)
        fclose(fin);
}

void aml_hifi4_inside_wakeup()
{
    int handle = xAudio_Ipc_init();
    xAIPC(handle, MBX_WAKE_ENGINE_DEMO, NULL, 0);
    xAudio_Ipc_Deinit(handle);
}


void aml_hifi4_timer_wakeup()
{
    int handle = xAudio_Ipc_init();
    xAIPC(handle, MBX_CMD_TIMER_WAKEUP, NULL, 0);
    xAudio_Ipc_Deinit(handle);
    system("echo mem > /sys/power/state");
    printf("Hifi4 Wake Up from SW Timer\n");
}

int rpc_meminfo(int id) {
    int h = xAudio_Ipc_Init(id);
    printf("id=%d h=%d meminfo\n", id, h);
    int r = xAIPC(h, MBX_CMD_MEMINFO, NULL, 0);
    printf("id=%d h=%d meminfo r=%d\n", id, h, r);
    xAudio_Ipc_Deinit(h);
    return 0;
}
