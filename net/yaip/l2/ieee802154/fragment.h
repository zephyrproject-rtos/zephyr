/** @file
 @brief 802.15.4 fragment handler

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

#ifndef __NET_FRAGMENT_H
#define __NET_FRAGMENT_H

#include <misc/slist.h>
#include <stdint.h>

#include <net/nbuf.h>

/**
 *  @brief Fragment IPv6 packet as per RFC 6282
 *
 *  @details After IPv6 compression, transimission of IPv6 over 802.15.4
 *  needs to be fragmented. Every fragment will have frgmentation header
 *  data size, data offset, data tag and payload.
 *
 *  @param Pointer to network buffer
 *  @param Header difference between origianl IPv6 header and compressed header
 *
 *  @return True in case of success, false otherwise
 */
bool ieee802154_fragment(struct net_buf *buf, int hdr_diff);

/**
 *  @brief Reassemble 802.15.4 fragments as per RFC 6282
 *
 *  @details If the data does not fit into sinle fragment whole IPv6 packet
 *  comes in number of fragments. This funtion will reassemble them all as
 *  per data tag, data offset and data size. First packet is uncompressed
 *  immediately after reception.
 *
 *  @param Pointer to network fragment
 *  @param Pointer to network buf (on NET_OK verdict only)
 *
 *  @return NET_OK reassembly done,
 *          NET_CONTINUE waiting for other fragments,
 *          NET_DROP invalid fragment.
 */

enum net_verdict ieee802154_reassemble(struct net_buf *frag,
				       struct net_buf **buf);

#endif /* __NET_FRAGMENT_H */
