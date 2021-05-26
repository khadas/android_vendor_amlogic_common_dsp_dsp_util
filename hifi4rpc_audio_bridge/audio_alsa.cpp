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
 * linux alsa operation api
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
#include "audio_alsa.h"

#include "asoundlib.h"

using namespace std;

struct AML_ALSA_DEVICE {
	int value;
	char *hw_name;
};

const struct AML_ALSA_DEVICE aml_adev[] =
{
	{((PCM_DEV_TDMA << 1) | PCM_CARTURE), (char *)"hw:0,0"},
	{((PCM_DEV_TDMA << 1) | PCM_PLAYBACK), (char *)"hw:0,0"},
	{((PCM_DEV_TDMB << 1) | PCM_CARTURE), (char *)"hw:0,1"},
	{((PCM_DEV_TDMB << 1) | PCM_PLAYBACK), (char *)"hw:0,1"},
	{((PCM_DEV_SPDIFIN << 1) | PCM_CARTURE), (char *)"hw:0,3"},
	{((PCM_DEV_SPDIFIN << 1) | PCM_PLAYBACK), (char *)"hw:0,3"},
	{((PCM_DEV_LOOPBACK << 1) | PCM_CARTURE), (char *)"hw:0,4"},
	{((PCM_DEV_PDMIN << 1) | PCM_CARTURE), (char *)"hw:0,2"},
	{((PCM_DEV_UVC << 1) | PCM_PLAYBACK), (char *)"hw:1,0"},
	{((PCM_DEV_UVC << 1) | PCM_CARTURE), (char *)"hw:1,0"},

};

int find_alsa_device_node(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode, char *hw_name)
{
	int i;
	for(i = 0; i < sizeof(aml_adev)/sizeof(struct AML_ALSA_DEVICE); i++)
	{
		if(aml_adev[i].value == ((pcm_hw << 1) | pcm_mode)){
			strcpy(hw_name, aml_adev[i].hw_name);
			return 0;
		}

	}
	return -1;
}

int find_alsa_format_node(PCM_FORMAT pcm_format)
{
	int ret = 0;
	switch(pcm_format)
	{
		case PCM_FOR_S8: ret = SND_PCM_FORMAT_S8;
			break;
		case PCM_FOR_S16_LE: ret = SND_PCM_FORMAT_S16_LE;
			break;
		case PCM_FOR_S16_BE: ret = SND_PCM_FORMAT_S16_BE;
			break;
		case PCM_FOR_S24_LE: ret = SND_PCM_FORMAT_S24_LE;
			break;
		case PCM_FOR_S24_BE: ret = SND_PCM_FORMAT_S24_BE;
			break;
		case PCM_FOR_U24_3LE: ret = SND_PCM_FORMAT_S24_3LE;
			break;
		case PCM_FOR_U24_3BE: ret = SND_PCM_FORMAT_S24_3BE;
			break;
		case PCM_FOR_S32_LE: ret = SND_PCM_FORMAT_S32_LE;
			break;
		case PCM_FOR_S32_BE: ret = SND_PCM_FORMAT_S32_BE;
			break;
		default:
			ret = -1;
			break;

	}

	return ret;
}

