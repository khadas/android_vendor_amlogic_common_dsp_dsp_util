/*
 * Copyright (C) 2014 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <time.h>
#include "mp3reader.h"
#include "sndfile.h"
#include "rpc_client_aac.h"
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "rpc_client_pcm.h"
#include "rpc_client_vsp.h"
#include "aipc_type.h"


#define MSECS_PER_SEC (1000L)
#define NSECS_PER_MSEC (1000000L)

void aprofiler_get_cur_timestamp(struct timespec* ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
    return;
}

uint32_t aprofiler_msec_duration(struct timespec* tsEnd, struct timespec* tsStart)
{
    uint32_t uEndMSec = (uint32_t)(tsEnd->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsEnd->tv_nsec/NSECS_PER_MSEC);
    uint32_t uStartMSec = (uint32_t)(tsStart->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsStart->tv_nsec/NSECS_PER_MSEC);
	//printf("uEndMSec:%d, uStartMSec:%d\n", uEndMSec, uStartMSec);
	return (uEndMSec - uStartMSec);
}


#define TIC              \
    struct timespec bgn, end; \
    aprofiler_get_cur_timestamp(&bgn)

#define TOC                            \
    aprofiler_get_cur_timestamp(&end); \
    uint32_t ms = aprofiler_msec_duration(&end, &bgn)


uint32_t audio_play_data[] = {
//#include "sinewav_48k_24bits_stereo_l1k_r2k_1s.in"
//#include "sinewav_48k_24bits_stereo_l1k_r2k_1s.in"
#include "pyghlkn_48k_24bits_stereo_10s.in"
};
uint32_t audio_play_data_len = sizeof(audio_play_data);

using namespace std;

static void mp3_show_hex(char* samples, uint32 size)
{
	int i;
	for (i = 0; i < size; i++) {
		printf("0x%x ", samples[i]);
	}
	printf("\n");
}

static int pcm_play_buildin()
{
	rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = 2;
	pconfig->rate = 48000;
	pconfig->format = PCM_FORMAT_S32_LE;
	pconfig->period_size = 1024;
	pconfig->period_count = 2;
	pconfig->start_threshold = 1024;
	pconfig->silence_threshold = 1024*2;
	pconfig->stop_threshold = 1024*2;
	tAmlPcmhdl p = pcm_client_open(0, 0, 0, pconfig);
	tAcodecShmHdl hShmBuf;

	uint8_t *play_data = (uint8_t *)audio_play_data;
	int in_fr = pcm_client_bytes_to_frame(p, audio_play_data_len);
	int i, fr = 0;
	const int ms = 36;
	const int oneshot = 48 * ms; // 1728 samples
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = Aml_ACodecMemory_Allocate(size);
	void *buf = Aml_ACodecMemory_GetVirtAddr(hShmBuf);
	void *phybuf = Aml_ACodecMemory_GetPhyAddr(hShmBuf);
	for (i = 0; i + oneshot <= in_fr; i += fr) {
		memcpy(buf, play_data + pcm_client_frame_to_bytes(p, i), size);
		Aml_ACodecMemory_Clean(phybuf, size);
		fr = pcm_client_writei(p, phybuf, oneshot);
		//printf("%dms pcm_write i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
			//  ms, i, p, buf, oneshot, fr);
	}
	pcm_client_close(p);
	Aml_ACodecMemory_Free(hShmBuf);
	free(pconfig);
}

static int pcm_play_test(int argc, char* argv[])
{
	rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = 2;
	pconfig->rate = 48000;
	pconfig->format = PCM_FORMAT_S32_LE;
	pconfig->period_size = 1024;
	pconfig->period_count = 2;
	pconfig->start_threshold = 1024;
	pconfig->silence_threshold = 1024*2;
	pconfig->stop_threshold = 1024*2;
	tAmlPcmhdl p = pcm_client_open(0, 0, 0, pconfig);
	tAcodecShmHdl hShmBuf;

	FILE *fileplay = fopen(argv[0], "rb");
	if (fileplay == NULL) {
		printf("failed to open played pcm file\n");
		return -1;
	}
	int fr = 0;
	const int ms = 36;
	const int oneshot = 48 * ms; // 1728 samples
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = Aml_ACodecMemory_Allocate(size);
	void *buf = Aml_ACodecMemory_GetVirtAddr(hShmBuf);
	void *phybuf = Aml_ACodecMemory_GetPhyAddr(hShmBuf);
	while (size = fread(buf, 1, size, fileplay)) {
		Aml_ACodecMemory_Clean(phybuf, size);
		fr = pcm_client_writei(p, phybuf, oneshot);
		//printf("%dms pcm_write pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
			//   ms, p, buf, oneshot, fr);
	}
	pcm_client_close(p);
	Aml_ACodecMemory_Free(hShmBuf);
	free(pconfig);
	fclose(fileplay);
}


#define PCM_CAPTURE_SAMPLES (48000*2)
static int pcm_capture_test(int argc, char* argv[])
{
	enum ALSA_DEVICE_IN device = DEVICE_TDMIN_B;
	if (argc >= 2)
		device = (enum ALSA_DEVICE_IN)atoi(argv[1]);
	rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = (device == DEVICE_LOOPBACK)?4:2;
	pconfig->rate = 48000;
	pconfig->format = PCM_FORMAT_S32_LE;
	pconfig->period_size = 1024;
	pconfig->period_count = 4;
	pconfig->start_threshold = 1024;
	pconfig->silence_threshold = 1024*2;
	pconfig->stop_threshold = 1024*2;
	tAmlPcmhdl p = pcm_client_open(0, device, PCM_IN, pconfig);
	tAcodecShmHdl hShmBuf;

	FILE *filecap = fopen(argv[0], "w+b");
	if (filecap == NULL) {
		printf("failed to open captured pcm file\n");
		return -1;
	}

	int in_fr = PCM_CAPTURE_SAMPLES;
	int i, fr = 0;
	const int ms = 36;
	const int oneshot = 48 * ms; // 1728 samples
	uint32_t size = pcm_client_frame_to_bytes(p, oneshot);
	hShmBuf = Aml_ACodecMemory_Allocate(size);
	void *buf = Aml_ACodecMemory_GetVirtAddr(hShmBuf);
	void *phybuf = Aml_ACodecMemory_GetPhyAddr(hShmBuf);
	for (i = 0; i + oneshot <= in_fr; i += fr) {
		fr = pcm_client_readi(p, phybuf, oneshot);
		Aml_ACodecMemory_Inv(phybuf, size);
		fwrite(buf, sizeof(char), size, filecap);
		//printf("%dms pcm_read i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
			//  ms, i, p, buf, oneshot, fr);
	}
	pcm_client_close(p);
	Aml_ACodecMemory_Free(hShmBuf);
	free(pconfig);
	fclose(filecap);
}

static int mp3_offload_dec(int argc, char* argv[]) {
	int bUserAllocShm = 1;
	tAmlMp3DecHdl hdlmp3 = 0;
	tAcodecShmHdl hShmInput =0;
	tAcodecShmHdl hShmOutput = 0;
	uint8_t *inputBuf = 0;
	int16_t *outputBuf = 0;
	void* inputphy = 0;
	void* outputphy = 0;

    // Initialize the config.
    tAmlACodecConfig_Mp3DecExternal config;
	memset(&config, 0, sizeof(tAmlACodecConfig_Mp3DecExternal));
    config.equalizerType = flat;
    config.crcEnabled = false;

    // Initialize the decoder.
    hdlmp3 = AmlACodecInit_Mp3Dec(&config);
    printf("Init mp3dec hdl=%p\n", hdlmp3);

    // Open the input file.
    Mp3Reader mp3Reader;
	printf("Read mp3 file:%s\n", argv[0]);
    bool success = mp3Reader.init(argv[0]);
    if (!success) {
        fprintf(stderr, "Encountered error reading %s\n", argv[0]);
		AmlACodecDeInit_Mp3Dec(hdlmp3);
        return EXIT_FAILURE;
    }
    printf("Init mp3reader\n");

    // Open the output file.
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(SF_INFO));
    sfInfo.channels = mp3Reader.getNumChannels();
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfInfo.samplerate = mp3Reader.getSampleRate();
    SNDFILE *handle = sf_open(argv[1], SFM_WRITE, &sfInfo);
    if (handle == NULL) {
        fprintf(stderr, "Encountered error writing %s\n", argv[1]);
        mp3Reader.close();
		AmlACodecDeInit_Mp3Dec(hdlmp3);
        return EXIT_FAILURE;
    }
    printf("Init outfile=%s\n", argv[1]);

    if (argc == 3)
		bUserAllocShm = atoi(argv[2]);
    if (bUserAllocShm) {
		// Allocate input buffer.
		hShmInput = Aml_ACodecMemory_Allocate(kInputBufferSize);
		inputBuf = (uint8_t*)Aml_ACodecMemory_GetVirtAddr(hShmInput);
		inputphy = Aml_ACodecMemory_GetPhyAddr(hShmInput);
	
		// Allocate output buffer.
		hShmOutput = Aml_ACodecMemory_Allocate(kOutputBufferSize);
		outputBuf = (int16_t*)Aml_ACodecMemory_GetVirtAddr(hShmOutput);
		outputphy = Aml_ACodecMemory_GetPhyAddr(hShmOutput);
		printf("Init in vir:%p phy:%p,  out vir:%p phy:%p\n", 
				inputBuf, inputphy, outputBuf, outputphy);
	} else {
		// Allocate input buffer.
		inputBuf = (uint8_t*)malloc(kInputBufferSize);

		// Allocate output buffer.
		outputBuf = (int16_t*)malloc(kOutputBufferSize);
		printf("Init input %p, output buffer %p\n", inputBuf, outputBuf);
	}

    // Decode loop.
    int retVal = EXIT_SUCCESS;
    while (1) {
        // Read input from the file.
        uint32_t bytesRead;
        success = mp3Reader.getFrame(inputBuf, &bytesRead);
        if (!success) {
			printf("EOF\n");
            break;
        }

        // Set the input config.
        config.inputBufferCurrentLength = bytesRead;
        config.inputBufferMaxLength = 0;
        config.inputBufferUsedLength = 0;
		if (bUserAllocShm) {
			config.pInputBuffer = (uint8_t*)inputphy;
			config.pOutputBuffer = (int16_t*)outputphy;			
		} else {
			config.pInputBuffer = (uint8_t*)inputBuf;
			config.pOutputBuffer = (int16_t*)outputBuf;
		}
        config.outputFrameSize = kOutputBufferSize / sizeof(int16_t);
		/*INFO("inputBufferCurrentLength:0x%x, inputBufferMaxLength:0x%x, inputBufferUsedLength:0x%x,"
			"pInputBuffer:0x%lx, pOutputBuffer:0x%lx, outputFrameSize:0x%x\n",
			config.inputBufferCurrentLength, config.inputBufferMaxLength, config.inputBufferUsedLength,
			config.pInputBuffer, config.pOutputBuffer, config.outputFrameSize);*/
		/*INFO("\n=================================\n");
		mp3_show_hex((char*)config.pInputBuffer, bytesRead);
		INFO("\n=================================\n");*/
        ERROR_CODE decoderErr;
        //printf("config.outputFrameSize:%d\n", config.outputFrameSize);
        if (bUserAllocShm) {
			Aml_ACodecMemory_Clean(inputphy, bytesRead);
			decoderErr = AmlACodecExec_UserAllocIoShm_Mp3Dec(hdlmp3, &config);
			Aml_ACodecMemory_Inv(outputphy, config.outputFrameSize*sizeof(int16_t));
		} else
			decoderErr = AmlACodecExec_Mp3Dec(hdlmp3, &config);

        if (decoderErr != NO_DECODING_ERROR) {
            fprintf(stderr, "Decoder encountered error:0x%x\n", decoderErr);
            retVal = EXIT_FAILURE;
            break;
        }
	    /*INFO("\n*************arm:%p**********************\n", outputBuf);
		mp3_show_hex((char*)outputBuf, config.outputFrameSize*sizeof(int16_t));
		INFO("\n***********************************\n");*/
        //INFO("config.outputFrameSize:%d\n", config.outputFrameSize);
        sf_writef_short(handle, outputBuf,
                        config.outputFrameSize / sfInfo.channels);
    }
    printf("mp3 decoder done\n");

    // Close input reader and output writer.
    mp3Reader.close();
    printf("reader close\n");
    sf_close(handle);
    printf("write clonse\n");

    // Free allocated memory.
	if (bUserAllocShm) {
		Aml_ACodecMemory_Free((tAcodecShmHdl)hShmInput);
		Aml_ACodecMemory_Free((tAcodecShmHdl)hShmOutput);
	} else {
		free(inputBuf);
		free(outputBuf);
	}
    AmlACodecDeInit_Mp3Dec(hdlmp3);

    return retVal;
}

