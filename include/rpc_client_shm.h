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
 * shm for offload audio
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

typedef void* tAcodecShmHdl;
tAcodecShmHdl Aml_ACodecMemory_Allocate(size_t size);
void Aml_ACodecMemory_Free(tAcodecShmHdl hShm);
void Aml_ACodecMemory_Transfer(tAcodecShmHdl hDst, tAcodecShmHdl hSrc, size_t size);
void* Aml_ACodecMemory_GetVirtAddr(tAcodecShmHdl hShm);
void* Aml_ACodecMemory_GetPhyAddr(tAcodecShmHdl hShm);
uint32_t Aml_ACodecMemory_Clean(tAcodecShmHdl hShm, size_t size);
uint32_t Aml_ACodecMemory_Inv(tAcodecShmHdl hShm, size_t size);


typedef tAcodecShmHdl AML_MEM_HANDLE;
#define AML_MEM_Allocate Aml_ACodecMemory_Allocate
#define AML_MEM_Free Aml_ACodecMemory_Free
#define AML_MEM_GetVirtAddr Aml_ACodecMemory_GetVirtAddr
#define AML_MEM_GetPhyAddr Aml_ACodecMemory_GetPhyAddr
#define AML_MEM_Clean Aml_ACodecMemory_Clean
#define AML_MEM_Invalidate Aml_ACodecMemory_Inv

#ifdef __cplusplus
 }
#endif


#endif // end _OFFLOAD_ACODEC_MP3_H_
