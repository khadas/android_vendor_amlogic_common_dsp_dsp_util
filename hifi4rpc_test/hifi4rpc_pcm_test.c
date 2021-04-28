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
 * hifi4 rpc client api, hifi4 tinyalsa
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
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "rpc_client_pcm.h"
#include <endian.h>

#include<getopt.h>
#include <time.h>
#include "rpc_dev.h"
#include "ipc_cmd_type.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT	0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t wave_id;
};

struct chunk_header {
	uint32_t id;
	uint32_t sz;
};

struct chunk_fmt {
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
};

#if 0
#define AML_MSECS_PER_SEC (1000L)
#define AML_NSECS_PER_MSEC (1000000L)

static void get_cur_timestamp(struct timespec* ts)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
	return;
}

static int32_t msec_duration(struct timespec* tsEnd, struct timespec* tsStart)
{
	uint32_t uEndMSec = (uint32_t)(tsEnd->tv_sec*AML_MSECS_PER_SEC) + (uint32_t)(tsEnd->tv_nsec/AML_NSECS_PER_MSEC);
	uint32_t uStartMSec = (uint32_t)(tsStart->tv_sec*AML_MSECS_PER_SEC) + (uint32_t)(tsStart->tv_nsec/AML_NSECS_PER_MSEC);
	//printf("uEndMSec:%d, uStartMSec:%d\n", uEndMSec, uStartMSec);
	return (uEndMSec - uStartMSec);
}
#endif
#if 0
struct timespec bgn, end;

#define AML_TIC 			 \
	get_cur_timestamp(&bgn)

#define AML_TOC 						   \
	get_cur_timestamp(&end)
#define AML_MS \
	msec_duration(&end, &bgn)

#endif
#if 1
uint32_t audio_play_data[] = {
//#include "sinewav_48k_24bits_stereo_l1k_r2k_1s.in"
#include "pyghlkn_48k_24bits_stereo_10s.in"
};
#else
uint8_t audio_play_data[] = {
#include "dtmf_48k_32bits_stereo.in"
};
#endif
uint32_t audio_play_data_len = sizeof(audio_play_data);

