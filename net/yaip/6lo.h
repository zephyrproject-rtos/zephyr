/** @file
 @brief 6lowpan handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NET_6LO_H
#define __NET_6LO_H

#include <misc/slist.h>
#include <stdint.h>

#include <net/nbuf.h>

/**
 *  @brief Compress IPv6 packet as per RFC 6282
 *
 *  @details After this IPv6 packet and next header(if UDP), headers
 *  are compressed as per RFC 6282. After header compression data
 *  will be adjusted according to remaining space in fragments.
 *
 *  @param Pointer to network buffer
 *  @param iphc true for IPHC compression, false for IPv6 dispatch header
 *
 *  @return True on success, false otherwise
 */
bool net_6lo_compress(struct net_buf *buf, bool iphc);

/**
 *  @brief Unompress IPv6 packet as per RFC 6282
 *
 *  @details After this IPv6 packet and next header(if UDP), headers
 *  are uncompressed as per RFC 6282. After header uncompression data
 *  will be adjusted according to remaining space in fragments.
 *
 *  @param Pointer to network buffer
 *
 *  @return True on success, false otherwise
 */

bool net_6lo_uncompress(struct net_buf *buf);

#endif /* __NET_6LO_H */
