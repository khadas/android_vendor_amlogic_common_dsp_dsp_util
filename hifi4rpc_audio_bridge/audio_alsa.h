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

#ifndef __AUDIO_ALSA_H
#define __AUDIO_ALSA_H
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
#include "audio_common.h"

#include "asoundlib.h"

using namespace std;

class AudioAlsa
{
public:
	int pcm_open(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode, Audio_Hw_Param * pcm_hparam);
	void pcm_close(void);
	int pcm_read(unsigned char *buffer, unsigned int pcm_period);
	int pcm_write(unsigned char *buffer, unsigned int pcm_period);
	int pcm_start(void);
	void pcm_stop(void);
	void *pcm_alloc_buffer(unsigned int size);
	void pcm_free_buffer(void);
	int pcm_restore(void);


private:
	void *virbuf;
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *hwparam;
	snd_pcm_sw_params_t *swparams;
	int pcm_byte;
	int pcm_period;
	int pcm_nchannels;
	int pcm_rate;

};


#endif