int pcm_play_buildin()
{
	const int ms = 36;
	const int oneshot = 48 * ms; // 1728 samples
	rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = 2;
	pconfig->rate = 48000;
	pconfig->format = PCM_FORMAT_S32_LE;
	pconfig->period_size = oneshot;
	pconfig->period_count = 8;
	pconfig->start_threshold = oneshot*4;
	pconfig->silence_threshold = oneshot * 2;
	pconfig->stop_threshold = oneshot * 2;
	tAmlPcmhdl p = pcm_client_open(0, DEVICE_TDMOUT_B, PCM_OUT, pconfig);
	AML_MEM_HANDLE hShmBuf;

	uint8_t *play_data = (uint8_t *)audio_play_data;
	int in_fr = pcm_client_bytes_to_frame(p, audio_play_data_len);
	int i, fr = 0;
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = AML_MEM_Allocate(size);
	void *buf = AML_MEM_GetVirtAddr(hShmBuf);
	void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
	for (i = 0; i + oneshot <= in_fr; i += fr) {
		memcpy(buf, play_data + pcm_client_frame_to_bytes(p, i), size);
		AML_MEM_Clean(phybuf, size);
		fr = pcm_client_writei(p, phybuf, oneshot);
		//printf("%dms pcm_write i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
		  //ms, i, p, buf, oneshot, fr);
	}
	pcm_client_close(p);
	AML_MEM_Free(hShmBuf);
	free(pconfig);
	return 0;
}

 int play_wav(int argc, char *argv[])
{
	FILE *file;
	struct riff_wave_header riff_wave_header;
	struct chunk_header chunk_header;
	struct chunk_fmt chunk_fmt;
	char *filename;
	int more_chunks = 1;
	int oneshot;
	int device;
	if (argc > 2) {
		fprintf(stderr, "Error:argc is greater than 2\n");
		return -1;
	}

	if(argc == 2) {
		device = atoi(argv[1]);
	} else if (argc == 1) {
		device = DEVICE_TDMOUT_B;
	}
	rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
	filename = argv[0];
	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		return -1;
	}
	fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
	if ((riff_wave_header.riff_id != ID_RIFF) ||(riff_wave_header.wave_id != ID_WAVE)) {
		fprintf(stderr, "Error: '%s' is not a riff/wave file\n", filename);
		fclose(file);
		return -1;
	}
	do {
		fread(&chunk_header, sizeof(chunk_header), 1, file);
		switch (chunk_header.id) {
		case ID_FMT:
			fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
				/* If the format header is larger, skip the rest */
				if (chunk_header.sz > sizeof(chunk_fmt))
					fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
				break;
			case ID_DATA:
				/* Stop looking for chunks */
				more_chunks = 0;
				chunk_header.sz = le32toh(chunk_header.sz);
				break;
			default:
				/* Unknown chunk, skip bytes */
				fseek(file, chunk_header.sz, SEEK_CUR);
		}
	} while (more_chunks);

	printf("audio_format:%d\n",chunk_fmt.audio_format);
	printf("num_channels:%d\n",chunk_fmt.num_channels);
	printf("sample_rate:%d\n",chunk_fmt.sample_rate);
	printf("byte_rate:%d\n",chunk_fmt.byte_rate);
	printf("block_align:%d\n",chunk_fmt.block_align);
	printf("bits_per_sample:%d\n",chunk_fmt.bits_per_sample);
	pconfig->channels = chunk_fmt.num_channels;
	pconfig->rate = chunk_fmt.sample_rate;
	if(chunk_fmt.bits_per_sample == 24) {
		if (chunk_fmt.block_align/chunk_fmt.num_channels == 3) {
			pconfig->format = PCM_FORMAT_S24_3LE;
			printf("format:PCM_FORMAT_S24_3LE\n");
		} else {

			pconfig->format = PCM_FORMAT_S24_LE;
			printf("format:PCM_FORMAT_S24_LE\n");
		}
	} else if(chunk_fmt.bits_per_sample == 32) {
		pconfig->format = PCM_FORMAT_S32_LE;
	} else if(chunk_fmt.bits_per_sample == 16) {
		pconfig->format = PCM_FORMAT_S16_LE;

	}
	pconfig->period_size = 1024;
	pconfig->period_count = 8;
	pconfig->start_threshold = 1024*4;
	pconfig->silence_threshold = 1024 * 2;
	pconfig->stop_threshold = 1024 * 2;
	oneshot = pconfig->period_size;
	tAmlPcmhdl p = pcm_client_open(0, device, PCM_OUT, pconfig);
	AML_MEM_HANDLE hShmBuf;
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = AML_MEM_Allocate(size);
	void *buf = AML_MEM_GetVirtAddr(hShmBuf);
	void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
	while (size) {
	  //  AML_TIC;
		size = fread(buf, 1, size,file);
	  //  AML_TOC;
	   // printf("read %d ms\n",AML_MS);
	  //  AML_TIC;
		AML_MEM_Clean(phybuf, size);
	   // AML_TOC;
	   // printf("clean %d ms\n",AML_MS);
	   // AML_TIC;
		pcm_client_writei(p, phybuf, oneshot);
	   // AML_TOC;
	   // printf("write %d ms\n",AML_MS);
	}
	pcm_client_close(p);
	AML_MEM_Free(hShmBuf);
	free(pconfig);
	fclose(file);
	return 0;
}

