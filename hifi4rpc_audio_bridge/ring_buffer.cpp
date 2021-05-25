/*
 * Copyright (C) 2021-2025 Amlogic, Inc. All rights reserved.
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
 * ring buffer operation api
 *
 * Author: ziheng li <ziheng.li@amlogic.com>
 * Version:
 * - 0.1        init
 */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ring_buffer.h"

using namespace std;

#define min(x, y) ((x) < (y) ? (x) : (y))

static char is_power_of_2(unsigned int n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static unsigned int roundup_power_of_2(unsigned int a)
{
	if (a == 0)
		return 0;

	unsigned int position = 0;
	for (int i = a; i != 0; i >>= 1)
		position++;

	return (unsigned int)(1 << position);
}

int RingBuf::rb_init(unsigned int size)
{
	if (!is_power_of_2(size))
		size = roundup_power_of_2(size);

	fifo.buffer = NULL;
	if((fifo.buffer = (unsigned char *)(malloc(size * sizeof(unsigned char)))) == NULL) {
		return -1;
	}

	fifo.in = 0;
	fifo.out = 0;
	fifo.size = size;
	return 0;
}

void RingBuf::rb_deinit(void)
{
	free(fifo.buffer);
	fifo.in = 0;
	fifo.out = 0;
	fifo.size = 0;
}

unsigned int RingBuf::rb_write(const unsigned char *data, unsigned int len)
{
	unsigned int l;

	//calculate the mininum value between the remaining size of fifo buffer and  the size of the buffer that needs to be written
	len = min(len,fifo.size - fifo.in + fifo.out);

	//calculate the remain size of the buffer from start point to end point in the continuous space
	l = min(len, fifo.size - (fifo.in  & (fifo.size - 1)));

	memcpy(fifo.buffer + (fifo.in & (fifo.size - 1)), data, l);

	memcpy(fifo.buffer, data + l, len - l);

	fifo.in += len;

	return len;
}

unsigned int RingBuf::rb_read(unsigned char *data, unsigned int len)
{
	unsigned int l;

	len = min(len, fifo.in - fifo.out);

	l = min(len, fifo.size - (fifo.out & (fifo.size - 1)));

	memcpy(data, fifo.buffer + (fifo.out & (fifo.size - 1)), l);

	memcpy(data + l, fifo.buffer, len - l);

	fifo.out += len;

	return len;
}

unsigned int RingBuf::rb_get_size_of_already_used(void)
{
	return fifo.in - fifo.out;
}

unsigned int RingBuf::rb_get_size_of_left(void)
{
	return fifo.size - fifo.in + fifo.out;
}


