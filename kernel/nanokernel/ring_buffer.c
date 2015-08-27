/* ring_buffer.c: Simple ring buffer API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <misc/ring_buffer.h>

/**
 * Internal data structure for a buffer header.
 *
 * We want all of this to fit in a single uint32_t. Every item stored in the
 * ring buffer will be one of these headers plus any extra data supplied
 */
struct ring_element {
	uint32_t  type   :16; /**< Application-specific */
	uint32_t  length :8;  /**< length in 32-bit chunks */
	uint32_t  value  :8;  /**< Room for small integral values */
};

int sys_ring_buf_put(struct ring_buf *buf, uint16_t type, uint8_t value,
		     uint32_t *data, uint8_t size32)
{
	uint32_t i, space, index, rc;

	space = sys_ring_buf_space_get(buf);
	if (space >= (size32 + 1)) {
		struct ring_element *header =
				(struct ring_element *)&buf->buf[buf->tail];
		header->type = type;
		header->length = size32;
		header->value = value;

		if (likely(buf->mask)) {
			for (i=0; i < size32; ++i) {
				index = (i + buf->tail + 1) & buf->mask;
				buf->buf[index] = data[i];
			}
			buf->tail = (buf->tail + size32 + 1) & buf->mask;
		} else {
			for (i=0; i < size32; ++i) {
				index = (i + buf->tail + 1) % buf->size;
				buf->buf[index] = data[i];
			}
			buf->tail = (buf->tail + size32 + 1) % buf->size;
		}
		rc = 0;
	} else {
		buf->dropped_put_count++;
		rc = -EMSGSIZE;
	}

	return rc;
}

int sys_ring_buf_get(struct ring_buf *buf, uint16_t *type, uint8_t *value,
		     uint32_t *data, uint8_t *size32)
{
	struct ring_element *header;
	uint32_t i, index;

	if (sys_ring_buf_is_empty(buf)) {
		return -EAGAIN;
	}

	header = (struct ring_element *) &buf->buf[buf->head];

	if (header->length > *size32) {
		*size32 = header->length;
		return -EMSGSIZE;
	}

	*size32 = header->length;
	*type = header->type;
	*value = header->value;

	if (likely(buf->mask)) {
		for (i = 0; i < header->length; ++i) {
			index = (i + buf->head + 1) & buf->mask;
			data[i] = buf->buf[index];
		}
		buf->head = (buf->head + header->length + 1) & buf->mask;
	} else {
		for (i = 0; i < header->length; ++i) {
			index = (i + buf->head + 1) % buf->size;
			data[i] = buf->buf[index];
		}
		buf->head = (buf->head + header->length + 1) % buf->size;
	}

	return 0;
}

