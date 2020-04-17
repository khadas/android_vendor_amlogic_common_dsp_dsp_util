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
 * amlogic audio util API.
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AMLOGIC_AUDIO_UTIL_H_
#define _AMLOGIC_AUDIO_UTIL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>
void* AML_SRCS16LE_Init(int32_t in_rate, int32_t out_rate, uint32_t channel);

void AML_SRCS16LE_DeInit(void *h);

int AML_SRCS16LE_Exec(void *h,
                 int16_t *dst, size_t dst_frame,
                 int16_t *src, size_t src_frame);


void aprofiler_get_cur_timestamp(struct timespec* ts);
uint32_t aprofiler_msec_duration(struct timespec* tsEnd, struct timespec* tsStart);

#define TIC              \
    struct timespec bgn, end; \
    aprofiler_get_cur_timestamp(&bgn)

#define TOC                            \
    aprofiler_get_cur_timestamp(&end); \
    uint32_t ms = aprofiler_msec_duration(&end, &bgn)


#ifdef __cplusplus
}
#endif

#endif /* _AMLOGIC_AUDIO_UTIL_H_ */

