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
 * amlogic wake up engine api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rpc_client_aipc.h"
#include "rpc_client_vsp.h"
#include "rpc_client_shm.h"
#include "aml_wakeup_api.h"

struct _AWE {
	AML_VSP_HANDLE hVsp;
    AML_MEM_HANDLE hParam;
    size_t param_size;
	AML_MEM_HANDLE hInput;
	size_t input_size;
	AML_MEM_HANDLE hOutput;
	size_t output_size;
    int32_t micInChannels;
    int32_t refInChannels;
    int32_t outChannels;
};

typedef struct {
	int32_t    numInChn;
	int32_t    lenInByte;
	uint64_t   pChanIn[AWE_MAX_IN_CHANS];
	int32_t    numOutChn;
	int32_t    lenOutByte;
	uint64_t   pChanOut[AWE_MAX_OUT_CHANS];
} __attribute__((packed)) aml_vsp_awe_process_param_in;

typedef struct {
	int32_t    lenInByte;
	int32_t    lenOutByte;
	uint32_t   isWaked;
} __attribute__((packed)) aml_vsp_awe_process_param_out;

extern AWE_RET AML_AWE_GetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para);

AWE_RET AML_AWE_Create(AWE **awe)
{
    AWE *pawe = NULL;
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
    pawe = calloc(1, sizeof(AWE));
	if (!pawe) {
		printf("Calloc AWE structure failed\n");
        return AWE_RET_ERR_NO_MEM;
	}
	memset(pawe, 0, sizeof(AWE));
	pawe->hParam = AML_MEM_Allocate(sizeof(AWE_PARA));
	pawe->param_size = sizeof(AWE_PARA);
	pawe->hInput = AML_MEM_Allocate(sizeof(aml_vsp_awe_process_param_in));
	pawe->input_size = sizeof(aml_vsp_awe_process_param_in);
	pawe->hOutput = AML_MEM_Allocate(sizeof(aml_vsp_awe_process_param_out));
	pawe->output_size = sizeof(aml_vsp_awe_process_param_out);
	pawe->hVsp = AML_VSP_Init(AML_VSP_AWE, NULL, 0);

	if (pawe->hParam && pawe->hInput && pawe->hOutput && pawe->hVsp) {
		AWE_PARA* para = (AWE_PARA*)AML_MEM_GetVirtAddr(pawe->hParam);

		AML_VSP_GetParam(pawe->hVsp, AWE_PARA_MIC_IN_CHANNELS, AML_MEM_GetPhyAddr(pawe->hParam), pawe->param_size);
		AML_MEM_Invalidate(AML_MEM_GetPhyAddr(pawe->hParam), pawe->param_size);
		pawe->micInChannels = para->micInChannels;

		AML_VSP_GetParam(pawe->hVsp, AWE_PARA_REF_IN_CHANNELS, AML_MEM_GetPhyAddr(pawe->hParam), pawe->param_size);
		AML_MEM_Invalidate(AML_MEM_GetPhyAddr(pawe->hParam), pawe->param_size);
		pawe->refInChannels = para->refInChannels;


		AML_VSP_GetParam(pawe->hVsp, AWE_PARA_OUT_CHANNELS, AML_MEM_GetPhyAddr(pawe->hParam), pawe->param_size);
		AML_MEM_Invalidate(AML_MEM_GetPhyAddr(pawe->hParam), pawe->param_size);
		pawe->outChannels = para->outChannels;

		*awe = pawe;
		printf("Create AWE success: hVsp:%p hParam:%p hInput:%p hOutput:%p\n"
				"defaut channels: mic:%d, ref:%d, out:%d\n",
				pawe->hVsp, pawe->hParam, pawe->hInput, pawe->hOutput,
				pawe->micInChannels, pawe->refInChannels, pawe->outChannels);
		return AWE_RET_OK;
	} else {
		printf("Allocate AWE resource failed: hVsp:%p hParam:%p hInput:%p hOutput:%p\n",
				pawe->hVsp, pawe->hParam, pawe->hInput, pawe->hOutput);
		return AWE_RET_ERR_NO_MEM;
	}
}