static int aac_offload_dec(int argc, char* argv[]) {
	SNDFILE *handle = NULL;
	FILE* pcmfile = NULL;
	int bUserAllocShm = 1;
	tAmlAacDecHdl hdlAac = 0;
	tAcodecShmHdl hShmInput =0;
	tAcodecShmHdl hShmOutput = 0;
	uint8_t *inputBuf = 0;
	int16_t *outputBuf = 0;
	void* inputphy = 0;
	void* outputphy = 0;

    // Initialize the decoder.
	tAmlAacInitCtx config;
	config.transportFmt = TT_MP4_ADTS;
	config.nrOfLayers = 1;
    hdlAac = AmlACodecInit_AacDec(&config);
    printf("Init aacdec hdl=%p\n", hdlAac);

    // Open the input file.
	FILE* aacfile = fopen(argv[0], "rb");
    if (!aacfile) {
        fprintf(stderr, "Encountered error reading %s\n", argv[0]);
		AmlACodecDeInit_AacDec(hdlAac);
        return EXIT_FAILURE;
    }
    printf("Open aac file\n");

	//AmlACodecSetParam(hdlAac, AAC_DRC_REFERENCE_LEVEL, 54);
	//AmlACodecSetParam(hdlAac, AAC_DRC_ATTENUATION_FACTOR, 32);
	//AmlACodecSetParam(hdlAac, AAC_PCM_LIMITER_ENABLE, 1);


    if (argc == 3)
		bUserAllocShm = atoi(argv[2]);
    if (bUserAllocShm) {
		// Allocate input buffer.
		hShmInput = Aml_ACodecMemory_Allocate(AAC_INPUT_SIZE);
		printf("hShmInput:%p\n", hShmInput);
		inputBuf = (uint8_t*)Aml_ACodecMemory_GetVirtAddr(hShmInput);
		inputphy = Aml_ACodecMemory_GetPhyAddr(hShmInput);

		// Allocate output buffer.
		hShmOutput = Aml_ACodecMemory_Allocate(PCM_OUTPUT_SIZE);
		outputBuf = (int16_t*)Aml_ACodecMemory_GetVirtAddr(hShmOutput);
		printf("hShmOutput:%p\n", hShmOutput);
		outputphy = Aml_ACodecMemory_GetPhyAddr(hShmOutput);
		printf("===Init in vir:%p phy:%p,  out vir:%p phy:%p====\n",
				inputBuf, inputphy, outputBuf, outputphy);
		if (!hShmOutput || !hShmInput){
			fprintf(stderr, "Encountered memory allocation issue\n");
			fclose(aacfile);
			AmlACodecDeInit_AacDec(hdlAac);
			return EXIT_FAILURE;
		}
	} else {
		// Allocate input buffer.
		inputBuf = (uint8_t*)malloc(AAC_INPUT_SIZE);

		// Allocate output buffer.
		outputBuf = (int16_t*)malloc(PCM_OUTPUT_SIZE);
		printf("Init input %p, output buffer %p\n", inputBuf, outputBuf);
		if (!inputBuf || !outputBuf) {
			fprintf(stderr, "Encountered memory allocation issue\n");
			fclose(aacfile);
			AmlACodecDeInit_AacDec(hdlAac);
			return EXIT_FAILURE;
		}
	}

    // Decode loop.
    int retVal = EXIT_SUCCESS;
    while (1) {
        // Read input from the file.
        uint32_t bytesRead = AAC_INPUT_SIZE;
		uint32_t aac_input_size, pcm_out_size, aac_input_left;
		tAmlAacOutputCtx out_ctx;
        bytesRead = fread(inputBuf, 1, bytesRead, aacfile);
        if (!bytesRead) {
			printf("EOF\n");
            break;
        }

        aac_input_size = bytesRead;
		pcm_out_size = PCM_OUTPUT_SIZE;
        AAC_DECODER_ERROR decoderErr;
        if (bUserAllocShm) {
			Aml_ACodecMemory_Clean(inputphy, bytesRead);
			decoderErr = AmlACodecExec_UserAllocIoShm_AacDec(hdlAac, inputphy, aac_input_size,
											   outputphy, &pcm_out_size,
												&aac_input_left, &out_ctx);
			//printf("aac_input_left:%dn", aac_input_left);
			Aml_ACodecMemory_Inv(outputphy, pcm_out_size);
			fseek(aacfile, -aac_input_left, SEEK_CUR);
		} else {
			decoderErr = AmlACodecExec_AacDec(hdlAac, inputBuf, aac_input_size,
											   outputBuf, &pcm_out_size,
												&aac_input_left, &out_ctx);
			fseek(aacfile, -aac_input_left, SEEK_CUR);
		}
        if (decoderErr != AAC_DEC_OK) {
            fprintf(stderr, "Decoder encountered error:0x%x\n", decoderErr);
            retVal = EXIT_FAILURE;
            break;
        }

		if (pcmfile == NULL) {
			// Open the output file.
			printf("=======Decode first aac frame:======\n");
			printf("channels: %d, sampleRate:%d, frameSize:%d\n"
					"channal mask - front:%d side:%d back:%d lfe:%d top:%d\n",
				   out_ctx.channelNum, out_ctx.sampleRate, out_ctx.frameSize,
				   out_ctx.chmask[ACT_FRONT], out_ctx.chmask[ACT_SIDE],
				   out_ctx.chmask[ACT_BACK],  out_ctx.chmask[ACT_LFE],
				   out_ctx.chmask[ACT_FRONT_TOP] + out_ctx.chmask[ACT_SIDE_TOP] +
				   out_ctx.chmask[ACT_BACK_TOP]  + out_ctx.chmask[ACT_TOP]);
			printf("Init outfile=%s\n", argv[1]);
			SF_INFO sfInfo;
			memset(&sfInfo, 0, sizeof(SF_INFO));
			sfInfo.channels = out_ctx.channelNum;
			sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;//FDK limited to 16bit per samples
			sfInfo.samplerate = out_ctx.sampleRate;
			//handle = sf_open(argv[1], SFM_WRITE, &sfInfo);
			pcmfile = fopen(argv[1], "w+b");
			if (pcmfile == NULL) {
				fprintf(stderr, "Encountered error writing %s\n", argv[1]);
				goto tab_end;
			}
		}

	    /*INFO("\n*************arm:%p**********************\n", outputBuf);
		mp3_show_hex((char*)outputBuf, config.outputFrameSize*sizeof(int16_t));
		INFO("\n***********************************\n");*/
        //INFO("config.outputFrameSize:%d\n", config.outputFrameSize);
		//if (handle)
	      //  sf_writef_short(handle, outputBuf,pcm_out_size/SF_FORMAT_PCM_16);
	    if (pcmfile)
			fwrite(outputBuf, 1, out_ctx.frameSize*out_ctx.channelNum*SF_FORMAT_PCM_16, pcmfile);
    }

    // Close input reader and output writer.
    printf("aac decoder done\n");

tab_end:
	if (aacfile) {
		fclose(aacfile);
		printf("aac file close\n");
	}
	if (handle) {
		sf_close(handle);
		printf("write close\n");
	}
	fclose(pcmfile);
    // Free allocated memory.
	if (bUserAllocShm) {
		if (hShmInput)
			Aml_ACodecMemory_Free((tAcodecShmHdl)hShmInput);
		if (hShmOutput)
			Aml_ACodecMemory_Free((tAcodecShmHdl)hShmOutput);
	} else {
		if (inputBuf)
			free(inputBuf);
		if (outputBuf)
			free(outputBuf);
	}
	if (hdlAac)
		AmlACodecDeInit_AacDec(hdlAac);
    return retVal;
}

