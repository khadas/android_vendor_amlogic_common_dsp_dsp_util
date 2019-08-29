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

typedef void* AML_MEM_HANDLE;
AML_MEM_HANDLE AML_MEM_Allocate(size_t size);
void AML_MEM_Free(AML_MEM_HANDLE hShm);
void Aml_ACodecMemory_Transfer(AML_MEM_HANDLE hDst, AML_MEM_HANDLE hSrc, size_t size);
void* AML_MEM_GetVirtAddr(AML_MEM_HANDLE hShm);
void* AML_MEM_GetPhyAddr(AML_MEM_HANDLE hShm);
uint32_t AML_MEM_Clean(AML_MEM_HANDLE hShm, size_t size);
uint32_t AML_MEM_Invalidate(AML_MEM_HANDLE hShm, size_t size);

typedef AML_MEM_HANDLE tAcodecShmHdl;
#define Aml_ACodecMemory_Allocate AML_MEM_Allocate
#define Aml_ACodecMemory_Free AML_MEM_Free
#define Aml_ACodecMemory_GetVirtAddr AML_MEM_GetVirtAddr
#define Aml_ACodecMemory_GetPhyAddr AML_MEM_GetPhyAddr
#define Aml_ACodecMemory_Clean AML_MEM_Clean
#define Aml_ACodecMemory_Inv AML_MEM_Invalidate

#ifdef __cplusplus
 }
#endif


#endif // end _OFFLOAD_ACODEC_MP3_H_
