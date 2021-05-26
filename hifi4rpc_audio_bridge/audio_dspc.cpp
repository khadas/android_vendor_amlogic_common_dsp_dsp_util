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

using namespace std;

#define DSPC_SAMPLE_RATE 16000
#define DSPC_PERIOD_SEC 10
#define DSPC_RAWDATA_CH_NUM 4
#define DSPC_PROCESSEDDATA_CH_NUM 2
#define DSPC_SAMPLE_BYTE 2

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
	for(i = 0; i < sizeof(aml_dsp_adev)/sizeof(struct AML_DSP_ADEVICE); i++)
	{
		if(aml_dsp_adev[i].value == ((pcm_hw << 1) | pcm_mode)){
			return aml_dsp_adev[i].ahw_id;
		}

	}
	return -1;
}

int find_dsp_format_node(PCM_FORMAT pcm_format)
{
	int ret = 0;
	switch(pcm_format)
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
	if(pcm_mode == PCM_PLAYBACK) {
		pconfig.channels = pcm_hparam->pcm_nchannels;
		pconfig.rate = pcm_hparam->pcm_rate;

		if((value = find_dsp_device_node(pcm_hw, pcm_mode)) < 0) {
			audio_err("not support this device(%d,%d)\n",pcm_hw,pcm_mode);
			return -1;
		}
		if((pconfig.format = find_dsp_format_node(pcm_hparam->pcm_format)) < 0) {
			audio_err("not support this format(%d,%d)\n",pcm_hparam->pcm_format);
			return -1;
		}

		pconfig.period_size = pcm_hparam->pcm_period;
		pconfig.period_count = 8;
		pconfig.start_threshold = pcm_hparam->pcm_period*4;
		pconfig.silence_threshold = pcm_hparam->pcm_period * 2;
		pconfig.stop_threshold = pcm_hparam->pcm_period * 2;

		if((pphandle = pcm_client_open(0, value, PCM_OUT, &pconfig)) == NULL) {
			audio_err("pcm client open fail\n");
			return -1;
		}

		pcm_hparam->pcm_period = pconfig.period_size;

	} else {

		phandle = xAudio_Ipc_init();
		uint32_t cmd = 0;
		xAIPC(phandle, MBX_CMD_FLATBUF_ARM2DSP_DSPC, &cmd, sizeof(uint32_t));
		xAIPC(phandle, MBX_CMD_FLATBUF_DSP2ARM_DSPC, &cmd, sizeof(uint32_t));

		memset(&proconfig, 0, sizeof(proconfig));
		proconfig.size = 256*DSPC_PROCESSEDDATA_CH_NUM*DSPC_SAMPLE_BYTE;
		proconfig.phy_ch = FLATBUF_CH_ARM2DSPA;
		prohFbuf = AML_FLATBUF_Create("DSP2ARM_DSPC_PROCESSEDDATA", FLATBUF_FLAG_RD, &proconfig);
		if (prohFbuf == NULL) {
			audio_err("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
			return -1;
		}

	}
	return 0;
}

void AudioDspc::pcm_close(void)
{
	if(pmode == PCM_PLAYBACK) {
		pcm_client_close(pphandle);
	} else {
		AML_FLATBUF_Destroy(prohFbuf);
		xAudio_Ipc_Deinit(phandle);
	}
}



void *AudioDspc::pcm_alloc_buffer(unsigned int size)
{

	if(pmode == PCM_PLAYBACK) {
		hShmBuf = AML_MEM_Allocate(size);
		virbuf = AML_MEM_GetVirtAddr(hShmBuf);
		phybuf = AML_MEM_GetPhyAddr(hShmBuf);
		return virbuf;
	} else {
		virbuf = (void *)malloc(size);
		return virbuf;
	}
}

void AudioDspc::pcm_free_buffer(void)
{

	if(pmode == PCM_PLAYBACK) {
		AML_MEM_Free(hShmBuf);
	} else {
		free(virbuf);
	}
}

int AudioDspc::pcm_read(unsigned char *buffer, unsigned int pcm_period)
{
	AMX_UNUSED(pcm_period);
	return AML_FLATBUF_Read(prohFbuf, buffer, proconfig.size, -1);
}

int AudioDspc::pcm_write(unsigned char *buffer, unsigned int pcm_period)
{
	if(buffer != virbuf)
		memcpy(virbuf, buffer, bufsize);
	if(AML_MEM_Clean(phybuf, bufsize) < 0)
		return -1;
	if(pcm_client_writei(pphandle, phybuf, pcm_period) < 0)
		return -1;

	return 0;
}

int AudioDspc::pcm_restore(void)
{
	return 0;
}

int AudioDspc::pcm_start(void)
{
	return 0;
}

void AudioDspc::pcm_stop(void)
{

}
