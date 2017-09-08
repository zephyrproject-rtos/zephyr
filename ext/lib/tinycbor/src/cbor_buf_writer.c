/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <tinycbor/cbor.h>
#include <tinycbor/cbor_buf_writer.h>

static inline int
would_overflow(struct cbor_buf_writer *cb, size_t len)
{
    ptrdiff_t remaining = (ptrdiff_t)cb->end;
    remaining -= (ptrdiff_t)cb->ptr;
    remaining -= (ptrdiff_t)len;
    return (remaining < 0);
}

int
cbor_buf_writer(struct cbor_encoder_writer *arg, const char *data, int len)
{
    struct cbor_buf_writer *cb = (struct cbor_buf_writer *) arg;

    if (would_overflow(cb, len)) {
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
    cb->enc.write = cbor_buf_writer;
}

size_t
cbor_buf_writer_buffer_size(struct cbor_buf_writer *cb, const uint8_t *buffer)
{
    return (size_t)(cb->ptr - buffer);
}
