/** @file
 @brief 802.15.4 fragment handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_IEEE802154_FRAGMENT_H__
#define __NET_IEEE802154_FRAGMENT_H__

#include <misc/slist.h>
#include <zephyr/types.h>

#include <net/net_pkt.h>

/**
 *  @brief Fragment IPv6 packet as per RFC 6282
 *
 *  @details After IPv6 compression, transmission of IPv6 over 802.15.4
 *  needs to be fragmented. Every fragment will have fragmentation header
 *  data size, data offset, data tag and payload.
 *
 *  @param Pointer to network packet
 *  @param Header difference between original IPv6 header and compressed header
 *
 *  @return True in case of success, false otherwise
 */
bool ieee802154_fragment(struct net_pkt *pkt, int hdr_diff);

/**
 *  @brief Reassemble 802.15.4 fragments as per RFC 6282
 *
 *  @details If the data does not fit into single fragment whole IPv6 packet
 *  comes in number of fragments. This function will reassemble them all as
 *  per data tag, data offset and data size. First packet is uncompressed
 *  immediately after reception.
 *
 *  @param Pointer to network fragment, which gets updated to full reassembled
 *         packet when verdict is NET_CONTINUE
 *
 *  @return NET_CONTINUE reassembly done, pkt is complete
 *          NET_OK waiting for other fragments,
 *          NET_DROP invalid fragment.
 */

enum net_verdict ieee802154_reassemble(struct net_pkt *pkt);

#endif /* __NET_IEEE802154_FRAGMENT_H__ */
