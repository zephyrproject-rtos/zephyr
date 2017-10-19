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

#include <tinycbor/cbor_mbuf_reader.h>
#include <tinycbor/compilersupport_p.h>
#include <os/os_mbuf.h>

static uint8_t
cbor_mbuf_reader_get8(struct cbor_decoder_reader *d, int offset)
{
    uint8_t val;
    struct cbor_mbuf_reader *cb = (struct cbor_mbuf_reader *) d;

    os_mbuf_copydata(cb->m, offset + cb->init_off, sizeof(val), &val);
    return val;
}

static uint16_t
cbor_mbuf_reader_get16(struct cbor_decoder_reader *d, int offset)
{
    uint16_t val;
    struct cbor_mbuf_reader *cb = (struct cbor_mbuf_reader *) d;

    os_mbuf_copydata(cb->m, offset + cb->init_off, sizeof(val), &val);
    return cbor_ntohs(val);
}

static uint32_t
cbor_mbuf_reader_get32(struct cbor_decoder_reader *d, int offset)
{
    uint32_t val;
    struct cbor_mbuf_reader *cb = (struct cbor_mbuf_reader *) d;

    os_mbuf_copydata(cb->m, offset + cb->init_off, sizeof(val), &val);
    return cbor_ntohl(val);
}

static uint64_t
cbor_mbuf_reader_get64(struct cbor_decoder_reader *d, int offset)
{
    uint64_t val;
    struct cbor_mbuf_reader *cb = (struct cbor_mbuf_reader *) d;

    os_mbuf_copydata(cb->m, offset + cb->init_off, sizeof(val), &val);
    return cbor_ntohll(val);
}

static uintptr_t
cbor_mbuf_reader_cmp(struct cbor_decoder_reader *d, char *buf, int offset,
                     size_t len)
{
    struct cbor_mbuf_reader *cb = (struct cbor_mbuf_reader *) d;
    return os_mbuf_cmpf(cb->m, offset + cb->init_off, buf, len);
}

static uintptr_t
cbor_mbuf_reader_cpy(struct cbor_decoder_reader *d, char *dst, int offset,
                     size_t len)
{
    int rc;
    struct cbor_mbuf_reader *cb = (struct cbor_mbuf_reader *) d;

    rc = os_mbuf_copydata(cb->m, offset + cb->init_off, len, dst);
    if (rc == 0) {
        return true;
    }
    return false;
}

void
cbor_mbuf_reader_init(struct cbor_mbuf_reader *cb, struct os_mbuf *m,
                      int initial_offset)
{
    struct os_mbuf_pkthdr *hdr;

    cb->r.get8 = &cbor_mbuf_reader_get8;
    cb->r.get16 = &cbor_mbuf_reader_get16;
    cb->r.get32 = &cbor_mbuf_reader_get32;
    cb->r.get64 = &cbor_mbuf_reader_get64;
    cb->r.cmp = &cbor_mbuf_reader_cmp;
    cb->r.cpy = &cbor_mbuf_reader_cpy;

    assert(OS_MBUF_IS_PKTHDR(m));
    hdr = OS_MBUF_PKTHDR(m);
    cb->m = m;
    cb->init_off = initial_offset;
    cb->r.message_size = hdr->omp_len - initial_offset;
}