AWE_RET AML_AWE_Destroy(AWE *awe)
{
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
	if (awe->hOutput)
		AML_MEM_Free(awe->hOutput);
	if (awe->hInput)
		AML_MEM_Free(awe->hInput);
	if (awe->hParam)
		AML_MEM_Free(awe->hParam);
	if (awe->hVsp)
		AML_VSP_Deinit(awe->hVsp);
	free(awe);
	return 0;
}

AWE_RET AML_AWE_Open(AWE *awe)
{
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
	return AML_VSP_Open(awe->hVsp);
}

AWE_RET AML_AWE_Close(AWE *awe)
{
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
	return AML_VSP_Close(awe->hVsp);
}

AWE_RET AML_AWE_SetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para)
{
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
	char* pParam = (char*)AML_MEM_GetVirtAddr(awe->hParam);
	memcpy(pParam, para, sizeof(AWE_PARA));
	AML_MEM_Clean(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
	return AML_VSP_SetParam(awe->hVsp, (int32_t)paraId, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
}

AWE_RET AML_AWE_GetParam(AWE *awe, AWE_PARA_ID paraId, AWE_PARA *para)
{
	AWE_RET ret = AWE_RET_OK;
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
	ret = AML_VSP_GetParam(awe->hVsp, (int32_t)paraId, AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
	AML_MEM_Invalidate(AML_MEM_GetPhyAddr(awe->hParam), awe->param_size);
	char* pParam = (char*)AML_MEM_GetVirtAddr(awe->hParam);
	memcpy(para, pParam, sizeof(AWE_PARA));
	return ret;
}


AWE_RET AML_AWE_Process(AWE *awe, AML_MEM_HANDLE in[], int32_t *inLenInByte, AML_MEM_HANDLE out[],
        int32_t *outLenInByte, uint32_t *isWaked)
{
	AWE_RET ret = AWE_RET_OK;
    if (!awe) {
		printf("Invalid param %s\n", __FUNCTION__);
        return AWE_RET_ERR_NULL_POINTER;
    }
	aml_vsp_awe_process_param_in* pParamIn = AML_MEM_GetVirtAddr(awe->hInput);
	aml_vsp_awe_process_param_out* pParamOut = AML_MEM_GetVirtAddr(awe->hOutput);
	size_t output_size = awe->output_size;

	pParamIn->lenInByte = *inLenInByte;
	pParamIn->numInChn = awe->micInChannels + awe->refInChannels;
	if (pParamIn->numInChn < AWE_MAX_IN_CHANS) {
		int i;
		for (i = 0; i < pParamIn->numInChn; i++)
			pParamIn->pChanIn[i] = (uint64_t)AML_MEM_GetPhyAddr(in[i]);
	} else {
		printf("The input channel number is out of AWE capability: mic:%d, ref:%d\n",
			awe->micInChannels, awe->refInChannels);
		return AWE_RET_ERR_NOT_SUPPORT;
	}

	pParamIn->lenOutByte = *outLenInByte;
	pParamIn->numOutChn = awe->outChannels;
	if (pParamIn->numOutChn < AWE_MAX_OUT_CHANS) {
		int i;
		for (i = 0; i < pParamIn->numOutChn; i++)
			pParamIn->pChanOut[i] = (uint64_t)AML_MEM_GetPhyAddr(out[i]);
	} else {
		printf("The output channel number is out of AWE capability: out:%d\n",
			awe->outChannels);
		return AWE_RET_ERR_NOT_SUPPORT;
	}

	AML_MEM_Clean(awe->hInput, awe->input_size);
	ret = AML_VSP_Process(awe->hVsp, AML_MEM_GetPhyAddr(awe->hInput), awe->input_size,
						AML_MEM_GetPhyAddr(awe->hOutput), &output_size);
	AML_MEM_Invalidate(awe->hOutput, output_size);
	*inLenInByte = pParamOut->lenInByte;
	*outLenInByte = pParamOut->lenOutByte;
	*isWaked = pParamOut->isWaked;
	return ret;
}


