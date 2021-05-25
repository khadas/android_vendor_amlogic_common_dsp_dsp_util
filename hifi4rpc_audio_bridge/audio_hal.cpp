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
 * Intermediate APIs that mask hardware differences
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
#include "audio_dspc.h"

#include "audio_hal.h"

using namespace std;

AudioHal::AudioHal(DEVICE_TYPE dev_t)
{
	dev_type = dev_t;
	if(dev_t == DEV_INTALSA) {
		aalsa = new AudioAlsa();
	} else {
		adspc = new AudioDspc();
	}
}

int AudioHal::pcm_open(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode, Audio_Hw_Param *pcm_hparam)
{
	if(dev_type == DEV_INTALSA) {
		return aalsa->pcm_open(pcm_hw, pcm_mode, pcm_hparam);
	} else {
		return adspc->pcm_open(pcm_hw, pcm_mode, pcm_hparam);
	}

}

void AudioHal::pcm_close(void)
{
	if(dev_type == DEV_INTALSA) {
		aalsa->pcm_close();
	} else {
		adspc->pcm_close();
	}
}


int AudioHal::pcm_read(unsigned char *buffer, unsigned int pcm_period)
{
	if(dev_type == DEV_INTALSA) {
		return aalsa->pcm_read(buffer, pcm_period);
	} else {
		return adspc->pcm_read(buffer, pcm_period);
	}
}

int AudioHal::pcm_write(unsigned char *buffer, unsigned int pcm_period)
{
	if(dev_type == DEV_INTALSA) {
		return aalsa->pcm_write(buffer, pcm_period);
	} else {
		return adspc->pcm_write(buffer, pcm_period);
	}
}

int AudioHal::pcm_restore(void)
{
	if(dev_type == DEV_INTALSA) {
		return aalsa->pcm_restore();
	} else {
		return adspc->pcm_restore();
	}
}

int AudioHal::pcm_start(void)
{
	if(dev_type == DEV_INTALSA) {
		return aalsa->pcm_start();
	} else {
		return adspc->pcm_start();
	}
}

void AudioHal::pcm_stop(void)
{
	if(dev_type == DEV_INTALSA) {
		aalsa->pcm_stop();
	} else {
		adspc->pcm_stop();
	}
}

void *AudioHal::pcm_alloc_buffer(unsigned int size)
{
	if(dev_type == DEV_INTALSA) {
		return aalsa->pcm_alloc_buffer(size);
	} else {
		return adspc->pcm_alloc_buffer(size);
	}
}

void AudioHal::pcm_free_buffer(void)
{
	if(dev_type == DEV_INTALSA) {
		aalsa->pcm_free_buffer();
	} else {
		adspc->pcm_free_buffer();
	}
}

