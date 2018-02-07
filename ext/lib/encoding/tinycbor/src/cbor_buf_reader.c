/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#include "cbor_buf_reader.h"
#include "compilersupport_p.h"

/**
 * \addtogroup CborParsing
 * @{
 */

/**
 * Gets 16 bit unsigned value from the passed in ptr location, it also
 * converts it to host byte order
 */
CBOR_INLINE_API uint16_t get16(const uint8_t *ptr)
{
    uint16_t result;
    memcpy(&result, ptr, sizeof(result));
    return cbor_ntohs(result);
}

/**
 * Gets 32 bit unsigned value from the passed in ptr location, it also
 * converts it to host byte order
 */
CBOR_INLINE_API uint32_t get32(const uint8_t *ptr)
{
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));
    return cbor_ntohl(result);
}

/**
 * Gets 64 bit unsigned value from the passed in ptr location, it also
 * converts it to host byte order
 */
CBOR_INLINE_API uint64_t get64(const uint8_t *ptr)
{
    uint64_t result;
    memcpy(&result, ptr, sizeof(result));
    return cbor_ntohll(result);
}

/**
 * Gets a string chunk from the passed in ptr location
 */
CBOR_INLINE_API uintptr_t get_string_chunk(const uint8_t *ptr)
{
    return (uintptr_t)ptr;
}

/**
 * Gets 8 bit unsigned value using the buffer pointed to by the
 * decoder reader from passed in offset
 */
static uint8_t
cbuf_buf_reader_get8(struct cbor_decoder_reader *d, int offset)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return cb->buffer[offset];
}

/**
 * Gets 16 bit unsigned value using the buffer pointed to by the
 * decoder reader from passed in offset
 */
static uint16_t
cbuf_buf_reader_get16(struct cbor_decoder_reader *d, int offset)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return get16(cb->buffer + offset);
}

/**
 * Gets 32 bit unsigned value using the buffer pointed to by the
 * decoder reader from passed in offset
 */
static uint32_t
cbuf_buf_reader_get32(struct cbor_decoder_reader *d, int offset)
{
    uint32_t val;
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    val = get32(cb->buffer + offset);
    return val;
}

/**
 * Gets 64 bit unsigned value using the buffer pointed to by the
 * decoder reader from passed in offset
 */
static uint64_t
cbuf_buf_reader_get64(struct cbor_decoder_reader *d, int offset)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return get64(cb->buffer + offset);
}

static uintptr_t
cbor_buf_reader_get_string_chunk(struct cbor_decoder_reader *d,
                                 int offset, size_t *len)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *)d;

    (void)*len;

    return get_string_chunk(cb->buffer + offset);
}

static uintptr_t
cbor_buf_reader_cmp(struct cbor_decoder_reader *d, char *dst, int src_offset,
                    size_t len)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;

    return !memcmp(dst, cb->buffer + src_offset, len);
}

static uintptr_t
cbor_buf_reader_cpy(struct cbor_decoder_reader *d, char *dst, int src_offset,
                    size_t len)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return (uintptr_t) memcpy(dst, cb->buffer + src_offset, len);
}

void
cbor_buf_reader_init(struct cbor_buf_reader *cb, const uint8_t *buffer,
                     size_t data)
{
    cb->buffer = buffer;
    cb->r.get8 = &cbuf_buf_reader_get8;
    cb->r.get16 = &cbuf_buf_reader_get16;
    cb->r.get32 = &cbuf_buf_reader_get32;
    cb->r.get64 = &cbuf_buf_reader_get64;
    cb->r.cmp = &cbor_buf_reader_cmp;
    cb->r.cpy = &cbor_buf_reader_cpy;
    cb->r.get_string_chunk = &cbor_buf_reader_get_string_chunk;
    cb->r.message_size = data;
}

/** @} */
