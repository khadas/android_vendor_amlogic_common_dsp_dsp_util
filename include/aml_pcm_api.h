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
 * PCM API, used to capture HiFi Dsp processed PCM
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#define AM_PCM_DEBUG(...)
//#define AM_PCM_DEBUG printf


typedef void* AML_PCM_HANDLE;

enum aml_pcm_format {
    /** Signed, 24-bit (32-bit in memory), little endian */
    AML_PCM_FORMAT_S24_LE,
    /** Signed, 32-bit , little endian */
    AML_PCM_FORMAT_S32_LE,
    AML_PCM_FORMAT_MAX
};

struct aml_pcm_config {
    /** The number of channels in a frame */
    unsigned int channels;
    /** The number of frames per second */
    unsigned int rate;
    /** The number of frames in a period */
    unsigned int period_size;
    /** The number of periods in a PCM */
    unsigned int period_count;
    /** The sample format of a PCM */
    enum aml_pcm_format format;
};


/**
 * Create a pcm stream instance
 *
 * @param[in] HiFiA:0, HiFiB:1
 *
 * @param[in] It is useless so far, may be used to identify a kind of connection of xaf pipeline
 *
 * @param[in] It is useless so far, may be used to config block/non-block mode, PCM_IN/PCM_OUT
 *
 * @return NULL if fail
 */
AML_PCM_HANDLE AML_PCM_Open(unsigned int card, unsigned int device, unsigned int flags,
                                        const struct aml_pcm_config *config);

/**
 * Destroy a pcm stream instance
 *
 * @param[in] pcm instance handle
 *
 * @return
 */
void AML_PCM_Close(AML_PCM_HANDLE Handle);

/**
 * Read pcm, the call is blocked till user requested bytes are read out
 *
 * @param[in] Pcm instance handle
 *
 * @param[out] Buffer passed by user to read out pcm data
 *
 * @param[in] Number of bytes of the read
 *
 * @return It is equal to 3rd parameter count if the call is sccuess, otherwise please check error no
 */
int AML_PCM_Read(AML_PCM_HANDLE Handle, void *data, unsigned int count);

