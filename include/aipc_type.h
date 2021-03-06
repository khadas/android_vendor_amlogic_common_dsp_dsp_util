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
 * data type for audio rpc
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AIPC_TYPE_H_
#define _AIPC_TYPE_H_

#include <stdint.h>
#include "ipc_cmd_type.h"
#include "aml_flatbuf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*rpc dummy*/
typedef struct {
	int32_t	  task_id;
	uint32_t  task_sleep_ms;
	int32_t	  func_code;
	int32_t   input_param;
	int32_t   output_param;
} __attribute__((packed)) tAmlDummyRpc;

/*mp3 decoder*/
typedef xpointer tAmlMp3DecRpcHdl;
typedef struct {
	int32_t	  inputBufferCurrentLength;
	int32_t	  inputBufferUsedLength;
	uint32_t	   CurrentFrameLength;
	uint32_t	   equalizerType;
	int32_t	  inputBufferMaxLength;
	int32_t		num_channels;
	int32_t		version;
	int32_t		samplingRate;
	int32_t		bitRate;
	int32_t	  outputFrameSize;
	int32_t	  crcEnabled;
	uint32_t	   totalNumberOfBitsUsed;
	xpointer	pOutputBuffer;
	xpointer	pInputBuffer;
} __attribute__((packed)) tAmlACodecConfig_Mp3DecRpc;

typedef struct {
    tAmlACodecConfig_Mp3DecRpc config;
    tAmlMp3DecRpcHdl hdl;
} __attribute__((packed)) mp3_init_st;

typedef struct {
    tAmlACodecConfig_Mp3DecRpc config;
    tAmlMp3DecRpcHdl hdl;
	uint32_t ret;
} __attribute__((packed)) mp3_decode_st;

typedef struct {
    tAmlMp3DecRpcHdl hdl;
} __attribute__((packed)) mp3_deinit_st;


/*aac decoder*/
typedef xpointer tAmlAacDecRpcHdl;
typedef struct {
    uint32_t transportFmt;
    uint32_t nrOfLayers;
    tAmlAacDecRpcHdl hdl;
} __attribute__((packed)) aacdec_init_st;


typedef struct {
	uint32_t sampleRate;
	uint32_t frameSize;
	uint32_t channels;
	uint8_t channel_counts[0x24];
} __attribute__((packed)) aacdec_out_ctx_st;

typedef struct {
    tAmlAacDecRpcHdl hdl;
    xpointer input_buf;
	uint32_t input_size;
    xpointer output_buf;
	uint32_t output_size;
	uint32_t input_left_size;
	aacdec_out_ctx_st out_ctx;
	uint32_t ret;
} __attribute__((packed)) aacdec_decode_st;

typedef struct {
    tAmlAacDecRpcHdl hdl;
} __attribute__((packed)) aacdec_deinit_st;

typedef struct {
	int32_t param;
	int32_t value;
    tAmlAacDecRpcHdl hdl;
	uint32_t ret;
} __attribute__((packed)) aacdec_setparam_st;


typedef struct {
	xpointer buf;
	int32_t size;
    tAmlAacDecRpcHdl hdl;
	uint32_t ret;
} __attribute__((packed)) aacdec_ancinit_st;

#define MAX_CONFRAW_LENGTH 1024
#define MAX_NUM_CONF 16
typedef struct {
	xpointer conf;
	int32_t length[MAX_NUM_CONF];
	int32_t num_conf;
    tAmlAacDecRpcHdl hdl;
	uint32_t ret;
} __attribute__((packed)) aacdec_cfgraw_st;

/*Voice Signal Processing*/
typedef xpointer tAmlVspRpcHdl;
typedef uint64_t xsize_t;
#define VSP_ID_MAX 32
typedef struct {
    char vsp_id[VSP_ID_MAX];
    tAmlVspRpcHdl hdl;
    uint32_t ret;
    xpointer param;
    xsize_t param_size;
    int32_t ops_idx;
} __attribute__((packed)) vsp_init_st;

typedef struct {
    int32_t ops_idx;
    tAmlVspRpcHdl hdl;
} __attribute__((packed)) vsp_deinit_st;

