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

#ifndef CBOR_BUF_WRITER_H
#define CBOR_BUF_WRITER_H

#include "cbor_encoder_writer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cbor_buf_writer {
    struct cbor_encoder_writer enc;
    uint8_t *ptr;
    const uint8_t *end;
    int bytes_needed;
};

struct CborEncoder;

void cbor_buf_writer_init(struct cbor_buf_writer *cb, uint8_t *buffer,
                          size_t data);
size_t cbor_buf_writer_buffer_size(struct cbor_buf_writer *cb,
                                   const uint8_t *buffer);
size_t cbor_encoder_get_extra_bytes_needed(const struct CborEncoder *encoder);
size_t cbor_encoder_get_buffer_size(const struct CborEncoder *encoder,
                                    const uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* CBOR_BUF_WRITER_H */
