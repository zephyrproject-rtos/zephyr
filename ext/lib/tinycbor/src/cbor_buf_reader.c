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

#include <tinycbor/cbor_buf_reader.h>
#include <tinycbor/extract_number_p.h>

static uint8_t
cbuf_buf_reader_get8(struct cbor_decoder_reader *d, int offset)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return cb->buffer[offset];
}

static uint16_t
cbuf_buf_reader_get16(struct cbor_decoder_reader *d, int offset)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return get16(cb->buffer + offset);
}

static uint32_t
cbuf_buf_reader_get32(struct cbor_decoder_reader *d, int offset)
{
    uint32_t val;
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    val = get32(cb->buffer + offset);
    return val;
}

static uint64_t
cbuf_buf_reader_get64(struct cbor_decoder_reader *d, int offset)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return get64(cb->buffer + offset);
}

static uintptr_t
cbor_buf_reader_cmp(struct cbor_decoder_reader *d, char *dst, int src_offset,
                    size_t len)
{
    struct cbor_buf_reader *cb = (struct cbor_buf_reader *) d;
    return memcmp(dst, cb->buffer + src_offset, len);
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
    cb->r.message_size = data;
}