int AudioAlsa::pcm_open(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode, Audio_Hw_Param *pcm_hparam)
{
	int ret;
	char hw_name[10];
	snd_pcm_stream_t pmode;
	snd_pcm_format_t format;
	unsigned int rate;
	static snd_output_t *log;
	pcm_mode == PCM_CARTURE ? pmode = SND_PCM_STREAM_CAPTURE : pmode = SND_PCM_STREAM_PLAYBACK;
	ret = find_alsa_device_node(pcm_hw, pcm_mode, hw_name);
	if (ret < 0) {
		audio_err("Audio device or mode not support\n");
		return -1;
	}

	format = (snd_pcm_format_t)find_alsa_format_node(pcm_hparam->pcm_format);
	if (format < 0) {
		audio_err("Audio format not support\n");
		return -1;
	}

	ret = snd_pcm_open(&pcm_handle, hw_name, pmode, 0);
	if (ret < 0) {
		audio_err("Audio open error: %s\n", snd_strerror(ret));
		return -1;
	}

	// alloc param space of pcm hardware.
	snd_pcm_hw_params_alloca(&hwparam);
	if(hwparam == NULL) {
		audio_err("Alloc param space of pcm hardware fail\n");
		return -1;
	}

	// set param of default value.
	ret = snd_pcm_hw_params_any(pcm_handle, hwparam);
	if (ret < 0) {
		audio_err("Broken configuration for this PCM: no configurations available\n");
		return -1;
	}

	// set pcm interleaved mode.
	ret = snd_pcm_hw_params_set_access(pcm_handle, hwparam, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		audio_err("Access type not available\n");
		return -1;
	}

	// set pcm format.
	ret = snd_pcm_hw_params_set_format(pcm_handle, hwparam, format);
	if (ret < 0) {
		audio_err("Sample format non available\n");
		return -1;
	}

	// set the number of channels.
	ret = snd_pcm_hw_params_set_channels(pcm_handle, hwparam, pcm_hparam->pcm_nchannels);
	if (ret < 0) {
		audio_err("Channels count non available\n");
		return -1;
	}

	// set sampling rate.
	rate = pcm_hparam->pcm_rate;
	ret = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparam, &pcm_hparam->pcm_rate, 0);
	if (ret < 0) {
		audio_warn("Warning: rate is not accurate (requested = %iHz, got = %iHz)\n", rate, pcm_hparam->pcm_rate);
	}

	// set period.
	ret = snd_pcm_hw_params_set_period_size(pcm_handle, hwparam, (snd_pcm_uframes_t)pcm_hparam->pcm_period, 0);
	if (ret < 0) {
		audio_err("Period size non available\n");
		return -1;
	}

	// enable the param of pcm hardware.
	ret = snd_pcm_hw_params(pcm_handle,hwparam);
	if (ret < 0) {
		audio_err("Unable to install hw params:\n");
		snd_pcm_hw_params_dump(hwparam, log);
		return -1;
	}

	// set sw
	snd_pcm_sw_params_alloca(&swparams);
	if(swparams == NULL) {
		audio_err("Alloc param space of pcm software fail\n");
		return -1;
	}

	ret = snd_pcm_sw_params_current(pcm_handle, swparams);
	if (ret < 0) {
		audio_err("Unable to get current sw params.\n");
		return -1;
	}

#if 0
	ret = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, 0);
	if (ret < 0) {
		audio_err("Set available min non available\n");
		return -1;
	}

	ret = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, pcm_hparam->pcm_period * 3);
	if (ret < 0) {
		audio_err("Set start threshold non available\n");
		return -1;
	}

	ret = snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, pcm_hparam->pcm_period);
	if (ret < 0) {
		audio_err("Set stop threshold non available\n");
		return -1;
	}

#endif

	ret = snd_pcm_sw_params(pcm_handle, swparams);
	if (ret < 0) {
		audio_err("unable to install sw params:\n");
		snd_pcm_sw_params_dump(swparams, log);
		return -1;
	}

	pcm_byte = pcm_hparam->pcm_byte;
	pcm_period = pcm_hparam->pcm_period;
	pcm_nchannels = pcm_hparam->pcm_nchannels;
	pcm_rate = pcm_hparam->pcm_rate;
	return 0;
}

void AudioAlsa::pcm_close(void)
{
	//destroy pcm handle.
	snd_pcm_drain(pcm_handle);
	//close pcm device.
	snd_pcm_close(pcm_handle);
}


int AudioAlsa::pcm_read(unsigned char *buffer, unsigned int pcm_period)
{
	return snd_pcm_readi(pcm_handle, buffer, (snd_pcm_uframes_t)pcm_period) * pcm_nchannels * pcm_byte;
}

int AudioAlsa::pcm_write(unsigned char *buffer, unsigned int pcm_period)
{
	int ret = 0;
	if ((ret = snd_pcm_writei(pcm_handle, buffer, (snd_pcm_uframes_t)pcm_period)) < 0) {
		audio_err("<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
	}

	return ret;
}

int AudioAlsa::pcm_restore(void)
{
	int ret = 0;
	if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
		audio_err("xrun: prepare error: %s \n", snd_strerror(ret));
	}

	return ret;
}

int AudioAlsa::pcm_start(void)
{
	return 0;
}

void AudioAlsa::pcm_stop(void)
{

}

void *AudioAlsa::pcm_alloc_buffer(unsigned int size)
{
	virbuf = (void *)malloc(size);
	return virbuf;
}

void AudioAlsa::pcm_free_buffer(void)
{
	free(virbuf);
}

