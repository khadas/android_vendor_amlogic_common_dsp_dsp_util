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
 * hifi cbuf client api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_CBUF_H_
#define _RPC_CLIENT_CBUF_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/**
 *  Circular hanlder
 */
typedef void* AML_CBUF_HANDLE;

/**
 * Create a circular buffer
 *
 * @param[in] cc buffer size in bytes
 *
 * @param[in] cc buffer padding size, used to handle wrap around
 *
 * @return a valid cc buffer handler if success, otherwise return 0
 */
AML_CBUF_HANDLE AML_CBUF_Create(uint32_t cbuf_id, size_t size, size_t pad_size);

/**
 * Free a circular buffer
 *
 * @param[in] CC buffer handler
 *
 * @return
 */
void AML_CBUF_Destory(AML_CBUF_HANDLE hCbuf);


/**
 * Read data from circular buffer
 *
 * @param[in] Shared memory handler
 *
 * @param[out] Read data into this buffer
 *
 * @param[in] Size of data to be read
 *
 * @return real size of the read
 */
size_t AML_CBUF_Read(AML_CBUF_HANDLE hCbuf, void* buf, size_t size);


#ifdef __cplusplus
 }
#endif


#endif // end _OFFLOAD_ACODEC_MP3_H_
