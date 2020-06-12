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
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "aml_audio_util.h"


/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 16000 Hz

* 0 Hz - 4000 Hz
  gain = 0.95
  desired ripple = 0.95 dB
  actual ripple = 0.7063731418184018 dB

* 4500 Hz - 8000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.08861272032049 dB

*/
#define SAMPLEFILTER_TAP_NUM 51
typedef struct {
    int16_t history[SAMPLEFILTER_TAP_NUM];
    unsigned int last_index;
} SampleFilter;

static double filter_taps[SAMPLEFILTER_TAP_NUM] = {
    0.00443527506731185,
    0.015266243571668382,
    0.004448228244326299,
    -0.0070940573928954965,
    -0.0024620044647436687,
    0.008850798254593152,
    0.0006779833808161324,
    -0.010864227866840034,
    0.001362227194551022,
    0.013130703138357597,
    -0.004216268474728633,
    -0.015487951462673441,
    0.008198817193046187,
    0.01782747144321327,
    -0.013764366970325683,
    -0.020071694547932974,
    0.021627574036802884,
    0.02205980181770327,
    -0.03340318735504971,
    -0.02373410547100674,
    0.05320995865526289,
    0.024999232696272057,
    -0.09631989907812818,
    -0.02578275373308752,
    0.30088296909927,
    0.501054191627973,
    0.30088296909927,
    -0.02578275373308752,
    -0.09631989907812818,
    0.024999232696272057,
    0.05320995865526289,
    -0.02373410547100674,
    -0.03340318735504971,
    0.02205980181770327,
    0.021627574036802884,
    -0.020071694547932974,
    -0.013764366970325683,
    0.01782747144321327,
    0.008198817193046187,
    -0.015487951462673441,
    -0.004216268474728633,
    0.013130703138357597,
    0.001362227194551022,
    -0.010864227866840034,
    0.0006779833808161324,
    0.008850798254593152,
    -0.0024620044647436687,
    -0.0070940573928954965,
    0.004448228244326299,
    0.015266243571668382,
    0.00443527506731185
};

void SampleFilter_init(SampleFilter* f) {
  int i;
  for(i = 0; i < SAMPLEFILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;
}

void SampleFilter_put(SampleFilter* f, double input) {
  f->history[f->last_index++] = input;
  if(f->last_index == SAMPLEFILTER_TAP_NUM)
    f->last_index = 0;
}

double SampleFilter_get(SampleFilter* f) {
  double acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < SAMPLEFILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : SAMPLEFILTER_TAP_NUM-1;
    acc += f->history[index] * filter_taps[i];
  };
  return acc;
}

typedef struct {
    // why it's signed number?
    // This SRC is for S16LE, signed number to avoid type conversation
    int32_t in_rate;
    int32_t out_rate;
    uint32_t channel;
    // 32bit is not enough, it's only good for playback in one day
    size_t in_base;
    size_t out_base;
    int16_t bEnableFir;
    int16_t* pFilteredSignal;
    SampleFilter f;
    int16_t last[0]; // channel number elements
} srcs16le_t;

static inline int16_t linear_interp(int16_t a, int16_t b, int32_t num, int32_t den) {
    int32_t x = a, y = b;
    int32_t z = x + ((y - x) * num + den / 2) / den;
    int16_t c = z;
    return c;
}

static uint32_t gcd(uint32_t a, uint32_t b) {
    if (a == 0 && b == 0) {
        return 0;
    }
    uint32_t t;
    while (1) {
        if (b == 0) {
            return a;
        }
        t = a;
        a = b;
        b = t % a;
    }
    printf("gcd implementation issue\n");
}

void *AML_SRCS16LE_Init(int32_t in_rate, int32_t out_rate, uint32_t channel) {
    size_t len = sizeof(srcs16le_t) + sizeof(int16_t) * channel;
    srcs16le_t *p =(srcs16le_t *) malloc(len);
    if (p == NULL) {
        printf("fail to malloc size=%zu\n", len);
        return NULL;
    }
    if (in_rate == 16000 && out_rate == 8000) {
        p->bEnableFir = 1;
        SampleFilter_init(&(p->f));
        p->pFilteredSignal = malloc(sizeof(int16_t));
    }
    else
        p->bEnableFir = 0;

    int32_t g = gcd(in_rate, out_rate);
    p->in_rate = in_rate / g;
    p->out_rate = out_rate / g;
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
    srcs16le_t *p = (srcs16le_t *)h;
    if (p->bEnableFir)
        free(p->pFilteredSignal);
    free(h);
}