typedef struct {
    int32_t ops_idx;
    tAmlVspRpcHdl hdl;
    uint32_t ret;
} __attribute__((packed)) vsp_open_st;

typedef struct {
    int32_t ops_idx;
    tAmlVspRpcHdl hdl;
    uint32_t ret;
} __attribute__((packed)) vsp_close_st;

typedef struct {
    int32_t ops_idx;
    tAmlVspRpcHdl hdl;
    int32_t param_id;
    xpointer param;
    xsize_t param_size;
    uint32_t ret;
} __attribute__((packed)) vsp_setparam_st;

typedef struct {
    int32_t ops_idx;
    tAmlVspRpcHdl hdl;
    int32_t param_id;
    xpointer param;
    xsize_t param_size;
    uint32_t ret;
} __attribute__((packed)) vsp_getparam_st;

typedef struct {
    int32_t ops_idx;
    tAmlVspRpcHdl hdl;
    uint32_t ret;
    xpointer input_buf;
    xsize_t input_size;
    xpointer output_buf;
    xsize_t output_size;
} __attribute__((packed)) vsp_process_st;


/*hifi codec shared memory*/
typedef xpointer AML_MEM_HANDLERpc;
typedef struct {
    AML_MEM_HANDLERpc hShm;
    xsize_t size;
    int32_t pid;
} __attribute__((packed)) acodec_shm_alloc_st;

typedef struct {
    AML_MEM_HANDLERpc hShm;
} __attribute__((packed)) acodec_shm_free_st;

typedef struct {
    AML_MEM_HANDLERpc hDst;
    AML_MEM_HANDLERpc hSrc;
    xsize_t size;
} __attribute__((packed)) acodec_shm_transfer_st;

typedef struct {
    int32_t pid;
} __attribute__((packed)) acodec_shm_recycle_st;

/*hifi flat buffer*/
typedef xpointer tAmlFlatBufHdlRpc;
#define FLATBUFFERS_MAX 32

typedef struct {
    tAmlFlatBufHdlRpc hFbuf;
    uint32_t clear;
} __attribute__((packed)) aml_flatbuf_reset_st;

typedef struct {
    tAmlFlatBufHdlRpc hFbuf;
    AML_MEM_HANDLERpc hInterBuf;
    char buf_id[FLATBUFFERS_MAX];
    int32_t flags;
    xsize_t size;
} __attribute__((packed)) aml_flatbuf_create_st;

typedef struct {
    tAmlFlatBufHdlRpc hFbuf;
} __attribute__((packed)) aml_flatbuf_destroy_st;

typedef struct {
    tAmlFlatBufHdlRpc hFbuf;
    xpointer mem;
    xsize_t size;
    uint32_t ms;
} __attribute__((packed)) aml_flatbuf_write_st;

typedef struct {
    tAmlFlatBufHdlRpc hFbuf;
    xpointer mem;
    xsize_t size;
    uint32_t ms;
} __attribute__((packed)) aml_flatbuf_read_st;

typedef struct {
    tAmlFlatBufHdlRpc hFbuf;
    xsize_t size;
} __attribute__((packed)) aml_flatbuf_size_st;

/*hifi4 register dump*/
typedef struct {
    uint32_t reg_addr;
    uint32_t size;
} __attribute__((packed)) aml_hifi4reg_dump_st;

/*tinyalsa*/
typedef xpointer tAcodecPcmSrvHdl;
typedef struct {
	/** The number of channels in a frame */
	unsigned int channels;
	/** The number of frames per second */
	unsigned int rate;
	/** The number of frames in a period */
	unsigned int period_size;
	/** The number of periods in a PCM */
	unsigned int period_count;
	/** The sample format of a PCM */
	int format;
	/* Values to use for the ALSA start, stop and silence thresholds.  Setting
	 * any one of these values to 0 will cause the default tinyalsa values to be
	 * used instead.  Tinyalsa defaults are as follows.
	 *
	 * start_threshold   : period_count * period_size
	 * stop_threshold    : period_count * period_size
	 * silence_threshold : 0
	 */
	/** The minimum number of frames required to start the PCM */
	unsigned int start_threshold;
	/** The minimum number of frames required to stop the PCM */
	unsigned int stop_threshold;
	/** The minimum number of frames to silence the PCM */
	unsigned int silence_threshold;
}__attribute__((packed)) rpc_pcm_config;


