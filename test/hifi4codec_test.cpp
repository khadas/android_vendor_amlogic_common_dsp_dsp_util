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
#include "mp3reader.h"
#include "sndfile.h"
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "rpc_client_tinyalsa.h"
#include "aipc_type.h"


uint32_t audio_play_data[] = {
#include "sinewav_48k_24bits_stereo_l1k_r2k_1s.in"
#include "sinewav_48k_24bits_stereo_l1k_r2k_1s.in"
};

uint32_t audio_play_data_len = sizeof(audio_play_data);

//#define INFO printf
#define INFO

using namespace std;

static void mp3_show_hex(char* samples, uint32 size)
{
	int i;
	for (i = 0; i < size; i++) {
		INFO("0x%x ", samples[i]);
	}
	INFO("\n");
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
	printf("arm: pconfig:%p\n", pconfig);
	tAmlPcmhdl p = pcm_client_open(0, 0, 0, pconfig);
	tAcodecShmHdl hShmBuf;
	printf("pcm_open pcm=%p\n", p);

	uint8_t *play_data = (uint8_t *)audio_play_data;
	int in_fr = pcm_bytes_to_frame(p, audio_play_data_len);
	int i, fr = 0;
	const int ms = 36;
	const int oneshot = 48 * ms; // 1728 samples
	uint32_t size = pcm_frame_to_bytes(p, oneshot);
	hShmBuf = Aml_ACodecMemory_Allocate(size);
	void *buf = Aml_ACodecMemory_GetVirtAddr(hShmBuf);
	void *phybuf = Aml_ACodecMemory_GetPhyAddr(hShmBuf);
	for (i = 0; i + oneshot <= in_fr; i += fr) {
		// fill to buf first to simulate customer's case
		memcpy(buf, play_data + pcm_frame_to_bytes(p, i), size);
		Aml_ACodecMemory_Clean(phybuf, size);
		fr = pcm_client_writei(p, phybuf, oneshot);
		printf("%dms pcm_write i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
			  ms, i, p, buf, oneshot, fr);
		// XXX: wrapper layer ensure fr == oneshot
	}
	int r = pcm_client_close(p);
	printf("pcm_close pcm=%p r=%d\n", p, r);

	Aml_ACodecMemory_Free(hShmBuf);
	free(pconfig);
}


#define PCM_CAPTURE_SAMPLES (48000*2)
static int pcm_capture_test(int argc, char* argv[])
{
	rpc_pcm_config* pconfig = (rpc_pcm_config*)malloc(sizeof(rpc_pcm_config));
	pconfig->channels = 2;
	pconfig->rate = 48000;
	pconfig->format = PCM_FORMAT_S32_LE;
	pconfig->period_size = 1024;
	pconfig->period_count = 4;
	pconfig->start_threshold = 1024;
	pconfig->silence_threshold = 1024*2;
	pconfig->stop_threshold = 1024*2;
	tAmlPcmhdl p = pcm_client_open(0, DEVICE_TDMIN_B, PCM_IN, pconfig);
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
	uint32_t size = pcm_frame_to_bytes(p, oneshot);
	hShmBuf = Aml_ACodecMemory_Allocate(size);
	void *buf = Aml_ACodecMemory_GetVirtAddr(hShmBuf);
	void *phybuf = Aml_ACodecMemory_GetPhyAddr(hShmBuf);
	for (i = 0; i + oneshot <= in_fr; i += fr) {
		fr = pcm_client_readi(p, phybuf, oneshot);
		Aml_ACodecMemory_Inv(phybuf, size);
		fwrite(buf, sizeof(char), size, filecap);
		printf("%dms pcm_read i=%d pcm=%p buf=%p in_fr=%d -> fr=%d xxx\n",
			  ms, i, p, buf, oneshot, fr);
	}
	pcm_client_close(p);
	Aml_ACodecMemory_Free(hShmBuf);
	free(pconfig);
	fclose(filecap);
}