typedef struct {
	int32_t	  Fs_Hz_in;
	int32_t	  Fs_Hz_out;
} __attribute__((packed)) aml_vsp_st_param;

/*Just an example show how to apply meta data*/
typedef struct {
	int32_t	  Fs;
	int32_t   Bitdepth;
} __attribute__((packed)) aml_vsp_meta_param;

#define SAMPLE_PER_MS 48
#define VOICE_MS 32
#define VOICE_LEN (4*SAMPLE_PER_MS*VOICE_MS)

static int offload_vsp_rsp(int argc, char* argv[]) {
	int ret = 0;
	int Vsp_Err = 0;
	AML_VSP_HANDLE hdlvsp = 0;
	AML_MEM_HANDLE hParam = 0;
	AML_MEM_HANDLE hShmInput =0;
	AML_MEM_HANDLE hShmOutput = 0;
	uint8_t *paramBuf = 0;
	uint8_t *inputBuf = 0;
	uint8_t *outputBuf = 0;
	void* paramphy = 0;
	void* inputphy = 0;
	void* outputphy = 0;
	size_t bytesRead = VOICE_LEN;
	size_t bytesWrite = 0;
	aml_vsp_st_param* st_param = NULL;
	aml_vsp_meta_param* meta_param = NULL;
	FILE* voicefile = NULL;
	FILE* outfile = NULL;

    // Open the input voice file.
	voicefile = fopen(argv[0], "rb");
    if (!voicefile) {
        printf("Open input file failure %s\n", argv[0]);
        ret = -1;
		goto tab_end;
    }
    printf("Open voice file\n");

	outfile = fopen(argv[1], "w+b");
    if (!outfile) {
        printf("Open output file failure %s\n", argv[1]);
        ret = -1;
		goto tab_end;
    }
    printf("Open output file\n");



	// Allocate input buffer.
	hShmInput = AML_MEM_Allocate(sizeof(aml_vsp_meta_param) + VOICE_LEN);
	inputBuf = (uint8_t*)AML_MEM_Getvirtaddr(hShmInput);
	inputphy = AML_MEM_Getphyaddr(hShmInput);

	// Allocate output buffer.
	hShmOutput = AML_MEM_Allocate(VOICE_LEN/3);
	outputBuf = (uint8_t*)AML_MEM_Getvirtaddr(hShmOutput);
	outputphy = AML_MEM_Getphyaddr(hShmOutput);

	// Allocate static param buffer, the param is used to initialize vsp
	hParam = AML_MEM_Allocate(sizeof(aml_vsp_st_param));
	paramBuf = (uint8_t*)AML_MEM_Getvirtaddr(hParam);
	paramphy = AML_MEM_Getphyaddr(hParam);

    // Initialize the vsp-rsp.
	st_param = (aml_vsp_st_param*)paramBuf;
	st_param->Fs_Hz_in = 48000;
	st_param->Fs_Hz_out = 16000;
	AML_MEM_Clean(paramphy, sizeof(aml_vsp_st_param));
	hdlvsp = AML_VSP_Init(AML_VSP_RESAMPLER, (void*)paramphy, sizeof(aml_vsp_st_param));
	if (!hdlvsp) {
        printf("Initialize vsp failure\n");
        ret = -1;
		goto tab_end;
	}
    printf("Init vsp-rsp hdl=%p\n", hdlvsp);

    while (1) {
		//example to show apply meta data associate with the buffer.
		meta_param = (aml_vsp_meta_param*)inputBuf;
		meta_param->Bitdepth = 16;
		meta_param->Fs = 48000;
        bytesRead = fread(inputBuf + sizeof(aml_vsp_meta_param), 1, bytesRead, voicefile);
        if (!bytesRead) {
			printf("EOF\n");
            break;
        }

		bytesWrite = bytesRead/3;
		AML_MEM_Clean(inputphy, bytesRead + sizeof(aml_vsp_meta_param));
		Vsp_Err = AML_VSP_Process(hdlvsp, inputphy, bytesRead + sizeof(aml_vsp_meta_param), outputphy, &bytesWrite);
		AML_MEM_Invalidate(outputphy, bytesWrite);

        if (Vsp_Err != 0) {
            printf("Decoder encountered error:0x%x\n", Vsp_Err);
            ret = -1;
            break;
        }
	    if (outfile)
			fwrite(outputBuf, 1, bytesWrite, outfile);
    }
    printf("voice signal resampling done\n");

tab_end:
    // Close input reader and output writer.
	if (voicefile)
		fclose(voicefile);
	if (outfile)
		fclose(outfile);

    // Free allocated memory.
    if (hShmInput)
		AML_MEM_Free((AML_MEM_HANDLE)hShmInput);
	if (hShmOutput)
		AML_MEM_Free((AML_MEM_HANDLE)hShmOutput);
	if (hParam)
		AML_MEM_Free((AML_MEM_HANDLE)hParam);
	if (hdlvsp)
		AML_VSP_Deinit(hdlvsp);

    return ret;
}



