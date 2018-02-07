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

#include "cbor.h"
#include "cbor_buf_writer.h"

CBOR_INLINE_API int
would_overflow(struct cbor_buf_writer *cb, size_t len)
{
    ptrdiff_t remaining = (ptrdiff_t)cb->end;

    if (!remaining)
        return 1;
    remaining -= (ptrdiff_t)cb->ptr;
    remaining -= (ptrdiff_t)len;
    return (remaining < 0);
}

int
cbor_buf_writer(struct cbor_encoder_writer *arg, const char *data, int len)
{
    struct cbor_buf_writer *cb = (struct cbor_buf_writer *) arg;

    if (would_overflow(cb, len)) {
        if (cb->end != NULL) {
            len -= cb->end - cb->ptr;
            cb->end = NULL;
            cb->bytes_needed = 0;
        }

        cb->bytes_needed += len;

        return CborErrorOutOfMemory;
    }

    memcpy(cb->ptr, data, len);
    cb->ptr += len;
    cb->enc.bytes_written += len;
    return CborNoError;
}

void
cbor_buf_writer_init(struct cbor_buf_writer *cb, uint8_t *buffer, size_t size)
{
    cb->ptr = buffer;
    cb->end = buffer + size;
    cb->enc.bytes_written = 0;
    cb->bytes_needed = 0;
    cb->enc.write = cbor_buf_writer;
}

size_t
cbor_buf_writer_buffer_size(struct cbor_buf_writer *cb, const uint8_t *buffer)
{
    return (size_t)(cb->ptr - buffer);
}

size_t cbor_encoder_get_extra_bytes_needed(const CborEncoder *encoder)
{
    struct cbor_buf_writer *wr = (struct cbor_buf_writer *)encoder->writer;

    return wr->end ? 0 : (size_t)wr->bytes_needed;
}

size_t cbor_encoder_get_buffer_size(const CborEncoder *encoder, const uint8_t *buffer)
{
    struct cbor_buf_writer *wr = (struct cbor_buf_writer *)encoder->writer;

    return (size_t)(wr->ptr - buffer);
}

uint8_t *_cbor_encoder_get_buffer_pointer(const CborEncoder *encoder)
{
    struct cbor_buf_writer *wr = (struct cbor_buf_writer *)encoder->writer;

    return wr->ptr;
}

