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
 * amlogic T buffer API.
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <stddef.h>
#include <stdint.h>

typedef void* AML_TBUF_HANDLE;

typedef enum {
    TBUF_READER_INV = 0,
    TBUF_READER_A = 1,
    TBUF_READER_B = 2,
    TBUF_READER_MAX = 3,
} TBUF_READER_T;

typedef enum {
    TBUF_RET_OK = 0,
    TBUF_RET_ERR_NO_MEM = -1,
    TBUF_RET_ERR_INV_PARAM = -1,
} TBUF_RET;

#define AML_TBUF_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
* Create a T buffer.
*
* @param[in] T buffer size
*
* @return a T buffer handle if success, otherwise return 0
*/
AML_TBUF_HANDLE AML_TBUF_Create(size_t size);

/**
* Destroy a T buffer
*
* @param[in] T buffer handle
*
* @return
*/
void AML_TBUF_Destory(AML_TBUF_HANDLE hTbuf);


/**
* Update write offset
*
* @param[in] T buffer handle
*
* @param[in] T buffer write offset is updated with this size in bytes
*
* @return TBUF_RET_OK if success, otherwise see TBUF_RET
*/
TBUF_RET AML_TBUF_UpdateWrOffset(AML_TBUF_HANDLE hTbuf, size_t size);

/**
* Update read offset
*
* @param[in] T buffer handle
*
* @param[in] reader A or reader B
*
* @param[in] T buffer reader B offset is updated with this size in bytes
*
* @return TBUF_RET_OK if success, otherwise see TBUF_RET
*/
TBUF_RET AML_TBUF_UpdateRdOffset(AML_TBUF_HANDLE hTbuf,  TBUF_READER_T rdType, size_t size);

/**
* Get address of write offset
*
* @param[in] T buffer handle
*
* @param[in] physical address of write offset
*
* @param[in] virtual address of write offset
*
* @return TBUF_RET_OK if success, otherwise see TBUF_RET
*/
TBUF_RET AML_TBUF_GetWrPtr(AML_TBUF_HANDLE hTbuf, void** phy, void** vir);

/**
* Get address of read offset
*
* @param[in] T buffer handle
*
* @param[in] reader A or reader B
*
* @param[in] physical address of write offset
*
* @param[in] virtual address of write offset
*
* @return TBUF_RET_OK if success, otherwise see TBUF_RET
*/
TBUF_RET AML_TBUF_GetRdPtr(AML_TBUF_HANDLE hTbuf, TBUF_READER_T rdType, void** phy, void** vir);

/**
* Get T buffer space
*
* @param[in] T buffer handle
*
* @param[out] T buffer space
*
* @return TBUF_RET_OK if success, otherwise see TBUF_RET
*/
TBUF_RET AML_TBUF_GetSpace(AML_TBUF_HANDLE hTbuf, size_t* szAvail);


/**
* Get T buffer space
*
* @param[in] T buffer handle
*
* @param[in] reader A or reader B
*
* @param[out] T buffer space
*
* @return TBUF_RET_OK if success, otherwise see TBUF_RET
*/
TBUF_RET AML_TBUF_GetFullness(AML_TBUF_HANDLE hTbuf, TBUF_READER_T rdType, size_t* szAvail);
