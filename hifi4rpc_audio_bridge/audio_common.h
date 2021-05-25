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
 * the audio common parameters
 *
 * Author: ziheng li <ziheng.li@amlogic.com>
 * Version:
 * - 0.1
 */

#ifndef __AUDIO_COMMON_H
#define __AUDIO_COMMON_H
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
#include "audio_debug.h"

enum DEVICE_TYPE {
	DEV_INTALSA = 0,
	DEV_DSPC
};


enum PCM_MODE {
	PCM_CARTURE = 0,
	PCM_PLAYBACK,
	PCM_NONE
};

enum PCM_DEVICE {
	PCM_DEV_TDMA = 0,
	PCM_DEV_TDMB,
	PCM_DEV_SPDIFIN,
	PCM_DEV_LOOPBACK,
	PCM_DEV_PDMIN,
	PCM_DEV_UVC,
	PCM_DEV_VAD
};

enum PCM_FORMAT {
	PCM_FOR_S8 = 0,
	PCM_FOR_U8,
	PCM_FOR_S16_LE,
	PCM_FOR_S16_BE,
	PCM_FOR_U16_LE,
	PCM_FOR_U16_BE,
	PCM_FOR_S24_LE,
	PCM_FOR_S24_BE,
	PCM_FOR_U24_LE,
	PCM_FOR_U24_BE,
	PCM_FOR_U24_3LE,
	PCM_FOR_U24_3BE,
	PCM_FOR_S32_LE,
	PCM_FOR_S32_BE,
	PCM_FOR_U32_LE,
	PCM_FOR_U32_BE,
};

typedef struct Audio_Hw_Param {
	unsigned int pcm_nchannels;
	unsigned int pcm_rate;
	PCM_FORMAT pcm_format;
	unsigned int pcm_byte;
	unsigned int pcm_period;
} Audio_Hw_Param;

#define a_sleep  sleep
#define a_usleep usleep

#endif
