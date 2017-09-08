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

#include <inttypes.h>
#include <ctype.h>
#include <stddef.h>

#include "base64/hex.h"

static const char hex_bytes[] = "0123456789abcdef";

/*
 * Turn byte array into a printable array. I.e. "\x01" -> "01"
 *
 * @param src_v		Data to convert
 * @param src_len	Number of bytes of input
 * @param dst		String where to place the results
 * @param dst_len	Size of the target string
 *
 * @return		Pointer to 'dst' if successful; NULL on failure
 */
char *
hex_format(void *src_v, int src_len, char *dst, int dst_len)
{
    int i;
    uint8_t *src = (uint8_t *)src_v;
    char *tgt = dst;

    if (dst_len <= src_len * 2) {
        return NULL;
    }
    for (i = 0; i < src_len; i++) {
        tgt[0] = hex_bytes[(src[i] >> 4) & 0xf];
        tgt[1] = hex_bytes[src[i] & 0xf];
        tgt += 2;
        dst_len -= 2;
    }
    *tgt = '\0';
    return dst;
}

/*
 * Turn string of hex decimals into a byte array. I.e. "01" -> "\x01
 *
 * @param src		String to convert
 * @param src_len	Number of bytes in input string
 * @param dst_v		Memory location to place the result
 * @param dst_len	Amount of space for the result
 *
 * @return		-1 on failure; number of bytes of input
 */
int
hex_parse(const char *src, int src_len, void *dst_v, int dst_len)
{
    int i;
    uint8_t *dst = (uint8_t *)dst_v;
    char c;

    if (src_len & 0x1) {
        return -1;
    }
    if (dst_len * 2 < src_len) {
        return -1;
    }
    for (i = 0; i < src_len; i++, src++) {
        c = *src;
        if (isdigit((int) c)) {
            c -= '0';
        } else if (c >= 'a' && c <= 'f') {
            c -= ('a' - 10);
        } else if (c >= 'A' && c <= 'F') {
            c -= ('A' - 10);
        } else {
            return -1;
        }
        if (i & 1) {
            *dst |= c;
            dst++;
            dst_len--;
        } else {
            *dst = c << 4;
        }
    }
    return src_len >> 1;
}
