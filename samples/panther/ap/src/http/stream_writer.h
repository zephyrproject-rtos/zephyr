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

#ifndef STREAM_WRITTER_H_INCLUDED
#define STREAM_WRITTER_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*stream_writer_write_func_t)(void *module, char *buffer, size_t buffer_len);

struct stream_writer {
	size_t max_size;
	size_t written;
	stream_writer_write_func_t write_func;
	void *priv_data;
	char *buffer;
};

/**
 * \brief Initialize the Stream writer module.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  buffer          Buffer which will be used for the storing the data.
 * \param[in]  max_length      Maximum size of buffer.
 * \param[in]  func            Function to be called when the buffer is full.
 * \param[in]  priv_data       Private data. It is passed along when callback was called.
 */
void stream_writer_init(struct stream_writer * writer, char *buffer, size_t max_length, stream_writer_write_func_t func, void *priv_data);

/**
 * \brief Write 8bit to the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  value           Value will be written.
 */
void stream_writer_send_8(struct stream_writer * writer, int8_t value);

/**
 * \brief Write 16bit big endian value to the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  value           Value will be written.
 */
void stream_writer_send_16BE(struct stream_writer * writer, int16_t value);

/**
 * \brief Write 16bit little endian value to the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  value           Value will be written.
 */
void stream_writer_send_16LE(struct stream_writer * writer, int16_t value);

/**
 * \brief Write 32bit big endian value to the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  value           Value will be written.
 */
void stream_writer_send_32BE(struct stream_writer * writer, int32_t value);

/**
 * \brief Write 32bit little endian value to the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  value           Value will be written.
 */
void stream_writer_send_32LE(struct stream_writer * writer, int32_t value);

/**
 * \brief Write buffer to the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 * \param[in]  buffer          Buffer will be written.
 * \param[in]  length          Size of the buffer.
 */
void stream_writer_send_buffer(struct stream_writer * writer, const char *buffer, size_t length);

/**
 * \brief Process remain data in the writer.
 *
 * \param[in]  writer          Pointer of stream writer.
 */
void stream_writer_send_remain(struct stream_writer * writer);


#ifdef __cplusplus
}
#endif

#endif /* STREAM_WRITTER_H_INCLUDED */