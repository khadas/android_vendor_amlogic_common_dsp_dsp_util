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
#include <signal.h>
#include "aipc_type.h"
#include "rpc_client_cbuf.h"
#include "rpc_client_shm.h"

#define UNUSED(x) (void)(x)

#define DUMP_LENTH (16000)
#define CCBUF_LENTH (DUMP_LENTH*4)
int32_t bDumpExit = 0;
void audio_dump_sighandler(int signum)
{
    UNUSED(signum);
    bDumpExit = 1;
}

int audio_dump(int argc, char* argv[]) {
    int ret = 0;
    AML_CBUF_HANDLE hCbuf = NULL;
    FILE *faudio_dump = NULL;
    int32_t totalSize = 0;
    char* pbuf = NULL;
    if (argc != 3) {
        printf("Invalid parameter number\n");
        return -1;
    }
    signal(SIGTERM, &audio_dump_sighandler);
    signal(SIGINT, &audio_dump_sighandler);
    pbuf = malloc(DUMP_LENTH);
    totalSize = atoi(argv[2]);

    faudio_dump = fopen(argv[1], "w+b");
    if ( !faudio_dump) {
        printf("Can not create dump file:%p\n", faudio_dump);
        ret = -1;
        goto end_tab;
    }

    hCbuf = AML_CBUF_Create(atoi(argv[0]), CCBUF_LENTH, CCBUF_LENTH);
    if (!hCbuf) {
        printf("Can not create CC buffer\n");
        ret = -1;
        goto end_tab;
    }

    while(!bDumpExit && totalSize >= 0) {
        size_t nbyteRead = DUMP_LENTH;
        nbyteRead = AML_CBUF_Read(hCbuf, pbuf,nbyteRead);
        if (nbyteRead > 0) {
            fwrite(pbuf, 1, nbyteRead, faudio_dump);
            totalSize -= nbyteRead;
        }
        usleep(1000);
    }
end_tab:
    if (pbuf) {
        free(pbuf);
    }
    if (hCbuf)
        AML_CBUF_Destory(hCbuf);

    if (faudio_dump)
        fclose(faudio_dump);

    return ret;
}

static void usage()
{
    printf ("media-dump Usage: hifi4_media_tool --media-dump $id $file $size\n");
    printf ("\n");
}


int main(int argc, char* argv[]) {
    int c = -1;
    int option_index = 0;
    struct option long_options[] =
    {
        {"help", no_argument, NULL, 0},
        {"media-dump", no_argument, NULL, 1},
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
            if (3 == argc - optind){
                audio_dump(argc - optind, &argv[optind]);
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


