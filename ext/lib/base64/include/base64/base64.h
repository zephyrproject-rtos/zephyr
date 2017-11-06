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
#ifndef __UTIL_BASE64_H
#define __UTIL_BASE64_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int base64_encode(const void *, int, char *, uint8_t);
int base64_decode(const char *, void *buf);
int base64_pad(char *, int);
int base64_decode_len(const char *str);

#define BASE64_ENCODE_SIZE(__size) (((((__size) - 1) / 3) * 4) + 4)

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_BASE64_H__ */
