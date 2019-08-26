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
 * offload voice signal processing api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <rpc_client_aipc.h>
#include "rpc_client_vsp.h"
#include "aipc_type.h"

struct tAmlVspCtx {
	tAmlVspRpcHdl vspsrvhdl;
	int aipchdl;
	int vsp_type;
};

AML_VSP_HANDLE AML_VSP_Init(int vsp_type, void* param, size_t param_size)
{
	vsp_init_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)malloc(sizeof(struct tAmlVspCtx));
	memset(pAmlVspCtx, 0, sizeof(struct tAmlVspCtx));
	pAmlVspCtx->aipchdl = xAudio_Ipc_init();
	memset(&arg, 0, sizeof(arg));
	arg.vsp_type = vsp_type;
	arg.param = (xpointer)param;
	arg.param_size = param_size;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_INIT, &arg, sizeof(arg));
	pAmlVspCtx->vspsrvhdl= arg.hdl;
	pAmlVspCtx->vsp_type = vsp_type;
	return (void*)pAmlVspCtx;
}

void AML_VSP_Deinit(AML_VSP_HANDLE hVsp)
{
	vsp_deinit_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)hVsp;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlVspRpcHdl)pAmlVspCtx->vspsrvhdl;
	arg.vsp_type = pAmlVspCtx->vsp_type;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_DEINIT, &arg, sizeof(arg));
	xAudio_Ipc_Deinit(pAmlVspCtx->aipchdl);
	free(pAmlVspCtx);
}

int  AML_VSP_Open(AML_VSP_HANDLE hVsp)
{
	vsp_open_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)hVsp;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlVspRpcHdl)pAmlVspCtx->vspsrvhdl;
	arg.vsp_type = pAmlVspCtx->vsp_type;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_OPEN, &arg, sizeof(arg));
	return arg.ret;
}

int  AML_VSP_Close(AML_VSP_HANDLE hVsp)
{
	vsp_close_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)hVsp;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlVspRpcHdl)pAmlVspCtx->vspsrvhdl;
	arg.vsp_type = pAmlVspCtx->vsp_type;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_CLOSE, &arg, sizeof(arg));
	return arg.ret;
}

int  AML_VSP_SetParam(AML_VSP_HANDLE hVsp, int32_t param_id, void* param, size_t param_size)
{
	vsp_setparam_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)hVsp;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlVspRpcHdl)pAmlVspCtx->vspsrvhdl;
	arg.vsp_type = pAmlVspCtx->vsp_type;
	arg.param_id = param_id;
	arg.param = (xpointer)param;
	arg.param_size = param_size;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_SETPARAM, &arg, sizeof(arg));
	return arg.ret;
}

int  AML_VSP_GetParam(AML_VSP_HANDLE hVsp, int32_t param_id, void* param, size_t param_size)
{
	vsp_getparam_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)hVsp;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlVspRpcHdl)pAmlVspCtx->vspsrvhdl;
	arg.vsp_type = pAmlVspCtx->vsp_type;	
	arg.param_id = param_id;
	arg.param = (xpointer)param;
	arg.param_size = param_size;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_GETPARAM, &arg, sizeof(arg));
	return arg.ret;
}

int  AML_VSP_Process(AML_VSP_HANDLE hVsp, void* input_buf, size_t input_size,
			void* output_buf, size_t* output_size)
{
	vsp_process_st arg;
	struct tAmlVspCtx* pAmlVspCtx = (struct tAmlVspCtx*)hVsp;
	memset(&arg, 0, sizeof(arg));
	arg.hdl = (tAmlVspRpcHdl)pAmlVspCtx->vspsrvhdl;
	arg.vsp_type = pAmlVspCtx->vsp_type;
	arg.input_buf = (xpointer)input_buf;
	arg.input_size = input_size;
	arg.output_buf = (xpointer)output_buf;
	arg.output_size = *output_size;
	xAIPC(pAmlVspCtx->aipchdl, MBX_CODEC_VSP_API_PROCESS, &arg, sizeof(arg));
	*output_size = arg.output_size;
	return arg.ret;
}

