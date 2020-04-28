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
 * shm for transfering memory between arm and hifi
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_SHM_H_
#define _RPC_CLIENT_SHM_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/**
 *  Shared Memory hanlder
 */
typedef void* AML_MEM_HANDLE;

/**
 * Allocate a block of shared memory
 *
 * @param[in] Shared memory size in bytes
 *
 * @return a valid shared memeory handler if success, otherwise return 0
 */
AML_MEM_HANDLE AML_MEM_Allocate(size_t size);

/**
 * Free a block of shared memory
 *
 * @param[in] Shared memory handler
 *
 * @return
 */
void AML_MEM_Free(AML_MEM_HANDLE hShm);

/**
 * Copy data from a shared memory to another
 *
 * @param[in] Handler of dest shared memory
 *
 * @param[in] Handler of source shared memory
 *
 * @param[in] shared memory size
 *
 * @return
 */
void AML_MEM_Transfer(AML_MEM_HANDLE hDst, AML_MEM_HANDLE hSrc, size_t size);


/**
 * Recycle all shared memory allocated by the process
 *
 * @param[in] process id
 *
 * @return
 */
void AML_MEM_Recycle(int pid);

/**
 * Get the virtual address of a shared memory
 *
 * @param[in] Shared memory handler
 *
 * @return virtual address if successful, otherwise return NULL
 */
void* AML_MEM_GetVirtAddr(AML_MEM_HANDLE hShm);

/**
 * Get physical address of a shared memory
 *
 * @param[in] Shared memory handler
 *
 * @return physical address if successful, otherwise return 0
 */
void* AML_MEM_GetPhyAddr(AML_MEM_HANDLE hShm);

/**
 * Clean cache for a block of shared memory
 *
 * @param[in] Shared memory handler
 *
 * @param[in] The size need to be cleaned
 *
 * @return 0 if successful, return -1 if failed
 */
int32_t AML_MEM_Clean(AML_MEM_HANDLE hShm, size_t size);

/**
 * Invalidate cache for a block of shared memory
 *
 * @param[in] Shared memory handler
 *
 * @param[in] The size need to be invalidated
 *
 * @return 0 if successful, return -1 if failed
 */
int32_t AML_MEM_Invalidate(AML_MEM_HANDLE hShm, size_t size);

/**
 * Make a block of memory work as shared memory
 *
 * @param[in] physical base of memory block
 *
 * @param[in] The size of memory block
 *
 * @return valid shared memory handle if successful, return NULL if failed
 */
AML_MEM_HANDLE AML_MEM_Import(void* phy, size_t size);


#ifdef __cplusplus
 }
#endif


#endif // end _OFFLOAD_ACODEC_MP3_H_