typedef struct {
    uint32_t card;
    uint32_t device;
    uint32_t flags;
    rpc_pcm_config pcm_config; // dsp also could access this address
    xpointer out_pcm;
} __attribute__((packed)) pcm_open_st;

typedef struct {
    xpointer pcm;
    int32_t out_ret;
} __attribute__((packed)) pcm_close_st;

typedef struct {
    xpointer pcm;
    xpointer data; // dsp need access this address
    uint32_t count;
    int32_t out_ret;
} __attribute__((packed)) pcm_io_st;

typedef struct {
    xpointer pcm;
    int32_t out_ret; // long to int32_t
} __attribute__((packed)) pcm_get_latency_st;

typedef struct {
    xpointer out_ptr; // returned pointer
    xsize_t size;     // memory's size
    xpointer ptr;
} __attribute__((packed)) shm_st;

typedef struct {
    uint32_t card;
    uint32_t device;
    uint32_t flags;
    rpc_pcm_config pcm_config; // dsp also could access this address
    xpointer out_pcm;
} __attribute__((packed)) pcm_process_open_st;

typedef struct {
    xpointer pcm;
    int32_t out_ret;
} __attribute__((packed)) pcm_process_close_st;

typedef struct {
    xpointer pcm;
    xpointer data; // dsp need access this address
    uint32_t count;
    uint32_t id;
    int32_t out_ret;
} __attribute__((packed)) pcm_process_io_st;

typedef struct {
    xpointer pcm;
    xpointer release_buf_handle;
    xpointer buf_handle;
    xpointer data;
    uint32_t count;
    uint32_t id;
    int32_t out_ret;
} __attribute__((packed)) pcm_process_buf_st;

typedef struct {
    xpointer pcm;
    int32_t gain;
    int32_t out_ret;
} __attribute__((packed)) pcm_process_gain_st;

typedef struct {
    xpointer pcm;
    uint32_t chanid;
    uint32_t size;
    uint32_t phy_ch;
    char buf_name[BUF_STR_ID_MAX];
    int32_t out_ret;
} __attribute__((packed)) pcm_process_memory_st;

#define PROCESSBUF 0
#define RAWBUF 1

// TODO: mailbox layer should expose this definition
// T7 mailbox FIFO only support 96bytes
#define MB_DATA_SHR_SIZE 96
#define STRING_RAW_MAX_LEN (MB_DATA_SHR_SIZE - sizeof(xsize_t))
typedef struct {
    xsize_t len;
    char raw[STRING_RAW_MAX_LEN];
} __attribute__((packed)) raw_st;

/**
 * @brief convert list of arguments to raw string
 * The arguments should NOT include special char, such as whitespace, quote, & etc.
 *
 * @param [in] argc number of arguments
 * @param [in] argv array of arguments
 * @param [out] p converted raw string
 *
 * @return 0, success
 */
int from_arg(int argc, char **argv, raw_st *p);
/**
 * @brief convert raw string back to list of arguments
 * The caller need free the list of arguments via free_arg function
 *
 * @param [in] p raw string
 * @param [out] o_argc number of arguments
 * @param [out] o_argv array of arguments
 *
 * @return 0, success
 */
int to_arg(raw_st *p, int *o_argc, char ***o_argv);
/**
 * @brief append string to list of arguments
 *
 * @param [in out] o_argc number of arguments
 * @param [in out] o_argv array of arguments
 * @param [in] s the string, which to add to list
 *
 * @return 0, success
 */
int append_arg(int *o_argc, char ***o_argv, char *s);
/** util function, help to free arguments which is converted from
 * to_arg/append_arg */
void free_arg(int argc, char **argv);
void show_arg(int argc, char **argv);

typedef struct {
    uint32_t times;
    int32_t is_dsp_clk;
    int32_t is_arm_on;
} __attribute__((packed)) vad_awe_wakeup_st;

#ifdef __cplusplus
}
#endif

#endif // end _AIPC_TYPE_H_
