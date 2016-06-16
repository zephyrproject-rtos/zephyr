/*
 * Copyright (c) 2016 Intel Corporation.
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

#ifdef CONFIG_NET_L2_IEEE802154_DEBUG
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_DEBUG 1
#endif

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/nbuf.h>

#include <errno.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"

static enum net_verdict ieee802154_recv(struct net_if *iface,
					struct net_buf *buf)
{
	struct net_linkaddr *ll_addr;
	struct ieee802154_mpdu mpdu;

	if (!ieee802154_validate_frame(net_nbuf_ll(buf),
				       net_buf_frags_len(buf), &mpdu)) {
		return NET_DROP;
	}

	/**
	 * We do not support other frames than MAC Data Service ones
	 * ToDo: Support MAC Management Service frames
	 */
	if (mpdu.mhr.fs->fc.frame_type != IEEE802154_FRAME_TYPE_DATA) {
		NET_DBG("Unsupported frame type found");
		return NET_DROP;
	}

	/**
	 * We now remove the size of the MFR from the total length
	 * so net_core will not count it as IP data.
	 */
	buf->frags->len -= IEEE802154_MFR_LENGTH;

	net_nbuf_set_ll_reserve(buf, mpdu.payload - (void *)net_nbuf_ll(buf));
	net_buf_pull(buf->frags, net_nbuf_ll_reserve(buf));

	/* ToDo: handle short address */
	if (mpdu.mhr.fs->fc.src_addr_mode != IEEE802154_ADDR_MODE_NONE) {
		ll_addr = net_nbuf_ll_src(buf);
		ll_addr->addr = mpdu.mhr.src_addr->plain.addr.ext_addr;
		ll_addr->len = IEEE802154_EXT_ADDR_LENGTH;
	}

	if (mpdu.mhr.fs->fc.dst_addr_mode != IEEE802154_ADDR_MODE_NONE) {
		ll_addr = net_nbuf_ll_dst(buf);
		ll_addr->addr = mpdu.mhr.dst_addr->plain.addr.ext_addr;
		ll_addr->len = IEEE802154_EXT_ADDR_LENGTH;
	}

	return NET_CONTINUE;
}

static enum net_verdict ieee802154_send(struct net_if *iface,
					struct net_buf *buf)
{
	if (net_nbuf_family(buf) != AF_INET6 ||
	    !ieee802154_create_data_frame(iface, buf)) {
		return NET_DROP;
	}

	net_if_queue_tx(iface, buf);

	return NET_OK;
}

static uint16_t ieeee802154_reserve(struct net_if *iface, void *data)
{
	return ieee802154_compute_header_size(iface, (struct in6_addr *)data);
}

NET_L2_INIT(IEEE802154_L2,
	    ieee802154_recv, ieee802154_send, ieeee802154_reserve);

void ieee802154_init(struct net_if *iface)
{
	struct ieee802154_radio_api *radio =
		(struct ieee802154_radio_api *)iface->dev->driver_api;

	NET_DBG("Initializing IEEE 802.15.4 stack on iface %p", iface);

	radio->start(iface->dev);
}
