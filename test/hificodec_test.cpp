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
#include "mp3reader.h"
#include "sndfile.h"
#include "rpc_client_mp3.h"
#include "rpc_client_shm.h"

//#define INFO printf
#define INFO

using namespace std;

enum {
    kInputBufferSize = 10 * 1024,
    kOutputBufferSize = 4608 * 2,
};

static void mp3_show_hex(char* samples, uint32 size)
{
	int i;
	for (i = 0; i < size; i++) {
		INFO("0x%x ", samples[i]);
	}
	INFO("\n");
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage %s <input file> <output file>\n", argv[0]);
        INFO("Usage %s <input file> <output file>\n", argv[0]);
        return EXIT_FAILURE;
    }
	tAmlMp3DecHdl hdlmp3;

    // Initialize the config.
    tAmlACodecConfig_Mp3DecExternal config;
	memset(&config, 0, sizeof(tAmlACodecConfig_Mp3DecExternal));
    config.equalizerType = flat;
    config.crcEnabled = false;

    // Initialize the decoder.
    hdlmp3 = AmlACodecInit_Mp3Dec(&config);
    INFO("Init mp3dec hdl=%p\n", hdlmp3);

    // Open the input file.
    Mp3Reader mp3Reader;
	INFO("Read mp3 file:%s\n", argv[1]);
    bool success = mp3Reader.init(argv[1]);
    if (!success) {
        fprintf(stderr, "Encountered error reading %s\n", argv[1]);
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
    SNDFILE *handle = sf_open(argv[2], SFM_WRITE, &sfInfo);
    if (handle == NULL) {
        fprintf(stderr, "Encountered error writing %s\n", argv[2]);
        mp3Reader.close();
		AmlACodecDeInit_Mp3Dec(hdlmp3);
        return EXIT_FAILURE;
    }
    INFO("Init outfile=%s\n", argv[2]);

    // Allocate input buffer.
    uint8_t *inputBuf = (uint8_t*)Aml_ACodecMemory_Allocate(kInputBufferSize);
    assert(inputBuf != NULL);

    // Allocate output buffer.
    int16_t *outputBuf = (int16_t*)Aml_ACodecMemory_Allocate(kOutputBufferSize);
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
        config.pInputBuffer = inputBuf;
        config.pOutputBuffer = outputBuf;
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
        Aml_ACodecMemory_Clean(config.pInputBuffer, bytesRead);
        decoderErr = AmlACodecExec_Mp3Dec(hdlmp3, &config);
        if (decoderErr != NO_DECODING_ERROR) {
            fprintf(stderr, "Decoder encountered error:0x%x\n", decoderErr);
            retVal = EXIT_FAILURE;
            break;
        }
        Aml_ACodecMemory_Inv(config.pOutputBuffer, config.outputFrameSize*sizeof(int16_t));
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
