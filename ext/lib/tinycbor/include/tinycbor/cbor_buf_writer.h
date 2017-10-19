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

#ifndef CBOR_BUF_WRITER_H
#define CBOR_BUF_WRITER_H

#ifdef __cplusplus
extern "C" {
#endif

struct cbor_buf_writer {
    struct cbor_encoder_writer enc;
    uint8_t *ptr;
    const uint8_t *end;
};

void cbor_buf_writer_init(struct cbor_buf_writer *cb, uint8_t *buffer,
                          size_t data);
size_t cbor_buf_writer_buffer_size(struct cbor_buf_writer *cb,
                                   const uint8_t *buffer);
#ifdef __cplusplus
}
#endif

#endif /* CBOR_BUF_WRITER_H */
