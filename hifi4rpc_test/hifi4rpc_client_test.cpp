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
 * hifi4 rpc client api sample codes
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdint.h>
#include <fcntl.h>
#include <sys/time.h>
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

#define MSECS_PER_SEC (1000L)
#define NSECS_PER_MSEC (1000000L)

void aprofiler_get_cur_timestamp(struct timespec* ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
    return;
}

uint32_t aprofiler_msec_duration(struct timespec* tsEnd, struct timespec* tsStart)
{
    uint32_t uEndMSec = (uint32_t)(tsEnd->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsEnd->tv_nsec/NSECS_PER_MSEC);
    uint32_t uStartMSec = (uint32_t)(tsStart->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsStart->tv_nsec/NSECS_PER_MSEC);
	//printf("uEndMSec:%d, uStartMSec:%d\n", uEndMSec, uStartMSec);
	return (uEndMSec - uStartMSec);
}

#define TIC              \
    struct timespec bgn, end; \
    aprofiler_get_cur_timestamp(&bgn)

#define TOC                            \
    aprofiler_get_cur_timestamp(&end); \
    uint32_t ms = aprofiler_msec_duration(&end, &bgn)
int mp3_offload_dec(int argc, char* argv[]);
int aac_offload_dec(int argc, char* argv[]);

#ifdef __cplusplus
extern "C" {
#endif
int pcm_play_buildin(void);
int pcm_play_test(int argc, char* argv[]);
int pcm_capture_test(int argc, char* argv[]);
int offload_vsp_rsp(int argc, char* argv[]);
int aml_wake_engine_unit_test(int argc, char* argv[]);
int aml_wake_engine_dspin_test(int argc, char* argv[]);
int ipc_uint_test(void);
int ipc_uint_test1(void);

int rpc_unit_test(int argc, char* argv[]) ;
int shm_uint_test(void);
void aml_s16leresampler(int argc, char* argv[]);
void aml_hifi4_inside_wakeup();
void aml_hifi4_timer_wakeup();
int aml_rsp_unit_test(int argc, char* argv[]);
int flat_buf_test(int argc, char* argv[]);
#ifdef __cplusplus
}
#endif
static void usage()
{
    printf ("\033[1mipc unit test usage:\033[m hifi4rpc_client_test --ipc\n");

    printf ("\033[1mrpc unit test usage:\033[m hifi4rpc_client_test --rpc $func_code[1:dummy, 2:square] $input_param $sleep_time[ms]\n");

    printf ("\033[1mshared memory unit test usage:\033[m hifi4rpc_client_test --shm\n");

    printf ("\033[1mmp3dec Usage:\033[m hifi4rpc_client_test --mp3dec $input_file $output_file\n");

    printf ("\033[1maacdec Usage:\033[m hifi4rpc_client_test --aacdec $format $input_file $output_file\n");
    printf ("aac supported transport format: 0-RAW, 1-ADIF, 2-ADTS, 6-LATM_MCP1, 7-LATM_MCP0, 10-LOAS\n");

    printf ("\033[1mpcmplay Usage:\033[m hifi4rpc_client_test --pcmplay $pcm_file\n");

    printf ("\033[1mpcmcap Usage:\033[m hifi4rpc_client_test --pcmcap  $pcm_file $input_mode[1:tdmin,3:tdmin&loopback, 4:pdmin]\n");

    printf ("\033[1mpcmplay-buildin Usage:\033[m hifi4rpc_client_test --pcmplay-buildin\n");

    printf ("\033[1mhifi inside wakeup test Usage:\033[m hifi4rpc_client_test --hifiwake\n");

    printf ("\033[1mvsp-rsp Usage:\033[m hifi4rpc_client_test --vsp-rsp $input_file $output_file $in_rate $out_rate\n");

    printf ("\033[1mvsp-awe-unit Usage:\033[m hifi4rpc_client_test --vsp-awe-unit $mic0.pcm $mic1.pcm $ref.pcm $out_asr.pcm $out_voip.pcm\n");

    printf ("\033[1mvsp-awe-dspin Usage:\033[m hifi4rpc_client_test --vsp-awe-dspin $out_asr.pcm $out_voip.pcm\n");

    printf ("\033[1mresampler Usage:\033[m hifi4rpc_client_test --resampler $inRate $outRate $inFile $outFile\n");

    printf ("\033[1mtimerwakeup Usage:\033[m hifi4rpc_client_test --timerwakeup\n");

    printf ("\033[1mflatbuffer Usage:\033[m hifi4rpc_client_test --flatbuf-unit $test_type[0:unit test, 1:throughput test]\n");

    printf ("\033[1mipc1 unit test Usage:\033[m hifi4rpc_client_test --ipc1\n");
}