int AML_SRCS16LE_Exec(void *h,
                 int16_t *dst, size_t dst_frame,
                 int16_t *src, size_t src_frame)
{
    srcs16le_t *p = (srcs16le_t *)h;
    uint64_t i = p->out_base;
    uint32_t c;
    if (p->bEnableFir) {
        int idx;
        p->pFilteredSignal = (int16_t*)realloc((void *)p->pFilteredSignal, sizeof(int16_t)*src_frame);
        for (idx = 0; idx < src_frame; idx++) {
            SampleFilter_put(&(p->f), (double)src[idx]);
            p->pFilteredSignal[idx] = (int16_t)SampleFilter_get(&(p->f));
        }
    } else {
        p->pFilteredSignal = src;
    }
    for (; ; i++) {
        uint64_t t = i * p->in_rate;
        size_t j = t / p->out_rate;
        if (j >= p->in_base) {
            break;
        }
        if (j + 1 != p->in_base) {
            printf("out of range for last: %"PRIu64"  %zu %" PRIu64 " %d %d %zu\n",
                i, j, t, p->in_rate, p->out_rate, p->in_base);
            assert(0); // implementation issue if assert here
        }
        int32_t k = t % p->out_rate;
        // j -= p->in_base;
        // uint32_t u = p->channel * j; // j = in_base - 1, so u is meaningless
        uint32_t v = p->channel * (i - p->out_base);
        for (c = 0; c != p->channel; c++, v++) {
            dst[v] = linear_interp(p->last[c], p->pFilteredSignal[c], k, p->out_rate);
        }
    }
    for (; ; i++) {
        uint64_t t = i * p->in_rate;
        size_t j = t / p->out_rate;
        j -= p->in_base;
        if (j + 1 >= src_frame) {
            break;
        }
        if (i >= p->out_base + dst_frame) {
            printf("out of range for output buffer: %"PRIu64" %zu %"PRIu64"%d %d %zu %zu\n",
                i, j, t, p->in_rate, p->out_rate, p->out_base, dst_frame);
            assert(0);
        }
        int32_t k = t % p->out_rate;
        uint32_t u = p->channel * j;
        uint32_t v = p->channel * (i - p->out_base);
        for (c = 0; c != p->channel; c++, u++, v++) {
            dst[v] = linear_interp(p->pFilteredSignal[u], p->pFilteredSignal[u + p->channel], k, p->out_rate);
        }
    }
    uint32_t r = i - p->out_base; // generated output sample at this time
    p->out_base = i;
    p->in_base += src_frame;
    for (c = 0; c != p->channel; c++) {
        p->last[c] = p->pFilteredSignal[(src_frame - 1) * p->channel + c];
    }
    return r;
}

#define MSECS_PER_SEC (1000L)
#define NSECS_PER_MSEC (1000000L)

void aprofiler_get_cur_timestamp(struct timespec* ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
    return;
}

int32_t aprofiler_msec_duration(struct timespec* tsEnd, struct timespec* tsStart)
{
    uint32_t uEndMSec = (uint32_t)(tsEnd->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsEnd->tv_nsec/NSECS_PER_MSEC);
    uint32_t uStartMSec = (uint32_t)(tsStart->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsStart->tv_nsec/NSECS_PER_MSEC);
    //printf("uEndMSec:%d, uStartMSec:%d\n", uEndMSec, uStartMSec);
    return (uEndMSec - uStartMSec);
}

bool anystr(const char *s, char **a) {
    int i;
    for (i = 0; a[i] != NULL; i++) {
        if (strcasecmp(s, a[i]) == 0) {
            return true;
        }
    }
    return false;
}

int str2id(char *s, IdGroup *g, int n) {
    int i;
    for (i = 0; i != n; i++) {
        if (anystr(s, g[i].a)) {
            return g[i].k;
        }
    }
    return -1;
}

int str2hifiId(char *s) {
    static const IdGroup g[] = {
        {0, {"0", "a", NULL}},
        {1, {"1", "b", NULL}}
    };
    int id = str2id(s, g, sizeof(g) / sizeof(g[0]));
    if (id == -1) {
        printf("unknown hifi id=%s\n", s);
        exit(-1);
    }
    return id;
}