#define IPC_UNIT_TEST_REPEAT 50
static int ipc_uint_tset(void) {
	unsigned int i;
	int num_repeat = IPC_UNIT_TEST_REPEAT;
	const char send_samples[16] = {
		0x1, 0xf, 0x2, 0xe, 0x3, 0xd, 0x4, 0xc, 0x5, 0xb, 0x6, 0xa, 0x7, 0x9, 0x0, 0x8
	};
	const char recv_samples[16] = {
		0x11, 0xf1, 0x21, 0x1e, 0x13, 0x1d, 0x14, 0xc2, 0x52, 0xb2, 0x62, 0x2a, 0x27, 0x29, 0x20, 0x38
	};
	char ipc_data[16];
	int arpchdl = xAudio_Ipc_init();
	while(num_repeat--) {
		memcpy(ipc_data, send_samples, sizeof(ipc_data));
		xAIPC(arpchdl, MBX_CMD_IPC_TEST, (void*)ipc_data, sizeof(ipc_data));
		for (i = 0; i < sizeof(recv_samples); i++) {
			if(recv_samples[i] != ipc_data[i])
				break;
		}
		if (i < sizeof(recv_samples)) {
			printf("arm ack: ipc unittest fail:%d, ipc data:\n", IPC_UNIT_TEST_REPEAT - num_repeat);
			for(i = 0; i < sizeof(recv_samples); i++)
				printf("0x%x ", ipc_data[i]);
			printf("\n");
			break;
		}
	}
	if(num_repeat <= 0)
		printf("ipc unittest pass, repeat: %d\n", IPC_UNIT_TEST_REPEAT);
	xAudio_Ipc_Deinit(arpchdl);
	return 0;
}

