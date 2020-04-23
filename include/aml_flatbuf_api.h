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
 * flat buffer api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */


#ifndef _AML_FLATBUF_API_
#define _AML_FLATBUF_API_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/** A flag that specifies that the flat buffer support write operation.
 */
#define FLATBUF_FLAG_WR 0x00000001

/** A flag that specifies that the flat buffer support read operation.
 */
#define FLATBUF_FLAG_RD 0x10000000

#define BUF_STR_ID_MAX 32
struct flatbuffer_config {
    /*Size of CC buffer*/
    size_t size;
};


typedef void* AML_FLATBUF_HANDLE;

/**
 * Create a flat buffer.
 * The handler is associated with a buffer string id,
 * If buffer string id already exists, return the already created handler.
 *
 * @param[in] buffer string id
 *
 * @param[in] flags, see the macro defined by FLATBUF_FLAG_XXX
 *
 * @param[in] flat buffer config, this param is ignored if buf_id already exists
 *
 * @return a flat buffer handler if success, otherwise return 0
 */
AML_FLATBUF_HANDLE AML_FLATBUF_Create(const char* buf_id, int flags,
                                    struct flatbuffer_config* config);

/**
 * Destroy a flat buffer
 *
 * @param[in] flat buffer handler
 *
 * @return
 */
void AML_FLATBUF_Destory(AML_FLATBUF_HANDLE hFbuf);

/**
 * Read data from flat buffer.
 *
 * The API blocks till all bytes are read, when msTimeout is equal to -1.
 * Time out mechanism is enabled when msTimeout is not equal to -1.
 * When time out is enabled, all bytes or part of bytes are read before time out.
 *
 * @param[in] flat buffer handler
 *
 * @param[out] read data into this buffer
 *
 * @param[in] size in bytes, to be read
 *
 * @param[in] time out in micro seconds
 *
 * @return real size of the read
 */
size_t AML_FLATBUF_Read(AML_FLATBUF_HANDLE hFbuf, void* buf, size_t size, int msTimeout);

/**
 * Write data to flat buffer
 *
 * The API blocks till all bytes are written when msTimeout is equal to -1
 * Time out mechanism is enabled when msTimeout is not equal to -1,
 * When time out is enabled, all bytes or part of bytes are written before time out
 *
 * @param[in] flat buffer handler
 *
 * @param[in] write data into this buffer
 *
 * @param[in] size in bytes, to be written
 *
 * @param[in] time out in micro seconds
 *
 * @return real size of the write
 */
size_t AML_FLATBUF_Write(AML_FLATBUF_HANDLE hFbuf, const void* buf, size_t size, int msTimeout);


/**
 * Get flat buffer fullness
 * .
 * @param[in] flat buffer handler
 *
 * @return flat buffer fullness in bytes
 */
size_t AML_FLATBUF_GetFullness(AML_FLATBUF_HANDLE hFbuf);

/**
 * Get flat buffer space
 *
 * @param[in] flat buffer handler
 *
 * @return flat buffer space in bytes
 */
size_t AML_FLATBUF_GetSpace(AML_FLATBUF_HANDLE hFbuf);

#ifdef __cplusplus
}
#endif


#endif