int pcm_play_test(int argc, char *argv[])
{
	//const int ms = 36;
	argc =1;
	const int oneshot = 1024; // 1728 samples
	rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = atoi(argv[1]);
	pconfig->rate = atoi(argv[2]);
	if(atoi(argv[3]) == 0) {
		pconfig->format = PCM_FORMAT_S32_LE;
	} else if(atoi(argv[3]) == 1) {
		pconfig->format = PCM_FORMAT_S16_LE;
	} else if(atoi(argv[3]) == 2) {
		pconfig->format = PCM_FORMAT_S24_3LE;
	}
	pconfig->period_size = oneshot;
	pconfig->period_count = 8;
	pconfig->start_threshold = oneshot*4;
	pconfig->silence_threshold = oneshot * 2;
	pconfig->stop_threshold = oneshot * 2;
	tAmlPcmhdl p = pcm_client_open(0, DEVICE_TDMOUT_B, PCM_OUT, pconfig);
	AML_MEM_HANDLE hShmBuf;
	FILE *fileplay = fopen(argv[0], "rb");
	if (fileplay == NULL) {
		printf("failed to open played pcm file\n");
		return -1;
	}
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = AML_MEM_Allocate(size);
	void *buf = AML_MEM_GetVirtAddr(hShmBuf);
	void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
	while (size) {
	  //  AML_TIC;
		size = fread(buf, 1, size, fileplay);
	  //  AML_TOC;
	   // printf("read %d ms\n",AML_MS);
	  //  AML_TIC;
		AML_MEM_Clean(phybuf, size);
	   // AML_TOC;
	   // printf("clean %d ms\n",AML_MS);
	   // AML_TIC;
		pcm_client_writei(p, phybuf, oneshot);
	   // AML_TOC;
	   // printf("write %d ms\n",AML_MS);
	}
	pcm_client_close(p);
	AML_MEM_Free(hShmBuf);
	free(pconfig);
	fclose(fileplay);
	return 0;
}

int pcm_capture_test(int argc, char *argv[])
{
	if (argc != 7 && argc != 8) {
		printf("Invalide param number:%d\n", argc);
		return -1;
	}
	int seconds = atoi(argv[0]);
	int chunkMs = atoi(argv[1]);
	int chn = atoi(argv[2]);
	int rate = atoi(argv[3]);
	int format = atoi(argv[4]);
	if (format != 0 && format != 1 && format!= 2) {
		printf("Not supported format:%d\n", format);
		return -1;
	}
	int device = atoi(argv[5]);
	if (device != 1 && device != 3 && device != 4) {
		printf("Not supported device:%d\n", device);
		return -1;
	}
	FILE *filecap = fopen(argv[6], "w+b");
	if (filecap == NULL) {
		printf("failed to open captured pcm file\n");
		return -1;
	}
	int flags = PCM_IN;
	if (argc >= 8) {
		if (strcasecmp(argv[7], "block") == 0) {
			// default to block
		} else if (strcasecmp(argv[7], "nonblock") == 0) {
			flags |= PCM_NONBLOCK;
		} else {
			printf("fail to parse option: [block,nonblock]\n");
			return -1;
		}
	}
	rpc_pcm_config *pconfig = (rpc_pcm_config *)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = chn;
	pconfig->rate = rate;
	if( format == 0) {
		pconfig->format = PCM_FORMAT_S32_LE;
	} else if( format == 1) {
		pconfig->format = PCM_FORMAT_S16_LE;
	} else if( format == 2) {
		 pconfig->format = PCM_FORMAT_S24_3LE;
	}
   // pconfig->format = (format == 0) ? PCM_FORMAT_S32_LE : PCM_FORMAT_S16_LE;
	pconfig->period_size = chunkMs * (rate/1000);
	pconfig->period_count = 4;
	pconfig->start_threshold = 1024;
	pconfig->silence_threshold = 1024 * 2;
	pconfig->stop_threshold = 1024 * 2;
	tAmlPcmhdl p = pcm_client_open(0, device, flags, pconfig);
	AML_MEM_HANDLE hShmBuf;
	int in_fr = rate * seconds;
	int i, fr = 0;
	const int oneshot = chunkMs * (rate/1000);
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = AML_MEM_Allocate(size);
	void *buf = AML_MEM_GetVirtAddr(hShmBuf);
	void *phybuf = AML_MEM_GetPhyAddr(hShmBuf);
	for (i = 0; i + oneshot <= in_fr; i += fr) {
		fr = pcm_client_readi(p, phybuf, oneshot);
		AML_MEM_Invalidate(phybuf, size);
		fwrite(buf, sizeof(char), size, filecap);
		//printf("i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n", i, p, buf, oneshot, fr);
	}
	pcm_client_close(p);
	AML_MEM_Free(hShmBuf);
	free(pconfig);
	fclose(filecap);
	return 0;
}
