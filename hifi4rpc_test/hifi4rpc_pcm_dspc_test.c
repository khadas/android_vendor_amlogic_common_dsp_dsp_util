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
 * Author: ZiHeng Li <ziheng.li@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <endian.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include "generic_macro.h"
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "aml_flatbuf_api.h"
#include "rpc_client_mp3.h"
#include "rpc_client_pcm.h"
#include "pcm.h"

tAmlPcmhdl phandle;
int seconds;

#define DQBUFREQUEST 1
/* DSPConcept algorithm audio data */
static rpc_pcm_config pconfig = {
	.channels = 4, // 2 ch PDM + 2 ch TDMB-loopback
	.rate = 48000,
	.format = PCM_FORMAT_S16_LE,
	.period_size = 256,
	.period_count = 4,
	.start_threshold = 1024,
	.silence_threshold = 1024 * 2,
	.stop_threshold = 1024 * 2
};

char raw_name[128] = "/data/pro_dspc_rawdata";
char process_name[128] = "/data/pro_dspc_processeddata";

unsigned int pcm_format_to_bits(enum pcm_format format) {
    switch (format) {
        case PCM_FORMAT_S24_LE:
        case PCM_FORMAT_S24_BE:
        case PCM_FORMAT_S32_LE:
        case PCM_FORMAT_S32_BE:
            return 32;
        case PCM_FORMAT_S24_3LE:
        case PCM_FORMAT_S24_3BE:
            return 24;
        case PCM_FORMAT_S16_LE:
        case PCM_FORMAT_S16_BE:
            return 16;
        default:
            printf("unknown format=%d\n", format);
            return 0;
    }
}

unsigned int pcm_format_to_bytes(enum pcm_format format) {
    return pcm_format_to_bits(format) >> 3;
}

void* pro_read_dspc_rawdata(void* arg)
{
	AMX_UNUSED(arg);
	int ret;
	unsigned int rdLen = pconfig.rate * pconfig.channels * pcm_format_to_bytes(pconfig.format) * seconds;
#ifdef DQBUFREQUEST
	struct buf_info buf;
#else
	unsigned int bufsize = pconfig.period_size * pconfig.channels * pcm_format_to_bytes(pconfig.format);
	AML_MEM_HANDLE hShmBuf = AML_MEM_Allocate(bufsize);
	void *virbuf = AML_MEM_GetVirtAddr(hShmBuf);
	void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
#endif
	FILE *fRawdata = fopen(raw_name,"w+b");
	if (fRawdata == NULL) {
		printf("failed to open %s file\n", raw_name);
		return NULL;
	}
	printf("open %s successful.\n", raw_name);
	while (rdLen > 0) {
#ifdef DQBUFREQUEST
		ret = pcm_process_client_dqbuf(phandle, &buf, NULL, RAWBUF);
		if (buf.size) {
			rdLen -= buf.size;
			fwrite(buf.viraddr, sizeof(char), buf.size, fRawdata);
			pcm_process_client_qbuf(phandle, &buf, RAWBUF);
		}
#else
		ret = pcm_process_client_readi(phandle, phybuf, pconfig.period_size, RAWBUF);
		if (ret) {
			AML_MEM_Invalidate(phybuf, ret);
			rdLen -= ret;
			fwrite(virbuf, sizeof(char), ret, fRawdata);
		}
#endif
	}

	printf("dspc_read_rawdata_finish_rdLen:%d\n",rdLen);
exit_capture:
#ifndef DQBUFREQUEST
	if (hShmBuf)
		AML_MEM_Free(hShmBuf);
#endif
	if (fRawdata)
		fclose(fRawdata);
	return NULL;
}

void* pro_read_dspc_processeddata(void* arg)
{
	AMX_UNUSED(arg);
	int ret;
	unsigned int rdLen = 16000 * (pconfig.channels - 2) * pcm_format_to_bytes(pconfig.format) * seconds;
#ifdef DQBUFREQUEST
	struct buf_info buf;
#else
	unsigned int bufsize = pconfig.period_size * (pconfig.channels - 2) * pcm_format_to_bytes(pconfig.format);
	AML_MEM_HANDLE hShmBuf = AML_MEM_Allocate(bufsize);
	void *virbuf = AML_MEM_GetVirtAddr(hShmBuf);
	void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
#endif
	FILE *fProcesseddata = fopen(process_name,"w+b");
	if (fProcesseddata == NULL) {
		printf("failed to open %s file\n", process_name);
		return NULL;
	}
	printf("open %s successful.\n", process_name);
	while (rdLen > 0) {
#ifdef DQBUFREQUEST
		ret = pcm_process_client_dqbuf(phandle, &buf, NULL, PROCESSBUF);
		if (buf.size) {
			rdLen -= buf.size;
			fwrite(buf.viraddr, sizeof(char), buf.size, fProcesseddata);
			pcm_process_client_qbuf(phandle, &buf, PROCESSBUF);
		}
#else
		ret = pcm_process_client_readi(phandle, phybuf, pconfig.period_size, PROCESSBUF);
		if (ret) {
			AML_MEM_Invalidate(phybuf, ret);
			rdLen -= ret;
			fwrite(virbuf, sizeof(char), ret, fProcesseddata);
		}
#endif

	}

	printf("dspc_read_processeddata_finish_rdLen:%d\n",rdLen);
exit_capture:
#ifndef DQBUFREQUEST
	if (hShmBuf)
		AML_MEM_Free(hShmBuf);
#endif
	if (fProcesseddata)
		fclose(fProcesseddata);
	return NULL;
}

int dspc_pro_pcm(int argc, char* argv[])
{
	int format;
	if (argc != 4 && argc != 5) {
		printf("Invalide param number:%d\n", argc);
		return -1;
	}
	seconds = atoi(argv[0]);
	pconfig.channels = atoi(argv[1]);
	pconfig.rate = atoi(argv[2]);
	format = atoi(argv[3]);
	if (format != 0 && format != 1) {
		printf("Not supported format:%d\n", format);
		return -1;
	}
	if (format == 0) {
		pconfig.format = PCM_FORMAT_S32_LE;
	} else if (format == 1) {
		pconfig.format = PCM_FORMAT_S16_LE;
	}

	if (argc == 5) {
		sprintf(raw_name, "%s%s", argv[4], "raw_data");
		sprintf(process_name, "%s%s", argv[4], "process_data");
	}

	pthread_t rd_dspc_thread_rawdata;
	pthread_t rd_dspc_thread_processeddata;

	if ((phandle = pcm_process_client_open(0, DEVICE_LOOPBACK, PCM_IN, &pconfig)) == NULL) {
		printf("pcm client open fail\n");
		return -1;
	}

	pthread_create(&rd_dspc_thread_rawdata, NULL, pro_read_dspc_rawdata, NULL);
	pthread_create(&rd_dspc_thread_processeddata, NULL, pro_read_dspc_processeddata, NULL);
	pthread_join(rd_dspc_thread_rawdata, NULL);
	pthread_join(rd_dspc_thread_processeddata, NULL);

	pcm_process_client_close(phandle);
	return 0;
}
