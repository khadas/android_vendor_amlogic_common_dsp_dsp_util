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
#include "aml_audio_util.h"

int mp3_offload_dec(int argc, char* argv[]);
int aac_offload_dec(int argc, char* argv[]);

#ifdef __cplusplus
extern "C" {
#endif
int pcm_play_buildin(void);
int pcm_play_test(int argc, char* argv[]);
int bcm_file_test(int argc, char *argv[]);
int bcm_pcm_test(int argc, char* argv[]);
int xaf_test(int argc, char *argv[]);
int xaf_dump(int argc, char **argv);
int pcm_capture_test(int argc, char* argv[]);
int offload_vsp_rsp(int argc, char* argv[]);
int aml_wake_engine_unit_test(int argc, char* argv[]);
int aml_wake_engine_dspin_test(int argc, char* argv[]);
int ipc_uint_test(int id);
int ipc_uint_test1(int id);
int rpc_unit_test(int argc, char* argv[]) ;
int shm_uint_test(void);
int pcm_loopback_test(int argc, char* argv[]);
void aml_s16leresampler(int argc, char* argv[]);
void aml_hifi4_inside_wakeup();
void aml_hifi4_timer_wakeup();
int aml_rsp_unit_test(int argc, char* argv[]);
int flat_buf_test(int argc, char* argv[]);
int aml_pcm_gain_unit_test(int argc, char* argv[]);
int hifi4_tbuf_test(int argc, char* argv[]);
int aml_pcm_test(int argc, char **argv);
#ifdef __cplusplus
}
#endif
static void usage()
{
    printf ("\033[1mipc unit test usage:\033[m hifi4rpc_client_test --ipc\n");

    printf ("\033[1mipc1 unit test Usage:\033[m hifi4rpc_client_test --ipc1\n");

    printf ("\033[1mipc2 unit test Usage:\033[m hifi4rpc_client_test --ipc2 $hifiId[0:HiFiA, 1:HiFiB]\n");

    printf ("\033[1mrpc unit test usage:\033[m hifi4rpc_client_test --rpc $func_code[1:dummy, 2:square] $input_param $sleep_time[ms]\n");

    printf ("\033[1mshared memory unit test usage:\033[m hifi4rpc_client_test --shm\n");

    printf ("\033[1mmp3dec Usage:\033[m hifi4rpc_client_test --mp3dec $input_file $output_file\n");

    printf ("\033[1maacdec Usage:\033[m hifi4rpc_client_test --aacdec $format $input_file $output_file\n");
    printf ("  format: [0-RAW, 1-ADIF, 2-ADTS, 6-LATM_MCP1, 7-LATM_MCP0, 10-LOAS]\n");

    printf ("\033[1mdspplay Usage:\033[m hifi4rpc_client_test --dspplay $pcm_file\n");

    printf ("\033[1mdspcap Usage:\033[m hifi4rpc_client_test --dspcap $seconds $chunkMs $chn $rate $format $device $pcm_file\n"
            "  format: [0-PCM_FORMAT_S32_LE]\n"
            "  device: [1-tdmin, 3-tdmin&loopback]\n");

    printf ("\033[1mdspplay-buildin Usage:\033[m hifi4rpc_client_test --dspplay-buildin\n");

    printf ("\033[1mhifi inside wakeup test Usage:\033[m hifi4rpc_client_test --hifiwake\n");

    printf ("\033[1mvsp-rsp Usage:\033[m hifi4rpc_client_test --vsp-rsp $input_file $output_file $in_rate $out_rate\n");

    printf ("\033[1mvsp-awe-unit Usage:\033[m hifi4rpc_client_test --vsp-awe-unit $mic0.pcm $mic1.pcm $ref1.pcm $ref2.pcm $out_asr.pcm $out_voip.pcm\n");

    printf ("\033[1mvsp-awe-dspin Usage:\033[m hifi4rpc_client_test --vsp-awe-dspin $out_asr.pcm $out_voip.pcm\n");

    printf ("\033[1mresampler Usage:\033[m hifi4rpc_client_test --resampler $inRate $outRate $inFile $outFile\n");

    printf ("\033[1mtimerwakeup Usage:\033[m hifi4rpc_client_test --timerwakeup\n");

    printf ("\033[1mflatbuffer Usage:\033[m hifi4rpc_client_test --flatbuf-unit\n");

    printf ("\033[1mpcm-gain Usage:\033[m hifi4rpc_client_test --pcm-gain $sampleRate $bitdepth $channels $gain $input $output\n");

    printf ("\033[1mpcm-file Usage:\033[m hifi4rpc_client_test --pcm-file $hifiId[0:HiFiA, 1:HiFiB] $input\n"
            "  File -> TinyCapturer\n");

    printf ("\033[1mpcm-cap Usage:\033[m hifi4rpc_client_test --pcm-cap $hifiId[0:HiFiA, 1:HiFiB]\n"
            "  Tinyalsa -> TinyCapturer\n");

    printf ("\033[1mshm-loopback usage:\033[m hifi4rpc_client_test --shm-loopback $hifiId[0:HiFiA, 1:HiFiB] $input $output\n");

    printf ("\033[1mpcm-loopback usage:\033[m hifi4rpc_client_test --pcm-loopback $hifiId[0:HiFiA, 1:HiFiB] $seconds $output\n");

    printf ("\033[1mpcm-dump Usage:\033[m hifi4rpc_client_test --pcm-dump $hifiId[0:HiFiA, 1:HiFiB] $output\n");

    printf ("\033[1mtbuf-file Usage:\033[m hifi4rpc_client_test --tbuf-file $input $output0 $output1\n");

    printf ("\033[1mtbuf-pcm Usage:\033[m hifi4rpc_client_test --tbuf-pcm $seconds $output0 $output1\n");

    printf ("\033[1maml-pcm Usage:\033[m hifi4rpc_client_test --aml-pcm $seconds $chunkMs $chn $sampleRate $sampleBytes $device $output0 $output1\n"
            "  device: [1-tdmin, 3-tdmin&loopback]\n");

    printf ("\033[1maml-pcm-single Usage:\033[m hifi4rpc_client_test --aml-pcm-single $seconds $chunkMs $chn $sampleRate $sampleBytes $device $hifiId[0:HiFiA, 1:HiFiB]0 $output\n"
            "  device: [1-tdmin, 3-tdmin&loopback]\n");

    printf ("\033[1mxaf Usage:\033[m hifi4rpc_client_test --xaf $hifiId[0:HiFiA, 1:HiFiB] $case\n"
            "  Case 0/am_cap: Trigger TinyCapturer -> PcmGain pipeline\n"
            "  Case 1/am_rnd: Trigger PcmGain -> TinyRenderer pipeline\n"
            "  Case 2/am_pipe: Trigger TinyCapturer -> PcmGain -> TinyRenderer pipeline\n");
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
        {"dspplay", no_argument, NULL, 4},
        {"dspcap", no_argument, NULL, 5},
        {"dspplay-buildin", no_argument, NULL, 6},
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
        {"pcm-gain", no_argument, NULL, 17},
        {"ipc2", no_argument, NULL, 18},
        {"pcm-loopback", no_argument, NULL, 19},
        {"pcm-file", no_argument, NULL, 20},
        {"pcm-cap", no_argument, NULL, 21},
        {"xaf", no_argument, NULL, 22},
        {"tbuf-file", no_argument, NULL, 23},
        {"pcm-dump", no_argument, NULL, 24},
        {"shm-loopback", no_argument, NULL, 25},
        {"tbuf-pcm", no_argument, NULL, 26},
        {"aml-pcm", no_argument, NULL, 27},
        {"aml-pcm-single", no_argument, NULL, 28},
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
                ipc_uint_test(0);
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
            if (7 == argc - optind)
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
            if (6 == argc - optind || 7 == argc - optind){
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
            {
                uint32_t msHighest = 450;
                TIC;
                flat_buf_test(argc - optind, &argv[optind]);
                TOC;
                printf("flat buf test use:%u ms, flatbuf_performance_result_%s\n",
                        ms, (ms < msHighest)?"success":"failure");
            }
            break;
        case 16:
            {
                TIC;
                ipc_uint_test1(0);
                TOC;
                printf("ipc unit test use:%u ms\n", ms);
            }
            break;
        case 17:
            {
                TIC;
                aml_pcm_gain_unit_test(argc - optind, &argv[optind]);
                TOC;
                printf("gain unit test use:%u ms\n", ms);
            }
            break;
        case 18:
            {
                ipc_uint_test(str2hifiId(argv[optind]));
                ipc_uint_test1(str2hifiId(argv[optind]));
            }
            break;
        case 19:
            if (argc == 5 && optind == 2) {
                char* tbuf_arg[4];
                tbuf_arg[0] = argv[optind];
                tbuf_arg[1] = (char*)"pcm";
                tbuf_arg[2] = argv[optind + 1];
                tbuf_arg[3] = argv[optind + 2];
                pcm_loopback_test(4, tbuf_arg);
            } else {
                usage();
                exit(1);
            }
            break;
        case 20: // pcm-file
            bcm_file_test(argc - optind, &argv[optind]);
            break;
        case 21: // pcm-cap
            bcm_pcm_test(argc - optind, &argv[optind]);
            break;
        case 22:
            xaf_test(argc - optind, &argv[optind]);
            break;
        case 23:
            if (argc == 5 && optind == 2)
            {
                char* tbuf_arg[4];
                tbuf_arg[0] = (char*)"file";
                tbuf_arg[1] = argv[optind];
                tbuf_arg[2] = argv[optind + 1];
                tbuf_arg[3] = argv[optind + 2];
                TIC;
                hifi4_tbuf_test(4, tbuf_arg);
                TOC;
                printf("tbuf unit test use:%u ms\n", ms);
            } else {
                usage();
                exit(1);
            }
            break;
        case 24: // pcm-dump
            xaf_dump(argc - optind, &argv[optind]);
            break;
        case 25:
            if (argc == 5 && optind == 2) {
                char* tbuf_arg[4];
                tbuf_arg[0] = argv[optind];
                tbuf_arg[1] = (char*)"file";
                tbuf_arg[2] = argv[optind + 1];
                tbuf_arg[3] = argv[optind + 2];
                TIC;
                pcm_loopback_test(4, tbuf_arg);
                TOC;
                printf("buffer loopback unit test use:%u ms\n", ms);
            } else {
                usage();
                exit(1);
            }
            break;
        case 26:
            if (argc == 5 && optind == 2)
            {
                char* tbuf_arg[4];
                tbuf_arg[0] = (char*)"pcm";
                tbuf_arg[1] = argv[optind];
                tbuf_arg[2] = argv[optind + 1];
                tbuf_arg[3] = argv[optind + 2];
                hifi4_tbuf_test(4, tbuf_arg);
            } else {
                usage();
                exit(1);
            }
            break;
        case 27:
            if (argc == 10 && optind == 2)
            {
                char* aml_pcm_arg[8];
                aml_pcm_arg[0] = (char*)"dual";
                aml_pcm_arg[1] = argv[optind];
                aml_pcm_arg[2] = argv[optind + 1];
                aml_pcm_arg[3] = argv[optind + 2];
                aml_pcm_arg[4] = argv[optind + 3];
                aml_pcm_arg[5] = argv[optind + 4];
                aml_pcm_arg[6] = argv[optind + 5];
                aml_pcm_arg[7] = argv[optind + 6];
                aml_pcm_arg[8] = argv[optind + 7];
                aml_pcm_test(9, aml_pcm_arg);
            } else {
                usage();
                exit(1);
            }
            break;
        case 28:
            if (argc == 10 && optind == 2)
            {
                char* aml_pcm_arg[8];
                aml_pcm_arg[0] = (char*)"single";
                aml_pcm_arg[1] = argv[optind];
                aml_pcm_arg[2] = argv[optind + 1];
                aml_pcm_arg[3] = argv[optind + 2];
                aml_pcm_arg[4] = argv[optind + 3];
                aml_pcm_arg[5] = argv[optind + 4];
                aml_pcm_arg[6] = argv[optind + 5];
                aml_pcm_arg[7] = argv[optind + 6];
                aml_pcm_arg[8] = argv[optind + 7];
                aml_pcm_test(9, aml_pcm_arg);
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
