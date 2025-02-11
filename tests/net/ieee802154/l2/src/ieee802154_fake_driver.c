/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_fake_driver, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

/** FAKE ieee802.15.4 driver **/
#include <zephyr/net/ieee802154_radio.h>

#include "net_private.h"
#include <ieee802154_frame.h>

struct net_pkt *current_pkt;
K_SEM_DEFINE(driver_lock, 0, UINT_MAX);

uint8_t mock_ext_addr_be[8] = {0x00, 0x12, 0x4b, 0x00, 0x00, 0x9e, 0xa3, 0xc2};

static enum ieee802154_hw_caps fake_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS;
}

static int fake_cca(const struct device *dev)
{
	return 0;
}

static int fake_set_channel(const struct device *dev, uint16_t channel)
{
	NET_INFO("Channel %u", channel);

	return 0;
}

static int fake_set_txpower(const struct device *dev, int16_t dbm)
{
	NET_INFO("TX power %d dbm", dbm);

	return 0;
}

static inline void insert_frag(struct net_pkt *pkt, struct net_buf *frag)
{
	struct net_buf *new_frag;

	new_frag = net_pkt_get_frag(pkt, frag->len, K_SECONDS(1));
	if (!new_frag) {
		return;
	}

	memcpy(new_frag->data, frag->data, frag->len);
	net_buf_add(new_frag, frag->len);

	net_pkt_frag_add(current_pkt, new_frag);
}

static int fake_tx(const struct device *dev,
		   enum ieee802154_tx_mode mode,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	NET_INFO("Sending packet %p - length %zu",
		 pkt, net_pkt_get_len(pkt));

	if (!current_pkt) {
		return 0;
	}

	insert_frag(pkt, frag);

	if (ieee802154_is_ar_flag_set(frag)) {
		struct net_if *iface = net_if_lookup_by_dev(dev);
		struct ieee802154_context *ctx = net_if_l2_data(iface);

		struct net_pkt *ack_pkt;

		ack_pkt = net_pkt_rx_alloc_with_buffer(iface, IEEE802154_ACK_PKT_LENGTH, AF_UNSPEC,
						       0, K_FOREVER);
		if (!ack_pkt) {
			NET_ERR("*** Could not allocate ack pkt.");
			return -ENOMEM;
		}

		if (!ieee802154_create_ack_frame(iface, ack_pkt, ctx->ack_seq)) {
			NET_ERR("*** Could not create ack frame.");
			net_pkt_unref(ack_pkt);
			return -EFAULT;
		}

		ieee802154_handle_ack(iface, ack_pkt);
		net_pkt_unref(ack_pkt);
	}

	k_sem_give(&driver_lock);

	return 0;
}

static int fake_start(const struct device *dev)
{
	NET_INFO("FAKE ieee802154 driver started");

	return 0;
}

static int fake_stop(const struct device *dev)
{
	NET_INFO("FAKE ieee802154 driver stopped");

	return 0;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

/* API implementation: attr_get */
static int fake_attr_get(const struct device *dev, enum ieee802154_attr attr,
			 struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

static void fake_iface_init(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	net_if_set_link_addr(iface, mock_ext_addr_be, 8, NET_LINK_IEEE802154);

	ieee802154_init(iface);

	ctx->pan_id = IEEE802154_PAN_ID_NOT_ASSOCIATED;
	ctx->short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	ctx->channel = 26U;
	ctx->sequence = 62U;

	NET_INFO("FAKE ieee802154 iface initialized");
}

static int fake_init(const struct device *dev)
{
	fake_stop(dev);

	return 0;
}

static struct ieee802154_radio_api fake_radio_api = {
	.iface_api.init	= fake_iface_init,

	.get_capabilities	= fake_get_capabilities,
	.cca			= fake_cca,
	.set_channel		= fake_set_channel,
	.set_txpower		= fake_set_txpower,
	.start			= fake_start,
	.stop			= fake_stop,
	.tx			= fake_tx,
	.attr_get		= fake_attr_get,
};

NET_DEVICE_INIT(fake, "fake_ieee802154",
		fake_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), IEEE802154_MTU);
