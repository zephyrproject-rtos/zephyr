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

#include <stdbool.h>
#include "util/mcumgr_util.h"

int
ull_to_s(unsigned long long val, int dst_max_len, char *dst)
{
    unsigned long copy;
    int digit;
    int off;
    int len;

    /* First, calculate the length of the resulting string. */
    copy = val;
    for (len = 0; copy != 0; len++) {
        copy /= 10;
    }

    /* A value of 0 still requires one character ("0"). */
    if (len == 0) {
        len = 1;
    }

    /* Ensure the buffer can accommodate the string and terminator. */
    if (len >= dst_max_len - 1) {
        return -1;
    }

    /* Encode the string from right to left. */
    off = len;
    dst[off--] = '\0';
    do {
        digit = val % 10;
        dst[off--] = '0' + digit;

        val /= 10;
    } while (val > 0);

    return len;
}

int
ll_to_s(long long val, int dst_max_len, char *dst)
{
    unsigned long long ull;

    if (val < 0) {
        if (dst_max_len < 1) {
            return -1;
        }

        dst[0] = '-';
        dst_max_len--;
        dst++;

        ull = -val;
    } else {
        ull = val;
    }

    return ull_to_s(ull, dst_max_len, dst);
}
