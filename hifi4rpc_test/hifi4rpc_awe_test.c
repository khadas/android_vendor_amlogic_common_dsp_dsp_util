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
 * hifi4 rpc client api sample codes, awe
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
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "rpc_client_pcm.h"
#include "aml_wakeup_api.h"
#include "aml_audio_util.h"
#include "generic_macro.h"

#define VOICE_CHUNK_LEN_MS 20
#define AWE_SAMPLE_RATE 16000
#define AWE_SAMPLE_BYTE 2
#define VOICE_CHUNK_LEN_BYTE (AWE_SAMPLE_BYTE*AWE_SAMPLE_RATE*VOICE_CHUNK_LEN_MS/1000)
static uint32_t uFeedChunk = 0;
static uint32_t uRecvChunk = 0;
static uint32_t uTotalBytesRead = 0;
static uint32_t uTotalBytesWrite = 0;

void aml_wake_engine_asr_data_handler(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data)
{
    AMX_UNUSED(awe);
    FILE* fout = (FILE*)user_data;
    if (AWE_DATA_TYPE_ASR == type) {
        fwrite(out, 1, size, fout);
        uRecvChunk++;
        uTotalBytesWrite += size;
    }
}

void aml_wake_engine_voip_data_handler(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data)
{
    AMX_UNUSED(awe);
    FILE* fout = (FILE*)user_data;
    if (AWE_DATA_TYPE_VOIP == type) {
        fwrite(out, 1, size, fout);
    }
}

void aml_wake_engine_event_handler(AWE *awe, const AWE_EVENT_TYPE type, int32_t code,
                                             const void *payload, void *user_data)
{
    AMX_UNUSED(awe);
    AMX_UNUSED(code);
    AMX_UNUSED(payload);
    AMX_UNUSED(user_data);
    if (type == AWE_EVENT_TYPE_WAKE)
        printf("wake word detected !!!! \n");
}

static AWE *gAwe = NULL;
void awe_test_sighandler(int signum)
{
    AMX_UNUSED(signum);
    if (gAwe)
        AML_AWE_Close(gAwe);
    if (gAwe)
        AML_AWE_Destroy(gAwe);
    gAwe = NULL;
    uFeedChunk = 0;
}

