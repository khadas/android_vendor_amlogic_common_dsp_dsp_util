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


#ifndef __AUDIO_DSPC_H
#define __AUDIO_DSPC_H
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

#include <endian.h>
#include <getopt.h>
#include <time.h>

#include "ipc_cmd_type.h"
#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "aml_flatbuf_api.h"
#include "rpc_client_mp3.h"
#include "rpc_client_pcm.h"
#include "generic_macro.h"

#include "pcm.h"

using namespace std;

class AudioDspc
{
public:
	int pcm_open(PCM_DEVICE pcm_hw, PCM_MODE pcm_mode, Audio_Hw_Param * pcm_hparam);
	void pcm_close(void);
	void *pcm_alloc_buffer(unsigned int size);
	void pcm_free_buffer(void);
	int pcm_read(unsigned char *buffer, unsigned int pcm_period);
	int pcm_write(unsigned char *buffer, unsigned int pcm_period);
	int pcm_start(void);
	void pcm_stop(void);
	int pcm_restore(void);

private:
	tAmlPcmhdl pphandle;
	PCM_MODE pmode;
	rpc_pcm_config pconfig;
	int phandle;
	AML_MEM_HANDLE hShmBuf;
	unsigned int bufsize;
	void *virbuf;
	void *phybuf;

	AML_FLATBUF_HANDLE prohFbuf;
	struct flatbuffer_config proconfig;
};


#endif
