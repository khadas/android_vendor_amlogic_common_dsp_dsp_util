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
 * the audio debug
 *
 * Author: ziheng li <ziheng.li@amlogic.com>
 * Version:
 * - 0.1
 */

#ifndef __AUDIO_DEBUG_H
#define __AUDIO_DEBUG_H
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

#define DEBUG_LEVEL AUDIO_DEBUG

#define AUDIO_DEBUG 4
#define AUDIO_INFO 3
#define AUDIO_WARN 2
#define AUDIO_ERR 1
#define AUDIO_NONE 0

#define audio_debug(format, ...) AUDIO_PRINTF(AUDIO_DEBUG, format, ## __VA_ARGS__)
#define audio_info(format, ...) AUDIO_PRINTF(AUDIO_INFO, format, ## __VA_ARGS__)
#define audio_warn(format, ...) AUDIO_PRINTF(AUDIO_WARN, format, ## __VA_ARGS__)
#define audio_err(format, ...) AUDIO_PRINTF(AUDIO_ERR, format, ## __VA_ARGS__)


#define AUDIO_PRINTF(level, format, ...) \
	({ \
		if(level <= DEBUG_LEVEL) \
			printf(format, ## __VA_ARGS__); \
	})



#endif