int aml_wake_engine_unit_test(int argc, char* argv[]) {
    uFeedChunk = 0;
    uRecvChunk = 0;
    uTotalBytesRead = 0;
    uTotalBytesWrite = 0;
    int syncMode = 1;
    AWE_PARA awe_para;
    int ret = 0;
    uint32_t isWakeUp = 0;
    AWE_RET awe_ret = AWE_RET_OK;
    int32_t inLen = 0, outLen = 0;
    void *in[4], *out[2];
    int nMic = 2;
    int nRef = 2;

    void *vir_mic0_buf = NULL;
    void *vir_mic1_buf = NULL;
    void *vir_ref0_buf = NULL;
    void *vir_ref1_buf = NULL;
    void *vir_interleave_buf = NULL;
    void *vir_out0_buf = NULL;
    void *vir_out1_buf = NULL;

    signal(SIGINT, &awe_test_sighandler);
    if (argc == 7)
        syncMode = atoi(argv[6]);

    FILE *fmic0 = fopen(argv[0], "rb");
    FILE *fmic1 = fopen(argv[1], "rb");
    FILE *fref0 = fopen(argv[2], "rb");
    FILE *fref1 = fopen(argv[3], "rb");
    FILE *fout0 = fopen(argv[4], "w+b");
    FILE *fout1 = fopen(argv[5], "w+b");
    if ( !fmic0 || !fmic1|| !fref0 || !fout0 || !fout1) {
        printf("Can not open io file:%p %p %p %p %p %p\n",
        fmic0, fmic1, fref0, fref1, fout0, fout1);
        ret = -1;
        goto end_tab;
    }

    vir_mic0_buf = malloc(VOICE_CHUNK_LEN_BYTE);
    vir_mic1_buf = malloc(VOICE_CHUNK_LEN_BYTE);
    vir_ref0_buf = malloc(VOICE_CHUNK_LEN_BYTE);
    vir_ref1_buf = malloc(VOICE_CHUNK_LEN_BYTE);
    vir_out0_buf = malloc(2048);
    vir_out1_buf = malloc(2048);
    if (!vir_mic0_buf || !vir_mic1_buf
        || !vir_ref0_buf || !vir_ref1_buf
        || !vir_out0_buf || !vir_out1_buf) {
        printf("Can not allocate buffer:%p %p %p %p %p %p\n",
            vir_mic0_buf, vir_mic1_buf, vir_ref0_buf,
            vir_ref1_buf, vir_out0_buf, vir_out1_buf);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_Create(&gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Can not create Hifi AWE service\n");
        ret = -1;
        goto end_tab;
    }

    if (syncMode == 1) {
        AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_ASR, aml_wake_engine_asr_data_handler,(void *)fout0);
        AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_VOIP, aml_wake_engine_voip_data_handler,(void *)fout1);
        AML_AWE_AddEventHandler(gAwe, AWE_EVENT_TYPE_WAKE, aml_wake_engine_event_handler, NULL);
    }

    awe_para.inputMode = AWE_USER_INPUT_MODE;
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_INPUT_MODE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set input mode fail:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_RATES, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Get supported samperate fail:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampRate = awe_para.supportSampRates[0];
    printf("awe_para.inSampRate:%d\n", awe_para.inSampRate);
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_RATE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set samperate fail:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to get sample bits:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampBits = awe_para.supportSampBits[0];
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to set sample bits:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_REF_IN_CHANNELS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to get sample bits:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }
    nRef = awe_para.refInChannels;
    vir_interleave_buf = malloc((nMic + nRef)*VOICE_CHUNK_LEN_BYTE);

    awe_ret = AML_AWE_Open(gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to open AWE:%d\n", awe_ret);
        ret = -1;
        goto end_tab;
    }
    printf("wake test start! !\n");
    uint32_t i;
    while (1) {
        uint32_t nbyteRead = VOICE_CHUNK_LEN_BYTE;
        uint32_t nbyteMic0 = 0;
        uint32_t nbyteMic1 = 0;
        uint32_t nbyteRef0 = 0;
        uint32_t nbyteRef1 = 0;
        nbyteMic0 = fread(vir_mic0_buf, 1, nbyteRead, fmic0);
        nbyteMic1 = fread(vir_mic1_buf, 1, nbyteRead, fmic1);
        nbyteRef0 = fread(vir_ref0_buf, 1, nbyteRead, fref0);
        nbyteRef1 = fread(vir_ref1_buf, 1, nbyteRead, fref1);
        if (nbyteMic0 < nbyteRead || nbyteMic1 < nbyteRead ||
            nbyteRef0 < nbyteRead || nbyteRef1 < nbyteRead) {
            printf("EOF\n");
            break;
        }

        if (syncMode == 0) {
            in[0] = vir_mic0_buf;
            in[1] = vir_mic1_buf;
            in[2] = vir_ref0_buf;
            in[3] = vir_ref1_buf;
            inLen = nbyteRead;
            out[0] = vir_out0_buf;
            out[1] = vir_out1_buf;
            outLen = 2048;
            AML_AWE_Process(gAwe, in, &inLen, out, &outLen, &isWakeUp);
            if (isWakeUp) {
                printf("wake word detected ! \n");
            }
            if (!inLen) {
                fseek(fmic0, -((long)inLen), SEEK_CUR);
                fseek(fmic1, -((long)inLen), SEEK_CUR);
                fseek(fref0, -((long)inLen), SEEK_CUR);
                fseek(fref1, -((long)inLen), SEEK_CUR);
            }
            fwrite(vir_out0_buf, 1, outLen, fout0);
            fwrite(vir_out1_buf, 1, outLen, fout1);
            uTotalBytesWrite += outLen;
            uTotalBytesRead += nbyteRead;
        } else if (syncMode == 1) {
            /*interleave mic0,mic1,ref0*/
            short* pMic0 = (short*)vir_mic0_buf;
            short* pMic1 = (short*)vir_mic1_buf;
            short* pRef0 = (short*)vir_ref0_buf;
            short* pRef1 = (short*)vir_ref1_buf;
            short* pInterleave = (short*)vir_interleave_buf;
            for (i = 0; i < (nbyteRead/2); i++) {
                *pInterleave = *pMic0++;
                pInterleave++;
                *pInterleave = *pMic1++;
                pInterleave++;
                *pInterleave = *pRef0++;
                pInterleave++; 
                if (nRef == 2) {
                    *pInterleave = *pRef1++;
                    pInterleave++;
                }
            }
            ret = AML_AWE_PushBuf(gAwe, (const char*)vir_interleave_buf, nbyteRead*(nMic + nRef));
            if (AWE_RET_ERR_NO_MEM == ret) {
                fseek(fmic0, -((long)nbyteRead), SEEK_CUR);
                fseek(fmic1, -((long)nbyteRead), SEEK_CUR);
                fseek(fref0, -((long)nbyteRead), SEEK_CUR);
                fseek(fref1, -((long)nbyteRead), SEEK_CUR);
                usleep(500);
            }
            else if (ret != AWE_RET_OK) {
                printf("Unknown error when execute AML_AWE_PushBuf, ret=%d\n", ret);
                break;
            } else {
                uFeedChunk++;
                uTotalBytesRead += nbyteRead;
            }
        } else {
            printf("Invalid sync mode:%d\n", syncMode);
            break;
        }
    }

    end_tab:
    while(uRecvChunk < uFeedChunk) {
        printf("uRecvChunk:%d, uFeedChunk:%d\n", uRecvChunk, uFeedChunk);
        usleep(5000);
    }
    printf("Read: %d Kbytes, Write %d Kbytes\n", uTotalBytesRead/1024, uTotalBytesWrite/1024);
    if (gAwe)
        AML_AWE_Close(gAwe);
    if (gAwe)
        AML_AWE_Destroy(gAwe);

    if (vir_interleave_buf) {
        free(vir_interleave_buf);
    }

    if (vir_mic0_buf) {
        free(vir_mic0_buf);
    }

    if (vir_mic1_buf) {
        free(vir_mic1_buf);
    }

    if (vir_ref0_buf) {
        free(vir_ref0_buf);
    }

    if (vir_ref1_buf) {
        free(vir_ref1_buf);
    }

    if (vir_out0_buf) {
        free(vir_out0_buf);
    }

    if (vir_out1_buf) {
        free(vir_out1_buf);
    }

    if (fmic0)
        fclose(fmic0);
    if (fmic1)
        fclose(fmic1);
    if (fref0)
        fclose(fref0);
    if (fref1)
        fclose(fref1);
    if (fout0)
        fclose(fout0);
    if (fout1)
        fclose(fout1);
    return ret;
}

