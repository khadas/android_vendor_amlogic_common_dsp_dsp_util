/*
 * Copyright (C) 2021-2025 Amlogic, Inc. All rights reserved.
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
 * sound card audio bridge
 *
 * Author: ziheng li <ziheng.li@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <iostream>
#include "ring_buffer.h"
#include "audio_hal.h"

using namespace std;

unsigned int nchannels = 2;
unsigned int rate = 48000;
PCM_FORMAT format = PCM_FOR_S16_LE;
unsigned int byte = 2;
unsigned int period = 256;
unsigned int buffer_size = period * nchannels * byte * 10;


struct PCM_DEVICE_INFO {
	DEVICE_TYPE type;
	PCM_DEVICE dev;
	PCM_MODE mode;
	pthread_t pid;
};

struct PCM_LINE {
	char *name;
	struct PCM_DEVICE_INFO src;
	struct PCM_DEVICE_INFO dst;
	RingBuf *rbuf;
};

int running = 1;
void Stop(int signo)
{
	audio_info("signal is %d.\n",signo);
	running = 0;
}

void *playback_pthread(void *arg)
{
	struct PCM_LINE *pcm_line = (struct PCM_LINE *)arg;
	struct PCM_DEVICE_INFO *pdev = &pcm_line->dst;
	AudioHal playback(pdev->type);
	unsigned char *buffer;
	int ret;

	Audio_Hw_Param play_hparam = {nchannels, rate, format, byte, period};
	int size = play_hparam.pcm_period * play_hparam.pcm_nchannels * play_hparam.pcm_byte;

	ret = playback.pcm_open(pdev->dev, pdev->mode, &play_hparam);
	if (ret < 0) {
		audio_err("pcm open device %d fail.\n",pdev->dev);
		running = 0;
	}

	buffer = (unsigned char *)playback.pcm_alloc_buffer(size);
	if (buffer == NULL) {
		audio_err("pcm alloc buffer fail.\n");
		running = 0;
	}
	playback.pcm_start();
	while (running) {
		ret = pcm_line->rbuf->rb_read(buffer, size) / (play_hparam.pcm_nchannels * play_hparam.pcm_byte);
		if (playback.pcm_write(buffer, ret) < 0) {			//size >> 2 -> size / channel / byte
			while ((pcm_line->rbuf->rb_get_size_of_already_used() < (size * 3)) && running)
				usleep(1000);
			playback.pcm_restore();
		}
	}
	playback.pcm_stop();
	playback.pcm_free_buffer();
	playback.pcm_close();
	return 0;
}

void *capture_pthread(void *arg)
{
	struct PCM_LINE *pcm_line = (struct PCM_LINE *)arg;
	struct PCM_DEVICE_INFO *pdev = &pcm_line->src;
	AudioHal capture(pdev->type);
	unsigned char *buffer;
	int ret;

	Audio_Hw_Param recor_hparam = {nchannels, rate, format, byte, period};
	int size = recor_hparam.pcm_period * recor_hparam.pcm_nchannels * recor_hparam.pcm_byte;

	ret = capture.pcm_open(pdev->dev, pdev->mode, &recor_hparam);
	if (ret < 0) {
		audio_err("pcm open device %d fail.\n",pdev->dev);
		running = 0;
	}

	buffer = (unsigned char *)capture.pcm_alloc_buffer(size);
	if (buffer == NULL) {
		audio_err("pcm alloc buffer fail.\n");
		running = 0;
	}

	capture.pcm_start();
	while (running) {
		size = capture.pcm_read(buffer, recor_hparam.pcm_period);
		if (size > 0)
			pcm_line->rbuf->rb_write(buffer, size);
	}
	capture.pcm_stop();
	capture.pcm_free_buffer();
	capture.pcm_close();
	return 0;
}

int create_line_pthread(struct PCM_LINE *PcmLine)
{
	RingBuf *tRB = new RingBuf();
	if (tRB == NULL)
		return -1;

	if (tRB->rb_init(buffer_size) < 0)
		return -1;

	PcmLine->rbuf = tRB;
	if (pthread_create(&PcmLine->src.pid, NULL, capture_pthread, PcmLine) != 0)
		return -1;

	if (pthread_create(&PcmLine->dst.pid, NULL, playback_pthread, PcmLine) != 0)
		return -1;

	return 0;
}

void destroy_line_pthread(struct PCM_LINE *PcmLine)
{
	pthread_join(PcmLine->src.pid, NULL);
	pthread_join(PcmLine->dst.pid, NULL);
	PcmLine->rbuf->rb_deinit();
	delete PcmLine->rbuf;
}

int main()
{
	signal(SIGINT, Stop);
	int ret;

	struct PCM_LINE PcmLine0 =
		{(char *)"line0",
		{DEV_DSPC, PCM_DEV_LOOPBACK, PCM_CARTURE},	//SRC
		{DEV_INTALSA, PCM_DEV_UVC, PCM_PLAYBACK},	//DST
		NULL};
	struct PCM_LINE PcmLine1 =
		{(char *)"line1",
		{DEV_INTALSA, PCM_DEV_UVC, PCM_CARTURE},	//SRC
		{DEV_DSPC, PCM_DEV_TDMB, PCM_PLAYBACK},		//DST
		NULL};

	ret = create_line_pthread(&PcmLine0);
	if (ret) {
		audio_err("%s:thread create fail.\n", PcmLine0.name);
		running = 0;
	}

	ret = create_line_pthread(&PcmLine1);
	if (ret) {
		audio_err("%s:thread create fail.\n", PcmLine1.name);
		running = 0;
	}

	//Wait for stable data transmission
	a_sleep(1);
	audio_debug("start size %s:%d, %s:%d.\n",PcmLine0.name, PcmLine0.rbuf->rb_get_size_of_already_used(), PcmLine1.name, PcmLine1.rbuf->rb_get_size_of_already_used());
	while (running)
	{
		//reduce cpu occupancy rate
		a_usleep(1000);
		audio_debug("\r");
		audio_debug("%s:buffer used size %d.        ",
			PcmLine0.name, PcmLine0.rbuf->rb_get_size_of_already_used());

		audio_debug("%s:buffer used size %d.        ",
			PcmLine1.name, PcmLine1.rbuf->rb_get_size_of_already_used());

		fflush(stdout);
	}

	destroy_line_pthread(&PcmLine0);
	destroy_line_pthread(&PcmLine1);
	return 0;
}
