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

#include <stdint.h>

/** A flag that specifies that the flat buffer support write operation.
 */
#define FLATBUF_FLAG_WR 0x00000001

/** A flag that specifies that the flat buffer support read operation.
 */
#define FLATBUF_FLAG_RD 0x10000000

/* MASK to block caller, till all requested bytes are finished
 * Masking this flag simultenously in both side is not recommanded,
 * since this possibly cause dead lock
 */
#define FLATBUF_FLAG_BLOCK 0x00000002

#define BUF_STR_ID_MAX 32
struct flatbuffer_config {
    /* Physical address of the internal CC buffer
   * If user has pre-allocated a bufer, pass in its physical address.
   * Otherwise pass in NULL.
   */
    void* phy_addr;
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
AML_FLATBUF_HANDLE AML_FLATBUF_Create(char* buf_id, int flags,
                                    struct flatbuffer_config* config);

/**
 * Destory a flat buffer
 *
 * @param[in] flat buffer handler
 *
 * @return
 */
void AML_FLATBUF_Destory(AML_FLATBUF_HANDLE hFbuf);

/**
 * Read data from flat buffer.
 *
 * The API works in unblock mode by default.
 * In unblock mode, the call returns if it can not read any more.
 * Part of bytes may be read, the return value is real size of read.
 *
 * The API works in block mode, if the flags parameter is masked as FLATBUF_FLAG_BLOCK.
 * In block mode, the call blocks there till all requested bytes are read.
 *
 * @param[in] flat buffer handler
 *
 * @param[out] read data into this buffer
 *
 * @param[in] size in bytes, to be read
 *
 * @return real size of the read
 */
size_t AML_FLATBUF_Read(AML_FLATBUF_HANDLE hFbuf, void* buf, size_t size);


/**
 * Write data to flat buffer
 *
 * The API works in unblock mode by default.
 * In unblock mode, the call returns if it can not write any more.
 * Part of bytes may be written, the return value is real size of write.
 *
 * The API works in block mode, if the flags is masked as FLATBUF_FLAG_BLOCK.
 * In block mode, the call blocks there till all requested bytes are written.
 *
 * @param[in] flat buffer handler
 *
 * @param[in] write data into this buffer
 *
 * @param[in] size in bytes, to be written
 *
 * @return real size of the write
 */
size_t AML_FLATBUF_Write(AML_FLATBUF_HANDLE hFbuf, void* buf, size_t size);

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
size_t AML_FALTBUF_GetSpace(AML_FLATBUF_HANDLE hFbuf);


#endif
