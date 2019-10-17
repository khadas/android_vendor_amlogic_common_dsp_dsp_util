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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "aml_audio_util.h"

 typedef struct {
    // why it's signed number?
    // This SRC is for S16LE, signed number to avoid type conversation
    int32_t in_rate;
    int32_t out_rate;
    uint32_t channel;
    // 32bit is not enough, it's only good for playback in one day
    size_t in_base;
    size_t out_base;
    int16_t last[0]; // channel number elements
} srcs16le_t;

static inline int16_t linear_interp(int16_t a, int16_t b, int32_t num, int32_t den) {
    int32_t x = a, y = b;
    int32_t z = x + ((y - x) * num + den / 2) / den;
    int16_t c = z;
    return c;
}

void *AML_SRCS16LE_Init(int32_t in_rate, int32_t out_rate, uint32_t channel) {
    size_t len = sizeof(srcs16le_t) + sizeof(int16_t) * channel;
    srcs16le_t *p =(srcs16le_t *) malloc(len);
    if (p == NULL) {
        printf("fail to malloc size=%zu\n", len);
        return NULL;
    }
    p->in_rate = in_rate;
    p->out_rate = out_rate;
    p->channel = channel;
    p->in_base = p->out_base = 0;
    uint32_t i;
    for (i = 0; i != channel; i++) {
        p->last[i] = 0;
    }
    return (void*)p;
}

void AML_SRCS16LE_DeInit(void *h)
{
    free(h);
}

int AML_SRCS16LE_Exec(void *h,
                 int16_t *dst, size_t dst_frame,
                 int16_t *src, size_t src_frame)
{
    srcs16le_t *p = (srcs16le_t *)h;
    size_t i = p->out_base;
    uint32_t c;
    for (; ; i++) {
        size_t t = i * p->in_rate;
        size_t j = t / p->out_rate;
        if (j >= p->in_base) {
            break;
        }
        if (j + 1 != p->in_base) {
            printf("out of range for last\n");
            assert(0); // implementation issue if assert here
        }
        int32_t k = t % p->out_rate;
        // j -= p->in_base;
        // uint32_t u = p->channel * j; // j = in_base - 1, so u is meaningless
        uint32_t v = p->channel * (i - p->out_base);
        for (c = 0; c != p->channel; c++, v++) {
            dst[v] = linear_interp(p->last[c], src[c], k, p->out_rate);
        }
    }
    for (; ; i++) {
        size_t t = i * p->in_rate;
        size_t j = t / p->out_rate;
        j -= p->in_base;
        if (j + 1 >= src_frame) {
            break;
        }
        if (i >= p->out_base + dst_frame) {
            printf("out of range for output buffer\n");
            assert(0);
        }
        int32_t k = t % p->out_rate;
        uint32_t u = p->channel * j;
        uint32_t v = p->channel * (i - p->out_base);
        for (c = 0; c != p->channel; c++, u++, v++) {
            dst[v] = linear_interp(src[u], src[u + p->channel], k, p->out_rate);
        }
    }
    uint32_t r = i - p->out_base; // generated output sample at this time
    p->out_base = i;
    p->in_base += src_frame;
    for (c = 0; c != p->channel; c++) {
        p->last[c] = src[(src_frame - 1) * p->channel + c];
    }
    return r;
}