#define VOIP_DATA_FIFO "/data/voip_data_fifo"
#define VOIP_CMD_FIFO "/data/voip_cmd_fifo"
#define VOIP_CMD_DATA_INVALID 0
#define VOIP_CMD_DATA_EOF 0xffffffff
#define VOIP_CMD_DATA_AVAIL 1

void aml_wake_engine_voip_data_receiver(char* strVoip)
{
    FILE* fout_voip = NULL;
    int fifo_reader_fd = -1;
    int fifo_cmdrev_fd = -1;
    uint32_t nread = 0;
    uint32_t cmd = 0;
    int nReadFail = 0;
    void* pReadBuf = NULL;
    void* pWriteBuf = NULL;
    void* hsrc = NULL;

    fifo_reader_fd = open(VOIP_DATA_FIFO ,O_RDONLY|O_NONBLOCK,0);
    if(fifo_reader_fd < 0)
    {
        printf("Voip data fifo open failed in receiver side\n");
        goto receiver_end;
    }

    fifo_cmdrev_fd = open(VOIP_CMD_FIFO ,O_RDONLY,0);
    if(fifo_cmdrev_fd < 0)
    {
        printf("Voip command fifo open failed in receiver side\n");
        goto receiver_end;
    }

    fout_voip = fopen(strVoip, "w+b");
    if (fout_voip == NULL) {
        printf("Voip dump file open failed in receiver side\n");
        goto receiver_end;
    }
    hsrc = AML_SRCS16LE_Init(16000, 8000, 1);
    pReadBuf = malloc(VOICE_CHUNK_LEN_BYTE);
    pWriteBuf = malloc(VOICE_CHUNK_LEN_BYTE/2);
    while(1) {
        read(fifo_cmdrev_fd, &cmd, sizeof(cmd));
        if (cmd == VOIP_CMD_DATA_EOF) {
            printf("Exit voip receiver process\n");
            break;
        } else if (cmd == VOIP_CMD_DATA_AVAIL) {
            nread = read(fifo_reader_fd, pReadBuf, VOICE_CHUNK_LEN_BYTE);
            if (nread != VOICE_CHUNK_LEN_BYTE) {
                nReadFail++;
                if (nReadFail > 50) {
                    printf("Too much data fifo read error, exit voip receiver process\n");
                    break;
                }
                continue;
            }
            nReadFail = 0;
            AML_SRCS16LE_Exec(hsrc, (int16_t*)pWriteBuf, VOICE_CHUNK_LEN_BYTE/(2*sizeof(int16_t)), (int16_t*)pReadBuf, VOICE_CHUNK_LEN_BYTE/sizeof(int16_t));
            fwrite(pWriteBuf, 1, VOICE_CHUNK_LEN_BYTE/2, fout_voip);
            usleep(20*1000);
        } else {
            printf("Invalid voip command, exit voip receiver process\n");
            break;
        }
    }
    AML_SRCS16LE_DeInit(hsrc);
    free(pWriteBuf);
    free(pReadBuf);

receiver_end:
    if (fout_voip)
        fclose(fout_voip);
    if (fifo_reader_fd > 0)
        close(fifo_reader_fd);
}