#define SHM_UNIT_TEST_REPEAT 50
static int shm_uint_tset(void)
{
	unsigned int i;
	int num_repeat = SHM_UNIT_TEST_REPEAT;
	char samples[16] = {0};
	void* pVirSrc = NULL;
	void* pVirDst = NULL;
	while(num_repeat--) {
		tAcodecShmHdl hDst, hSrc;
		hDst = Aml_ACodecMemory_Allocate(sizeof(samples));
		hSrc = Aml_ACodecMemory_Allocate(sizeof(samples));
		memset((void*)samples, num_repeat, sizeof(samples));
		pVirSrc = Aml_ACodecMemory_GetVirtAddr(hSrc);
		memcpy((void*)pVirSrc, samples, sizeof(samples));
		Aml_ACodecMemory_Clean(hSrc, sizeof(samples));
		Aml_ACodecMemory_Transfer(hDst, hSrc, sizeof(samples));
		Aml_ACodecMemory_Inv(hDst, sizeof(samples));
		pVirDst = Aml_ACodecMemory_GetVirtAddr(hSrc);
		if (memcmp((void*)pVirDst, samples, sizeof(samples))) {
			printf("shm unit test fail, repeat:%d\n",
				SHM_UNIT_TEST_REPEAT - num_repeat);
			break;
		} else {
			/*char* k = (char*)pVirDst;
			for(i = 0; i < sizeof(samples); i++)
				printf("0x%x ", k[i]);
			printf("\n");*/
		}
		Aml_ACodecMemory_Free(hSrc);
		Aml_ACodecMemory_Free(hDst);
	}
	if(num_repeat <= 0)
		printf("ipc unittest pass, repeat %d\n", SHM_UNIT_TEST_REPEAT);
	return 0;

}


