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

#ifndef H_MGMT_ENDIAN_
#define H_MGMT_ENDIAN_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#ifndef ntohs
#define ntohs(x) (x)
#endif

#ifndef htons
#define htons(x) (x)
#endif

#else
/* Little endian. */

#ifndef ntohs
#define ntohs(x)   ((uint16_t)  \
    ((((x) & 0xff00) >> 8) |    \
     (((x) & 0x00ff) << 8)))
#endif

#ifndef htons
#define htons(x) (ntohs(x))
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