void aml_wake_engine_voip_data_transfer(AWE *awe, const AWE_DATA_TYPE type,
                                            char* out, size_t size, void *user_data)
{
    AMX_UNUSED(awe);
    int* pfifo = (int*)user_data;
    int fifo_writer_fd = pfifo[0];
    int fifo_cmdsend_fd = pfifo[1];
    if (AWE_DATA_TYPE_VOIP == type) {
        uint32_t cmd = VOIP_CMD_DATA_AVAIL;
        write(fifo_writer_fd, out, size);
        write(fifo_cmdsend_fd, &cmd, sizeof(cmd));
    }
}

int aml_wake_engine_dspin_test(int argc, char* argv[]) {
    AMX_UNUSED(argc);
    AWE_PARA awe_para;
    int ret = 0;
    AWE_RET awe_ret = AWE_RET_OK;
    pid_t pid = -1;
    FILE *fout_asr = NULL;
    int fifo_writer_fd = -1;
    int fifo_cmdsend_fd = -1;
    int fifo_array[2] = {0};
    uint32_t cmd = VOIP_CMD_DATA_INVALID;

    if((mkfifo(VOIP_DATA_FIFO ,O_CREAT|O_EXCL|O_RDWR)<0)&&(errno!=EEXIST)) {
        printf("Can not create voip data fifo\n");
        goto end_tab;
    }

    if((mkfifo(VOIP_CMD_FIFO ,O_CREAT|O_EXCL|O_RDWR)<0)&&(errno!=EEXIST)) {
        printf("Can not create voip command fifo\n");
        goto end_tab;
    }

    /* fork a child process */
    pid = fork();
    if (pid < 0) { /* error occurred */\
        printf("Fork Failed");
        return 1;
    }
    else if (pid == 0) { /* child process */
        printf("Start voip receiver process, pid=%d\n", getpid());
        sleep(1);
        aml_wake_engine_voip_data_receiver(argv[1]);
        return 0;
    }
    else { /* parent process */
        printf("Continue parent process\n");
    }

    signal(SIGINT, &awe_test_sighandler);
    fifo_writer_fd = open(VOIP_DATA_FIFO, O_RDWR | O_NONBLOCK, 0);
    if (fifo_writer_fd < 0) {
        printf("Can not open voip data fifo writer fd:%d\n", fifo_writer_fd);
        goto end_tab;
    }

    fifo_cmdsend_fd = open(VOIP_CMD_FIFO, O_RDWR | O_NONBLOCK, 0);
    if (fifo_cmdsend_fd < 0) {
        printf("Can not open voip command fifo writer fd:%d\n", fifo_writer_fd);
        goto end_tab;
    }
    fifo_array[0] = fifo_writer_fd;
    fifo_array[1] = fifo_cmdsend_fd;

    fout_asr = fopen(argv[0], "w+b");
    if (!fout_asr) {
        printf("Can not open asr output file:%p\n", fout_asr);
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_Create(&gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Can not create Hifi AWE service\n");
        ret = -1;
        goto end_tab;
    }

    AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_ASR, aml_wake_engine_asr_data_handler,(void *)fout_asr);
    AML_AWE_AddDataHandler(gAwe, AWE_DATA_TYPE_VOIP, aml_wake_engine_voip_data_transfer,(void *)fifo_array);
    AML_AWE_AddEventHandler(gAwe, AWE_EVENT_TYPE_WAKE, aml_wake_engine_event_handler, NULL);

    awe_para.inputMode = AWE_DSP_INPUT_MODE;
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_INPUT_MODE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set input mode fail\n");
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_RATES, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Get supported sample rate fail\n");
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampRate = awe_para.supportSampRates[0];
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_RATE, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Set sample rate fail\n");
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_GetParam(gAwe, AWE_PARA_SUPPORT_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to get sample bits\n");
        ret = -1;
        goto end_tab;
    }

    awe_para.inSampBits = awe_para.supportSampBits[0];
    awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_IN_SAMPLE_BITS, &awe_para);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to set sample bits\n");
        ret = -1;
        goto end_tab;
    }

    awe_ret = AML_AWE_Open(gAwe);
    if (awe_ret != AWE_RET_OK) {
        printf("Failed to open AWE\n");
        ret = -1;
        goto end_tab;
    }
    printf("wake test start in dsp input mode\n");
    char user_cmd[16];
    while (1) {
        printf("Command Guide:\n");
        printf("suspend - suspend and resume from voice wake up\n");
        printf("exit - quit voice capture\n");
        scanf("%s", user_cmd);
        if(!strcmp(user_cmd, "suspend")) {
            system("amixer cset numid=7 1");
            system("amixer cset numid=21 1");
            system("arecord -Dhw:0,2 -c 2 -r 16000 -f S32_LE /tmp/aa.wav &");
            sleep(1);
            printf("Start awe freeRun mode\n");
            awe_para.freeRunMode = 1;
            awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_FREE_RUN_MODE, &awe_para);
            system("echo "" > /sys/kernel/config/usb_gadget/amlogic/UDC");
            system("echo mem > /sys/power/state");
            awe_para.freeRunMode = 0;
            awe_ret = AML_AWE_SetParam(gAwe, AWE_PARA_FREE_RUN_MODE, &awe_para);
            printf("Stop awe freeRun mode\n");
        } else if(!strcmp(user_cmd, "exit")) {
            break;
        } else {
            printf("Invalid command!\n");
        }
    }

    cmd = VOIP_CMD_DATA_EOF;
    write(fifo_cmdsend_fd, &cmd, sizeof(cmd));

end_tab:
    if (gAwe)
        AML_AWE_Close(gAwe);
    if (gAwe)
        AML_AWE_Destroy(gAwe);

    if (fout_asr)
        fclose(fout_asr);

    if (fifo_writer_fd > 0)
        close(fifo_writer_fd);

    unlink(VOIP_CMD_FIFO);
    unlink(VOIP_DATA_FIFO);
    return ret;
}

