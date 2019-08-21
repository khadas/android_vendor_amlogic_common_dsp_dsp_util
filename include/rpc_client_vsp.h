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
 * offload voice signal process
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_VSP_H_
#define _RPC_CLIENT_VSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __VSP_COMPOSE_TYPE(mod, func)  (((mod) << 10) | ((func) & 0x3FF))
/*******************************************************************************
 * Define VSP Vendor ID here, 6bits valid
 ******************************************************************************/
#define AML_VSP          0x1
/*******************************************************************************
 * Define VSP type here, 10bits valid
 ******************************************************************************/
/*VSP TYPE*/
#define VSP_RESAMPLER       0x1
/*******************************************************************************
 * VSP type comopsition
 ******************************************************************************/
#define AML_VSP_RESAMPLER        __VSP_COMPOSE_TYPE(AML_VSP, VSP_RESAMPLER)



typedef void* AML_VSP_HANDLE;
AML_VSP_HANDLE AML_VSP_Init(int vsp_type, void* param, size_t param_size);
void AML_VSP_Deinit(AML_VSP_HANDLE hVsp);
int  AML_VSP_Process(AML_VSP_HANDLE hVsp,
								void* input_buf, size_t input_size,
								void* output_buf, size_t* output_size);



#ifdef __cplusplus
}
#endif

#endif // end _OFFLOAD_ACODEC_MP3_H_
