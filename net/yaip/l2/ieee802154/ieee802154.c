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

#include <errno.h>

#ifdef CONFIG_NET_6LO
#include <6lo.h>
#endif /* CONFIG_NET_6LO */

#include <net/ieee802154_radio.h>

#include "ieee802154_frame.h"

#if 0

#include <stdio.h>

static void pkt_hexdump(uint8_t *pkt, uint8_t length)
{
	int i;

	printf("IEEE 802.15.4 payload content:\n");

	for (i = 0; i < length;) {
		int j;

		printf("\t");

		for (j = 0; j < 10 && i < length; j++, i++) {
			printf("%02x ", *pkt);
			pkt++;
		}

		printf("\n");
	}
}
#else
#define pkt_hexdump(...)
#endif

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
static inline void ieee802154_acknowledge(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu)
{
	struct net_buf *buf, *frag;

	if (!mpdu->mhr.fs->fc.ar) {
		return;
	}

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return;
	}

	frag = net_nbuf_get_reserve_data(IEEE802154_ACK_PKT_LENGTH);

	net_buf_frag_insert(buf, frag);
	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));

	if (ieee802154_create_ack_frame(iface, buf, mpdu->mhr.fs->sequence)) {
		struct ieee802154_radio_api *radio =
			(struct ieee802154_radio_api *)iface->dev->driver_api;

		net_buf_add(frag, IEEE802154_ACK_PKT_LENGTH);

		radio->tx(iface->dev, buf);
	}

	net_nbuf_unref(buf);

	return;
}
#else
#define ieee802154_acknowledge(...)
#endif /* CONFIG_NET_L2_IEEE802154_ACK_REPLY */

static inline void set_buf_ll_addr(struct net_linkaddr *addr, bool comp,
				   enum ieee802154_addressing_mode mode,
				   struct ieee802154_address_field *ll)
{
	if (mode == IEEE802154_ADDR_MODE_NONE) {
		return;
	}

	if (mode == IEEE802154_ADDR_MODE_EXTENDED) {
		addr->len = IEEE802154_EXT_ADDR_LENGTH;

		if (comp) {
			addr->addr = ll->comp.addr.ext_addr;
		} else {
			addr->addr = ll->plain.addr.ext_addr;
		}
	} else {
		/* ToDo: Handle short address (lookup known nbr, ...) */
		addr->len = 0;
		addr->addr = NULL;
	}
}

static enum net_verdict ieee802154_recv(struct net_if *iface,
					struct net_buf *buf)
{
	struct ieee802154_mpdu mpdu;
#ifdef CONFIG_NET_6LO
	uint32_t src;
	uint32_t dst;
#endif /* CONFIG_NET_6LO */

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

	ieee802154_acknowledge(iface, &mpdu);

	/**
	 * We now remove the size of the MFR from the total length
	 * so net_core will not count it as IP data.
	 */
	buf->frags->len -= IEEE802154_MFR_LENGTH;
	net_nbuf_set_ll_reserve(buf, mpdu.payload - (void *)net_nbuf_ll(buf));
	net_buf_pull(buf->frags, net_nbuf_ll_reserve(buf));

	set_buf_ll_addr(net_nbuf_ll_src(buf), mpdu.mhr.fs->fc.pan_id_comp,
			mpdu.mhr.fs->fc.src_addr_mode, mpdu.mhr.src_addr);

	set_buf_ll_addr(net_nbuf_ll_dst(buf), false,
			mpdu.mhr.fs->fc.dst_addr_mode, mpdu.mhr.dst_addr);

	pkt_hexdump(net_nbuf_ip_data(buf), net_buf_frags_len(buf));

#ifdef CONFIG_NET_6LO
	/** Uncompress will drop the current fragment. Buf ll src/dst address
	 * will then be wrong and must be updated according to the new fragment.
	 */
	src = net_nbuf_ll_src(buf)->addr ?
		net_nbuf_ll_src(buf)->addr - net_nbuf_ll(buf) : 0;
	dst = net_nbuf_ll_dst(buf)->addr ?
		net_nbuf_ll_dst(buf)->addr - net_nbuf_ll(buf) : 0;

	if (!net_6lo_uncompress(buf)) {
		NET_DBG("Packet decompression failed");
		return NET_DROP;
	}

	net_nbuf_ll_src(buf)->addr = src ? net_nbuf_ll(buf) + src : NULL;
	net_nbuf_ll_dst(buf)->addr = dst ? net_nbuf_ll(buf) + dst : NULL;

	pkt_hexdump(net_nbuf_ip_data(buf), net_buf_frags_len(buf));
#endif /* CONFIG_NET_6LO */

	return NET_CONTINUE;
}

static enum net_verdict ieee802154_send(struct net_if *iface,
					struct net_buf *buf)
{
	if (net_nbuf_family(buf) != AF_INET6 ||
	    !ieee802154_create_data_frame(iface, buf)) {
		return NET_DROP;
	}

	pkt_hexdump(net_nbuf_ip_data(buf), net_buf_frags_len(buf));

#ifdef CONFIG_NET_6LO
	if (!net_6lo_compress(buf, true, NULL)) {
		return NET_DROP;
	}

	pkt_hexdump(net_nbuf_ip_data(buf), net_buf_frags_len(buf));
#endif /* CONFIG_NET_6LO */

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

#ifdef CONFIG_NET_L2_IEEE802154_ORFD
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	const uint8_t *mac = iface->link_addr.addr;
	uint16_t short_addr;

	short_addr = (mac[0] << 8) + mac[1];

	radio->set_short_addr(iface->dev, short_addr);
	radio->set_ieee_addr(iface->dev, mac);

	ctx->pan_id = CONFIG_NET_L2_IEEE802154_ORFD_PAN_ID;
	ctx->channel = CONFIG_NET_L2_IEEE802154_ORFD_CHANNEL;

	radio->set_pan_id(iface->dev, ctx->pan_id);
	radio->set_channel(iface->dev, ctx->channel);
#endif
	radio->start(iface->dev);
}
