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

#ifndef CBOR_CNT_WRITER_H
#define CBOR_CNT_WRITER_H

#include "cbor.h"


#ifdef __cplusplus
extern "C" {
#endif

    /* use this count writer if you want to try out a cbor encoding to see
     * how long it would be (before allocating memory). This replaced the
     * code in tinycbor.h that would try to do this once the encoding failed
     * in a buffer.  Its much easier to understand this way (for me)
     */

struct CborCntWriter {
    struct cbor_encoder_writer enc;
};

static inline int
cbor_cnt_writer(struct cbor_encoder_writer *arg, const char *data, int len) {
    struct CborCntWriter *cb = (struct CborCntWriter *) arg;
    cb->enc.bytes_written += len;
    return CborNoError;
}

static inline void
cbor_cnt_writer_init(struct CborCntWriter *cb) {
    cb->enc.bytes_written = 0;
    cb->enc.write = &cbor_cnt_writer;
}

#ifdef __cplusplus
}
#endif

#endif /* CBOR_CNT_WRITER_H */

