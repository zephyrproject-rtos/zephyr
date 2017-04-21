/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <net/net_core.h>
#define NET_LOG_ENABLED 1
#define NET_SYS_LOG_LEVEL 4
#include "net_private.h"

#include <net/net_pkt.h>

/** FAKE ieee802.15.4 driver **/
#include <net/ieee802154_radio.h>

extern struct net_pkt *current_pkt;
extern struct k_sem driver_lock;

static int fake_cca(struct device *dev)
{
	return 0;
}

static int fake_set_channel(struct device *dev, u16_t channel)
{
	NET_INFO("Channel %u\n", channel);

	return 0;
}

static int fake_set_pan_id(struct device *dev, u16_t pan_id)
{
	NET_INFO("PAN id 0x%x\n", pan_id);

	return 0;
}

static int fake_set_short_addr(struct device *dev, u16_t short_addr)
{
	NET_INFO("Short address: 0x%x\n", short_addr);

	return 0;
}

static int fake_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	NET_INFO("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
		 ieee_addr[0], ieee_addr[1], ieee_addr[2], ieee_addr[3],
		 ieee_addr[4], ieee_addr[5], ieee_addr[6], ieee_addr[7]);

	return 0;
}

static int fake_set_txpower(struct device *dev, s16_t dbm)
{
	NET_INFO("TX power %d dbm\n", dbm);

	return 0;
}

static inline void insert_frag_dummy_way(struct net_pkt *pkt)
{
	if (current_pkt->frags) {
		struct net_buf *frag, *prev_frag = NULL;

		frag = current_pkt->frags;
		while (frag) {
			prev_frag = frag;

			frag = frag->frags;
		}

		prev_frag->frags = net_buf_ref(pkt->frags);
	} else {
		current_pkt->frags = net_buf_ref(pkt->frags);
	}
}

static int fake_tx(struct device *dev,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	NET_INFO("Sending packet %p - length %zu\n",
		 pkt, net_pkt_get_len(pkt));

	net_pkt_set_ll_reserve(current_pkt, net_pkt_ll_reserve(pkt));

	insert_frag_dummy_way(pkt);

	k_sem_give(&driver_lock);

	return 0;
}

static int fake_start(struct device *dev)
{
	NET_INFO("FAKE ieee802154 driver started\n");

	return 0;
}

static int fake_stop(struct device *dev)
{
	NET_INFO("FAKE ieee802154 driver stopped\n");

	return 0;
}

static void fake_iface_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	static u8_t mac[8] = { 0x00, 0x12, 0x4b, 0x00,
				  0x00, 0x9e, 0xa3, 0xc2 };

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	ctx->pan_id = 0xabcd;
	ctx->channel = 26;
	ctx->sequence = 62;

	NET_INFO("FAKE ieee802154 iface initialized\n");
}

static int fake_init(struct device *dev)
{
	fake_stop(dev);

	return 0;
}

static struct ieee802154_radio_api fake_radio_api = {
	.iface_api.init	= fake_iface_init,
	.iface_api.send	= ieee802154_radio_send,

	.cca		= fake_cca,
	.set_channel	= fake_set_channel,
	.set_pan_id	= fake_set_pan_id,
	.set_short_addr	= fake_set_short_addr,
	.set_ieee_addr	= fake_set_ieee_addr,
	.set_txpower	= fake_set_txpower,
	.start		= fake_start,
	.stop		= fake_stop,
	.tx		= fake_tx,
};

NET_DEVICE_INIT(fake, "fake_ieee802154",
		fake_init, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
