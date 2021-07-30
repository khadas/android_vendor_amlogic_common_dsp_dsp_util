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
 * dspc operation api
 *
 * Author: ziheng li <ziheng.li@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <iostream>
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

#include "audio_dspc.h"
#include "audio_common.h"

#define DQBUF_MODE 1
using namespace std;

struct AML_DSP_ADEVICE {
	int value;
	int ahw_id;
};

const struct AML_DSP_ADEVICE aml_dsp_adev[] =
{
	{((PCM_DEV_TDMA << 1) | PCM_CARTURE), DEVICE_TDMIN_A},
	{((PCM_DEV_TDMA << 1) | PCM_PLAYBACK), DEVICE_TDMOUT_A},
	{((PCM_DEV_TDMB << 1) | PCM_CARTURE), DEVICE_TDMIN_B},
	{((PCM_DEV_TDMB << 1) | PCM_PLAYBACK), DEVICE_TDMOUT_B},
	{((PCM_DEV_SPDIFIN << 1) | PCM_CARTURE), DEVICE_SPDIFIN},
	{((PCM_DEV_LOOPBACK << 1) | PCM_CARTURE), DEVICE_LOOPBACK},
	{((PCM_DEV_PDMIN << 1) | PCM_CARTURE), DEVICE_PDMIN},

};

int find_dsp_device_node(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode)
{
	int i;
	for (i = 0; i < sizeof(aml_dsp_adev)/sizeof(struct AML_DSP_ADEVICE); i++)
	{
		if (aml_dsp_adev[i].value == ((pcm_hw << 1) | pcm_mode)) {
			return aml_dsp_adev[i].ahw_id;
		}

	}
	return -1;
}

int find_dsp_format_node(PCM_FORMAT pcm_format)
{
	int ret = 0;
	switch (pcm_format)
	{
		case PCM_FOR_S8: ret = PCM_FORMAT_S8;
			break;
		case PCM_FOR_S16_LE: ret = PCM_FORMAT_S16_LE;
			break;
		case PCM_FOR_S16_BE: ret = PCM_FORMAT_S16_BE;
			break;
		case PCM_FOR_S24_LE: ret = PCM_FORMAT_S24_LE;
			break;
		case PCM_FOR_S24_BE: ret = PCM_FORMAT_S24_BE;
			break;
		case PCM_FOR_U24_3LE: ret = PCM_FORMAT_S24_3LE;
			break;
		case PCM_FOR_U24_3BE: ret = PCM_FORMAT_S24_3BE;
			break;
		case PCM_FOR_S32_LE: ret = PCM_FORMAT_S32_LE;
			break;
		case PCM_FOR_S32_BE: ret = PCM_FORMAT_S32_BE;
			break;
		default:
			ret = -1;
			break;

	}

	return ret;
}

int AudioDspc::pcm_open(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode, Audio_Hw_Param *pcm_hparam)
{
	int value;

	pmode = pcm_mode;
	if (pcm_mode == PCM_PLAYBACK) {
		pconfig.channels = pcm_hparam->pcm_nchannels;
		pconfig.rate = pcm_hparam->pcm_rate;

		if ((value = find_dsp_device_node(pcm_hw, pcm_mode)) < 0) {
			audio_err("not support this device(%d,%d)\n",pcm_hw,pcm_mode);
			return -1;
		}
		if ((pconfig.format = find_dsp_format_node(pcm_hparam->pcm_format)) < 0) {
			audio_err("not support this format(%d,%d)\n",pcm_hparam->pcm_format);
			return -1;
		}

		pconfig.period_size = pcm_hparam->pcm_period;
		pconfig.period_count = 8;
		pconfig.start_threshold = 0;
		pconfig.silence_threshold = 0;
		pconfig.stop_threshold = 0;

		if ((phandle = pcm_client_open(0, value, PCM_OUT, &pconfig)) == NULL) {
			audio_err("pcm client open fail\n");
			return -1;
		}

		pcm_hparam->pcm_period = pconfig.period_size;

	} else {
		pconfig.channels = pcm_hparam->pcm_nchannels*2;
		pconfig.rate = pcm_hparam->pcm_rate;

		if ((value = find_dsp_device_node(pcm_hw, pcm_mode)) < 0) {
			audio_err("not support this device(%d,%d)\n",pcm_hw,pcm_mode);
			return -1;
		}
		if ((pconfig.format = find_dsp_format_node(pcm_hparam->pcm_format)) < 0) {
			audio_err("not support this format(%d,%d)\n",pcm_hparam->pcm_format);
			return -1;
		}

		pconfig.period_size = pcm_hparam->pcm_period;
		pconfig.period_count = 4;
		pconfig.start_threshold = 0;
		pconfig.silence_threshold = 0;
		pconfig.stop_threshold = 0;

		if ((phandle = pcm_process_client_open(0, value, PCM_IN, &pconfig)) == NULL) {
			audio_err("pcm client open fail\n");
			return -1;
		if (pcm_process_client_set_volume_gain(phandle, 20))
			return -1;
		}
	}
	return 0;
}

void AudioDspc::pcm_close(void)
{
	if (pmode == PCM_PLAYBACK) {
		pcm_client_close(phandle);
	} else {
		pcm_process_client_close(phandle);
	}
}



void *AudioDspc::pcm_alloc_buffer(unsigned int size)
{

	bufsize = size;
	hShmBuf = AML_MEM_Allocate(bufsize);
	virbuf = AML_MEM_GetVirtAddr(hShmBuf);
	phybuf = AML_MEM_GetPhyAddr(hShmBuf);
	return virbuf;
}

void AudioDspc::pcm_free_buffer(void)
{
	AML_MEM_Free(hShmBuf);
}

int AudioDspc::pcm_read(unsigned char *buffer, unsigned int pcm_period)
{
	unsigned int size = 0;
#if DQBUF_MODE
	struct buf_info buf;
	AMX_UNUSED(pcm_period);
	pcm_process_client_dqbuf(phandle, &buf, NULL, PROCESSBUF);
	if (buf.size) {
		size = buf.size;
		memcpy(buffer, buf.viraddr, buf.size);
		pcm_process_client_qbuf(phandle, &buf, PROCESSBUF);
	}
#else
	size = pcm_process_client_readi(phandle, phybuf, pcm_period, PROCESSBUF);
	if (size) {
		AML_MEM_Invalidate(phybuf, size);
		if (buffer != virbuf)
			memcpy(buffer, virbuf, size);
	}
#endif

	return size;
}

int AudioDspc::pcm_write(unsigned char *buffer, unsigned int pcm_period)
{
	if (buffer != virbuf)
		memcpy(virbuf, buffer, bufsize);
	if (AML_MEM_Clean(phybuf, bufsize) < 0)
		return -1;
	if (pcm_client_writei(phandle, phybuf, pcm_period) < 0)
		return -1;

	return 0;
}

int AudioDspc::pcm_restore(void)
{
	return 0;
}

int AudioDspc::pcm_start(void)
{
	if (pmode == PCM_PLAYBACK) {

	} else {
		pcm_process_client_start(phandle);
	}
	return 0;
}

void AudioDspc::pcm_stop(void)
{
	if (pmode == PCM_PLAYBACK) {

	} else {
		pcm_process_client_stop(phandle);
	}
}
