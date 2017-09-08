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

#ifndef CBOR_MBUF_WRITER_H
#define CBOR_MBUF_WRITER_H

#include <tinycbor/cbor.h>
#include <os/os_mbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cbor_mbuf_writer {
    struct cbor_encoder_writer enc;
    struct os_mbuf *m;
};

void cbor_mbuf_writer_init(struct cbor_mbuf_writer *cb, struct os_mbuf *m);

#ifdef __cplusplus
}
#endif

#endif /* CBOR_MBUF_WRITER_H */

