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
#include <os/os_mbuf.h>
#include <tinycbor/cbor.h>
#include <tinycbor/cbor_mbuf_writer.h>

int
cbor_mbuf_writer(struct cbor_encoder_writer *arg, const char *data, int len)
{
    int rc;
    struct cbor_mbuf_writer *cb = (struct cbor_mbuf_writer *) arg;

    rc = os_mbuf_append(cb->m, data, len);
    if (rc) {
        return CborErrorOutOfMemory;
    }
    cb->enc.bytes_written += len;
    return CborNoError;
}


void
cbor_mbuf_writer_init(struct cbor_mbuf_writer *cb, struct os_mbuf *m)
{
    cb->m = m;
    cb->enc.bytes_written = 0;
    cb->enc.write = &cbor_mbuf_writer;
}

