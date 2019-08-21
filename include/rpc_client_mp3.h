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
typedef struct {
        /*
         * INPUT:
         * Pointer to the input buffer that contains the encoded bistream data.
         * The data is filled in such that the first bit transmitted is
         * the most-significant bit (MSB) of the first array element.
         * The buffer is accessed in a linear fashion for speed, and the number of
         * bytes consumed varies frame to frame.
         * The calling environment can change what is pointed to between calls to
         * the decode function, library, as long as the inputBufferCurrentLength,
         * and inputBufferUsedLength are updated too. Also, any remaining bits in
         * the old buffer must be put at the beginning of the new buffer.
         */
        uint8      *pInputBuffer;

        /*
         * INPUT:
         * Number of valid bytes in the input buffer, set by the calling
         * function. After decoding the bitstream the library checks to
         * see if it when past this value; it would be to prohibitive to
         * check after every read operation. This value is not modified by
         * the MP3 library.
         */
        int32     inputBufferCurrentLength;

        /*
         * INPUT/OUTPUT:
         * Number of elements used by the library, initially set to zero by
         * the function pvmp3_resetDecoder(), and modified by each
         * call to pvmp3_framedecoder().
         */
        int32     inputBufferUsedLength;

        /*
         * OUTPUT:
         * holds the predicted frame size. It used on the test console, for parsing
         * purposes.
         */
        uint32     CurrentFrameLength;

        /*
         * INPUT:
         * This variable holds the type of equalization used
         *
         *
         */
        e_equalization     equalizerType;


        /*
         * INPUT:
         * The actual size of the buffer.
         * This variable is not used by the library, but is used by the
         * console test application. This parameter could be deleted
         * if this value was passed into these function.
         */
        int32     inputBufferMaxLength;

        /*
         * OUTPUT:
         * The number of channels decoded from the bitstream.
         */
        int16       num_channels;

        /*
         * OUTPUT:
         * The number of channels decoded from the bitstream.
         */
        int16       version;

        /*
         * OUTPUT:
         * The sampling rate decoded from the bitstream, in units of
         * samples/second.
         */
        int32       samplingRate;

        /*
         * OUTPUT:
         * The bit depth decoded from the bitstream, in units of bit.
         */
        int32       bitDepth;

        /*
         * OUTPUT:
         * This value is the bitrate in units of bits/second. IT
         * is calculated using the number of bits consumed for the current frame,
         * and then multiplying by the sampling_rate, divided by points in a frame.
         * This value can changes frame to frame.
         */
        int32       bitRate;

        /*
         * INPUT/OUTPUT:
         * In: Inform decoder how much more room is available in the output buffer in int16 samples
         * Out: Size of the output frame in 16-bit words, This value depends on the mp3 version
         */
        int32     outputFrameSize;

        /*
         * INPUT:
         * Flag to enable/disable crc error checking
         */
        int32     crcEnabled;

        /*
         * OUTPUT:
         * This value is used to accumulate bit processed and compute an estimate of the
         * bitrate. For debugging purposes only, as it will overflow for very long clips
         */
        uint32     totalNumberOfBitsUsed;


        /*
         * INPUT: (but what is pointed to is an output)
         * Pointer to the output buffer to hold the 16-bit PCM audio samples.
         * If the output is stereo, both left and right channels will be stored
         * in this one buffer.
         */

        int16       *pOutputBuffer;

} tAmlACodecConfig_Mp3DecExternal;

tAmlMp3DecHdl AmlACodecInit_Mp3Dec(tAmlACodecConfig_Mp3DecExternal *pconfig);
void AmlACodecDeInit_Mp3Dec(tAmlMp3DecHdl hMp3Dec);
ERROR_CODE AmlACodecExec_Mp3Dec(tAmlMp3DecHdl hMp3, tAmlACodecConfig_Mp3DecExternal *pconfig);

ERROR_CODE AmlACodecExec_UserAllocIoShm_Mp3Dec(tAmlMp3DecHdl hMp3, tAmlACodecConfig_Mp3DecExternal *pconfig);

#ifdef __cplusplus
}
#endif

#endif // end _OFFLOAD_ACODEC_MP3_H_
