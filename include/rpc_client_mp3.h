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
 * offload mp3 decoder
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_MP3_H_
#define _RPC_CLIENT_MP3_H_

#include "pvmp3decoder_api.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kInputBufferSize = 10 * 1024,
    kOutputBufferSize = 4608 * 2,
};

typedef void* tAmlMp3DecHdl;
typedef tPVMP3DecoderExternal tAmlACodecConfig_Mp3DecExternal;

tAmlMp3DecHdl AmlACodecInit_Mp3Dec(tAmlACodecConfig_Mp3DecExternal *pconfig, int bInplace);
void AmlACodecDeInit_Mp3Dec(tAmlMp3DecHdl hMp3Dec);
ERROR_CODE AmlACodecExec_Mp3Dec(tAmlMp3DecHdl hMp3, tAmlACodecConfig_Mp3DecExternal *pconfig);

#ifdef __cplusplus
}
#endif

#endif // end _OFFLOAD_ACODEC_MP3_H_