static void usage()
{
	printf ("ipc unit test usage: hifi4rpc_client_test --ipc\n");
	printf ("\n");
	printf ("shared memory unit test usage: hifi4rpc_client_test --shm\n");
	printf ("\n");
	printf ("mp3dec Usage: hifi4rpc_client_test --mp3dec input_file output_file bUserAllocShm\n");
	printf ("\n");
	printf ("aacdec Usage: hifi4rpc_client_test --aacdec input_file output_file bUserAllocShm\n");
	printf ("\n");
	printf ("pcmplay Usage: hifi4rpc_client_test --pcmplay pcm_file\n");
	printf ("\n");
	printf ("pcmcap Usage: hifi4rpc_client_test --pcmcap  pcm_file [1:tdmin,3:tdmin&loopback, 4:pdmin]\n");
	printf ("\n");
	printf ("pcmplay-buildin Usage: hifi4rpc_client_test --pcmplay-buildin\n");
	printf ("\n");
	printf ("vsp-rsp Usage: hifi4rpc_client_test --vsp-rsp input_file output_file\n");
	printf ("\n");
}


int main(int argc, char* argv[]) {
	int c = -1;
	int option_index = 0;
	struct option long_options[] =
	{
	   {"help", no_argument, NULL, 0},
	   {"ipc", no_argument, NULL, 1},
	   {"shm", no_argument, NULL, 2},
	   {"mp3dec", no_argument, NULL, 3},
	   {"pcmplay", no_argument, NULL, 4},
	   {"pcmcap", no_argument, NULL, 5},
	   {"pcmplay-buildin", no_argument, NULL, 6},
	   {"aacdec", no_argument, NULL, 7},
	   {"vsp-rsp", no_argument, NULL, 8},
	   {0, 0, 0, 0}
	};
	c = getopt_long (argc, argv, "hvV", long_options, &option_index);
	if(-1 == c) {
		printf("error options\n");
	}
	switch(c)
	{
		case 0:
			usage();
			break;
		case 1:
			{
				TIC;
				ipc_uint_tset();
				TOC;
				printf("ipc unit test use:%u ms\n", ms);
			}
			break;
		case 2:
			{
				TIC;
				shm_uint_tset();
				TOC;
				printf("shm unit test use %u ms\n", ms);
			}
			break;
		case 3:
			if (2 == argc - optind || 3 == argc - optind) {
				TIC;
				mp3_offload_dec(argc - optind, &argv[optind]);
				TOC;
				printf("mp3 offload decoder use:%u ms\n", ms);
			}
			else {
				usage();
				exit(1);
			}
			break;
		case 4:
			if (1 == argc - optind)
				pcm_play_test(argc - optind, &argv[optind]);
			else {
				usage();
				exit(1);
			}
			break;
		case 5:
			if (1 == argc - optind || 2 ==  argc - optind)
				pcm_capture_test(argc - optind, &argv[optind]);
			else {
				usage();
				exit(1);
			}
			break;
		case 6:
			pcm_play_buildin();
		case 7:
			if (2 == argc - optind || 3 == argc - optind) {
				TIC;
				aac_offload_dec(argc - optind, &argv[optind]);
				TOC;
				printf("aac offload decoder use:%u ms\n", ms);
			}
			else {
				usage();
				exit(1);
			}
			break;
		case 8:
			if (2 == argc - optind) {
				TIC;
				offload_vsp_rsp(argc - optind, &argv[optind]);
				TOC;
				printf("offload voice signal resampler use:%u ms\n", ms);
			}
			else {
				usage();
				exit(1);
			}
			break;
		case '?':
		   usage();
		   exit(1);
		   break;
	}
	return 0;
}
