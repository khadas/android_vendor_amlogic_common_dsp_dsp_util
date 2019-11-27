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
 * voice signal process API
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

typedef void* AML_VSP_HANDLE;

/**
 * Create and initialize an instance context according to vsp id
 *
 * @param[in] vsp id
 *
 * @param[in] static parameter used to initialize instance context
 *
 * @param[in] length of static parameter
 *
 * @return instance handler if successful, otherwise return NULL
 */
AML_VSP_HANDLE AML_VSP_Init(char* vsp_id, void* param, size_t param_size);

/**
 * Destory and deinitialize instance context.
 *
 * @param[in] instance handler
 *
 * @return
 */
void AML_VSP_Deinit(AML_VSP_HANDLE hVsp);

/**
 * Enable instance, make instance start to work
 * Static parameter can not be configured any more after this call
 *
 * @param[in] instance handler
 *
 * @return error codes
 */
int  AML_VSP_Open(AML_VSP_HANDLE hVsp);

/**
 * Disable instance, make instance stop to work
 * Static parameter can be re-configured after this call
 *
 * @param[in] instance handler
 *
 * @return error codes
 */
int  AML_VSP_Close(AML_VSP_HANDLE hVsp);

/**
 * Configure parameter
 * Static parameter can be configured only after instance stops
 * Dynamic parameter can be configured at run time
 *
 * @param[in] instance handler
 *
 * @param[in] parameter id
 *
 * @param[in] buffer carrying parameter
 *
 * @param[in] length of parameter
 *
 * @return error codes
 */
int  AML_VSP_SetParam(AML_VSP_HANDLE hVsp, int32_t param_id, void* param, size_t param_size);

/**
 * Obtain parameter
 *
 * @param[in] instance handler
 *
 * @param[in] parameter id
 *
 * @param[in] buffer carrying parameter
 *
 * @param[in] length of parameter
 *
 * @return error codes
 */
int  AML_VSP_GetParam(AML_VSP_HANDLE hVsp, int32_t param_id, void* param, size_t param_size);


/**
* Main data processing entry.
*
* @param[in] instance handler
*
* @param[in] Buffer carrying input data/meta
*
* @param[in] Input buffer size
*
* @param[in/out] Buffer carrying output data/meta
*
* @param[in/out] Space of buffer before the call
*                Size of output data/meta after the call
*
* @return error codes
*/
int  AML_VSP_Process(AML_VSP_HANDLE hVsp, void* input, size_t input_size,
                            void* output, size_t* output_size);



#ifdef __cplusplus
}
#endif

#endif // end _RPC_CLIENT_VSP_H_
