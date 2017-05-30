/**
 * \file
 *
 * \brief Stream utility for the IoT service.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "iot/stream_writer.h"
#include <string.h>
#include <asf.h>

void stream_writer_init(struct stream_writer * writer, char *buffer, size_t max_length, stream_writer_write_func_t func, void *priv_data)
{
	writer->max_size = max_length;
	writer->buffer = buffer;
	writer->written = 0;
	writer->write_func = func;
	writer->priv_data = priv_data;
}

void stream_writer_send_8(struct stream_writer * writer, int8_t value)
{
	int remain = writer->max_size - writer->written;
	
	if (remain < 1) {
		stream_writer_send_remain(writer);
	}
	
	writer->buffer[writer->written++] = (char)value;
}

void stream_writer_send_16BE(struct stream_writer * writer, int16_t value)
{
	stream_writer_send_8(writer, (value >> 8) & 0xFF);
	stream_writer_send_8(writer, value & 0xFF);
}

void stream_writer_send_16LE(struct stream_writer * writer, int16_t value)
{
	stream_writer_send_8(writer, value & 0xFF);
	stream_writer_send_8(writer, (value >> 8) & 0xFF);
}	

void stream_writer_send_32BE(struct stream_writer * writer, int32_t value)
{
	stream_writer_send_8(writer, (value >> 24) & 0xFF);
	stream_writer_send_8(writer, (value >> 16) & 0xFF);
	stream_writer_send_8(writer, (value >> 8) & 0xFF);
	stream_writer_send_8(writer, value & 0xFF);
}

void stream_writer_send_32LE(struct stream_writer * writer, int32_t value)
{
	stream_writer_send_8(writer, value & 0xFF);
	stream_writer_send_8(writer, (value >> 8) & 0xFF);
	stream_writer_send_8(writer, (value >> 16) & 0xFF);
	stream_writer_send_8(writer, (value >> 24) & 0xFF);
}

void stream_writer_send_buffer(struct stream_writer * writer, const char *buffer, size_t length)
{
	for (; length > 0; length--, buffer++) {
		stream_writer_send_8(writer, *buffer);
	}
}

void stream_writer_send_remain(struct stream_writer * writer)
{
	if(writer->written > 0) {
		writer->write_func(writer->priv_data, writer->buffer, writer->written);
		writer->written = 0;
	}
}
