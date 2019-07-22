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
 * audio rpc shm api implementation
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include "aipc_type.h"
#include "rpc_client_shm.h"

tAcodecShmHdl Aml_ACodecMemory_Allocate(size_t size)
{
	acodec_shm_alloc_st arg;
	arg.size = size;
	xAIPC(MBX_CMD_SHM_ALLOC, &arg, sizeof(arg));
	return (tAcodecShmHdl)arg.hShm;
}

void Aml_ACodecMemory_Free(tAcodecShmHdl hShm)
{
	acodec_shm_free_st arg;
	arg.hShm = (tAcodecShmHdlRpc)hShm;
	xAIPC(MBX_CMD_SHM_FREE, &arg, sizeof(arg));
}

uint32_t Aml_ACodecMemory_Clean(tAcodecShmHdl phy, size_t size)
{
	return 0;
}

uint32_t Aml_ACodecMemory_Inv(tAcodecShmHdl phy, size_t size)
{
	return 0;
}


