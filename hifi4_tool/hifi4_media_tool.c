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
 * hifi4 media tool
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "aipc_type.h"
#include "aml_flatbuf_api.h"
#include "rpc_client_aipc.h"

struct audio_dump_context {
    int32_t ch;
    int32_t rate;
    int32_t bytesSample;
    int32_t ms;
    int bExit;
    FILE* fdump;
    AML_FLATBUF_HANDLE hFbuf;
};

void* thread_dump(void* arg) {
    struct audio_dump_context* pContext = (struct audio_dump_context*)arg;
    size_t chunkSize = (pContext->ch*pContext->rate*pContext->bytesSample*20)/1000;//20 ms
    char* pbuf = malloc(chunkSize);

    while(!pContext->bExit) {
        size_t nbytesRead;
        nbytesRead = AML_FLATBUF_Read(pContext->hFbuf, pbuf,chunkSize, 0);
        fwrite(pbuf, 1, nbytesRead, pContext->fdump);
        usleep(10*1000);
    }

    free(pbuf);
    return NULL;
}

int audio_dump(int argc, char* argv[])
{
    pthread_t dump_thread;
    struct audio_dump_context context;
    memset(&context, 0, sizeof(context));

    if (argc != 5) {
        printf("Invalid parameter number:%d\n", argc);
        return -1;
    }

    context.fdump = fopen(argv[0], "w+b");
    if (!context.fdump) {
        printf("Can not create dump file:%p\n", context.fdump);
        goto end_tab;
    }
    context.ch = atoi(argv[1]);
    context.rate = atoi(argv[2]);
    context.bytesSample = atoi(argv[3]);

    struct flatbuffer_config config;
    config.size = (context.ch*context.rate*context.bytesSample*50)/1000;//50 ms buffer
    context.hFbuf = AML_FLATBUF_Create(argv[4], FLATBUF_FLAG_RD, &config);
    if (!context.hFbuf) {
        printf("Can not create flat file:%p\n", context.hFbuf);
        goto end_tab;
    }

    pthread_create(&dump_thread, NULL, thread_dump, (void*)&context);
    while(1) {
        char user_cmd[16];
        printf("Execute 'quit' to exit\n");
        scanf("%s", user_cmd);
        if(!strcmp(user_cmd, "quit")) {
            context.bExit = 1;
            break;
        }
    }
    pthread_join(dump_thread,NULL);

end_tab:
    if (context.hFbuf)
        AML_FLATBUF_Destroy(context.hFbuf);
    if (context.fdump)
        fclose(context.fdump);
    return 0;
}

int reg_dump(int argc, char* argv[]) {
    aml_hifi4reg_dump_st reg_dump;
    if (argc != 2) {
        printf("Invalid parameter number\n");
        return -1;
    }
    reg_dump.reg_addr = strtoul(argv[0], 0, 0);
    reg_dump.size = strtoul(argv[1], 0, 0);
    printf("dump reg:0x%x, size:0x%x\n", reg_dump.reg_addr, reg_dump.size);
    int arpchdl = xAudio_Ipc_init();
    xAIPC(arpchdl, MBX_CMD_REG_DUMP, (void*)&reg_dump, sizeof(reg_dump));
    xAudio_Ipc_Deinit(arpchdl);
    return 0;
}


static void usage()
{
    printf ("media-dump Usage: hifi4_media_tool --audio-dump $file $ch $rate $bytesSample $str_id\n");
    printf ("reg-dump Usage: hifi4_media_tool --reg-dump $addr $size\n");
    printf ("\n");
}


int main(int argc, char* argv[]) {
    int c = -1;
    int option_index = 0;
    struct option long_options[] =
    {
        {"help", no_argument, NULL, 0},
        {"audio-dump", no_argument, NULL, 1},
        {"reg-dump", no_argument, NULL, 2},
        {0, 0, 0, 0}
    };
    c = getopt_long (argc, argv, "hvV", long_options, &option_index);
    if(-1 == c) {
        printf("error options\n");
    }
    switch(c)
    {
        case 0:
            usage();
            break;
        case 1:
            if (5 == argc - optind){
                audio_dump(argc - optind, &argv[optind]);
            } else {
                usage();
                exit(1);
            }
            break;
        case 2:
            if (2 == argc - optind){
                reg_dump(argc - optind, &argv[optind]);
            } else {
                usage();
                exit(1);
            }
            break;
        case '?':
            usage();
            exit(1);
            break;
        }
    return 0;
}