int main(int argc, char* argv[]) {
    int c = -1;
    int option_index = 0;
    struct option long_options[] =
    {
        {"help", no_argument, NULL, 0},
        {"ipc", no_argument, NULL, 1},
        {"shm", no_argument, NULL, 2},
        {"mp3dec", no_argument, NULL, 3},
        {"pcmplay", no_argument, NULL, 4},
        {"pcmcap", no_argument, NULL, 5},
        {"pcmplay-buildin", no_argument, NULL, 6},
        {"aacdec", no_argument, NULL, 7},
        {"vsp-rsp", no_argument, NULL, 8},
        {"rpc", no_argument, NULL, 9},
        {"vsp-awe-unit", no_argument, NULL, 10},
        {"vsp-awe-dspin", no_argument, NULL, 11},
        {"hifiwake", no_argument, NULL, 12},
        {"resampler", no_argument, NULL, 13},
        {"timerwakeup", no_argument, NULL, 14},
        {"flatbuf-unit", no_argument, NULL, 15},
        {"ipc1", no_argument, NULL, 16},
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
            {
                uint32_t msHighest = 50;
                TIC;
                ipc_uint_test();
                TOC;
                printf("ipc unit test use:%u ms, ipc_performance_result_%s\n",
                    ms, (ms < msHighest)?"success":"failure");
            }
            break;
        case 2:
            {
                TIC;
                shm_uint_test();
                TOC;
                printf("shm unit test use %u ms\n", ms);
            }
            break;
        case 3:
            if (2 == argc - optind || 3 == argc - optind) {
                TIC;
                mp3_offload_dec(argc - optind, &argv[optind]);
                TOC;
                printf("mp3 offload decoder use:%u ms\n", ms);
            }
            else {
                usage();
                exit(1);
            }
            break;
        case 4:
            if (1 == argc - optind)
                pcm_play_test(argc - optind, &argv[optind]);
                else {
                usage();
                exit(1);
            }
            break;
        case 5:
            if (1 == argc - optind || 2 ==  argc - optind)
                pcm_capture_test(argc - optind, &argv[optind]);
            else {
                usage();
                exit(1);
            }
            break;
        case 6:
            pcm_play_buildin();
        case 7:
            if (3 == argc - optind || 4 == argc - optind) {
                TIC;
                aac_offload_dec(argc - optind, &argv[optind]);
                TOC;
                printf("aac offload decoder use:%u ms\n", ms);
            }
            else {
                usage();
                exit(1);
            }
            break;
        case 8:
            if (2 == argc - optind || 4 == argc - optind) {
                TIC;
                //offload_vsp_rsp(argc - optind, &argv[optind]);
                aml_rsp_unit_test(argc - optind, &argv[optind]);
                TOC;
                printf("offload voice signal resampler use:%u ms\n", ms);
            }
            else {
                usage();
                exit(1);
            }
            break;
        case 9:
            if (3 == argc - optind){
                TIC;
                rpc_unit_test(argc - optind, &argv[optind]);
                TOC;
                printf("rpc unit test use:%u ms\n", ms);
            } else {
                usage();
                exit(1);
            }
            break;
        case 10:
            if (5 == argc - optind || 6 == argc - optind){
                TIC;
                aml_wake_engine_unit_test(argc - optind, &argv[optind]);
                TOC;
                printf("awe unit test use:%u ms\n", ms);
            } else {
                usage();
                exit(1);
            }
            break;
        case 11:
            if (2 == argc - optind){
                aml_wake_engine_dspin_test(argc - optind, &argv[optind]);
            } else {
                usage();
                exit(1);
            }
            break;
        case 12:
            aml_hifi4_inside_wakeup();
            break;
        case 13:
            if (4 == argc - optind){
                aml_s16leresampler(argc - optind, &argv[optind]);
            } else {
                usage();
                exit(1);
            }
            break;
        case 14:
            aml_hifi4_timer_wakeup();
            break;
        case 15:
            if (1 == argc - optind){
                uint32_t msHighest = 450;
                TIC;
                flat_buf_test(argc - optind, &argv[optind]);
                TOC;
                printf("flat buf test use:%u ms, flatbuf_performance_result_%s\n",
                        ms, (ms < msHighest)?"success":"failure");
            } else {
                usage();
                exit(1);
            }
            break;
        case 16:
            {
                TIC;
                ipc_uint_test1();
                TOC;
                printf("ipc unit test use:%u ms\n", ms);
            }
            break;
        case '?':
            usage();
            exit(1);
            break;
    }
    return 0;
}