static int mp3_offload_dec(int argc, char* argv[]) {
	tAmlMp3DecHdl hdlmp3;

    // Initialize the config.
    tAmlACodecConfig_Mp3DecExternal config;
	memset(&config, 0, sizeof(tAmlACodecConfig_Mp3DecExternal));
    config.equalizerType = flat;
    config.crcEnabled = false;

    // Initialize the decoder.
    hdlmp3 = AmlACodecInit_Mp3Dec(&config, 1);
    INFO("Init mp3dec hdl=%p\n", hdlmp3);

    // Open the input file.
    Mp3Reader mp3Reader;
	INFO("Read mp3 file:%s\n", argv[0]);
    bool success = mp3Reader.init(argv[0]);
    if (!success) {
        fprintf(stderr, "Encountered error reading %s\n", argv[0]);
		AmlACodecDeInit_Mp3Dec(hdlmp3);
        return EXIT_FAILURE;
    }
    INFO("Init mp3reader\n");

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
    INFO("Init outfile=%s\n", argv[1]);

    // Allocate input buffer.
    tAcodecShmHdl hShmInput = Aml_ACodecMemory_Allocate(kInputBufferSize);
    uint8_t *inputBuf = (uint8_t*)Aml_ACodecMemory_GetVirtAddr(hShmInput);
	void* inputphy = Aml_ACodecMemory_GetPhyAddr(hShmInput);
    assert(inputBuf != NULL);

    // Allocate output buffer.
    tAcodecShmHdl hShmOutput = Aml_ACodecMemory_Allocate(kOutputBufferSize);
    int16_t *outputBuf = (int16_t*)Aml_ACodecMemory_GetVirtAddr(hShmOutput);
	void* outputphy = Aml_ACodecMemory_GetPhyAddr(hShmOutput);
    assert(outputBuf != NULL);

	INFO("Init input %p, output buffer %p\n", inputBuf, outputBuf);

    // Decode loop.
    int retVal = EXIT_SUCCESS;
    while (1) {
        // Read input from the file.
        uint32_t bytesRead;
        success = mp3Reader.getFrame(inputBuf, &bytesRead);
        if (!success) {
			INFO("EOF\n");
            break;
        }

        // Set the input config.
        config.inputBufferCurrentLength = bytesRead;
        config.inputBufferMaxLength = 0;
        config.inputBufferUsedLength = 0;
        config.pInputBuffer = (uint8_t*)inputphy;
        config.pOutputBuffer = (int16_t*)outputphy;
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
        Aml_ACodecMemory_Clean(inputphy, bytesRead);
        decoderErr = AmlACodecExec_Mp3Dec(hdlmp3, &config);
        if (decoderErr != NO_DECODING_ERROR) {
            fprintf(stderr, "Decoder encountered error:0x%x\n", decoderErr);
            retVal = EXIT_FAILURE;
            break;
        }
        Aml_ACodecMemory_Inv(outputphy, config.outputFrameSize*sizeof(int16_t));
		/*INFO("\n*************arm:%p**********************\n", outputBuf);
		mp3_show_hex((char*)outputBuf, config.outputFrameSize*sizeof(int16_t));
		INFO("\n***********************************\n");*/
        //INFO("config.outputFrameSize:%d\n", config.outputFrameSize);
        sf_writef_short(handle, outputBuf,
                        config.outputFrameSize / sfInfo.channels);
    }

    // Close input reader and output writer.
    mp3Reader.close();
    INFO("reader close\n");
    sf_close(handle);
    INFO("write clonse\n");

    // Free allocated memory.
    Aml_ACodecMemory_Free((tAcodecShmHdl)inputBuf);
    Aml_ACodecMemory_Free((tAcodecShmHdl)outputBuf);
    AmlACodecDeInit_Mp3Dec(hdlmp3);
    INFO("mp3 decoder done\n");

    return retVal;
}

#define IPC_UNIT_TEST_REPEAT 50
static int ipc_uint_tset(void) {
	unsigned int i;
	unsigned int num_repeat = IPC_UNIT_TEST_REPEAT;
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
		} else {
			printf("ipc unittest pass:%d\n", IPC_UNIT_TEST_REPEAT - num_repeat);
		}
	}
	xAudio_Ipc_Deinit(arpchdl);
	return 0;
}

#define SHM_UNIT_TEST_REPEAT 50
static int shm_uint_tset(void)
{
	unsigned int i;
	unsigned int num_repeat = SHM_UNIT_TEST_REPEAT;
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
			char* k = (char*)pVirDst;
			for(i = 0; i < sizeof(samples); i++)
				printf("0x%x ", k[i]);
			printf("\n");
			printf("ipc unittest pass:%d\n", SHM_UNIT_TEST_REPEAT - num_repeat);
		}
		Aml_ACodecMemory_Free(hSrc);
		Aml_ACodecMemory_Free(hDst);
	}
	return 0;

}


static void usage()
{
	printf ("ipc unit test usage: hificodec_test --ipc\n");
	printf ("\n");
	printf ("shared memory unit test usage: hificodec_test --shm\n");
	printf ("\n");
	printf ("mp3dec Usage: hificodec_test --mp3dec input_file output_file\n");
	printf ("\n");
	printf ("pcmplay Usage: hificodec_test --pcmplay pcm_file\n");
	printf ("\n");
	printf ("playcap Usage: hificodec_test --pcmcap  pcm_file\n");
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
			ipc_uint_tset();
			break;
		case 2:
			shm_uint_tset();
			break;
		case 3:
			if (2 == argc - optind)
				mp3_offload_dec(argc - optind, &argv[optind]);
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
			if (1 == argc - optind)
				pcm_capture_test(argc - optind, &argv[optind]);
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
