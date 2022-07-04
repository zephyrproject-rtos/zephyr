/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Dummy IEEE 802.15.4 interface driver. This is meant for network connectivity between
 * a host and an RCP radio.
 */

#define LOG_MODULE_NAME ot_spinel
#define LOG_LEVEL CONFIG_NET_OPENTHREAD_HOST_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stddef.h>
#include <openthread/platform/radio.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio.h>

struct ot_spinel_context {
	struct net_if *iface;
	struct openthread_context *ot_context;
};

static void ot_spinel_iface_init(struct net_if *iface)
{
	struct ot_spinel_context *ctx = net_if_get_device(iface)->data;
	otExtAddress eui64;

	ctx->iface = iface;

	ieee802154_init(iface);

	ctx->ot_context = net_if_l2_data(iface);

	otPlatRadioGetIeeeEui64(ctx->ot_context->instance, eui64.m8);
	net_if_set_link_addr(iface, eui64.m8, OT_EXT_ADDRESS_SIZE, NET_LINK_IEEE802154);
}

static enum ieee802154_hw_caps ot_spinel_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int ot_spinel_cca(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int ot_spinel_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	return 0;
}

static int ot_spinel_filter(const struct device *dev,
			    bool set,
			    enum ieee802154_filter_type type,
			    const struct ieee802154_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(set);
	ARG_UNUSED(type);
	ARG_UNUSED(filter);

	return -ENOTSUP;
}

static int ot_spinel_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dbm);

	return 0;
}

static int ot_spinel_tx(const struct device *dev,
		     enum ieee802154_tx_mode mode,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mode);
	ARG_UNUSED(pkt);
	ARG_UNUSED(frag);

	return 0;
}

static int ot_spinel_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int ot_spinel_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int ot_spinel_configure(const struct device *dev,
			       enum ieee802154_config_type type,
			       const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(config);

	return 0;
}

static int ot_spinel_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static struct ot_spinel_context ot_spinel_context_data;

static struct ieee802154_radio_api ot_spinel_if_api = {
	.iface_api.init   = ot_spinel_iface_init,

	.get_capabilities = ot_spinel_get_capabilities,
	.cca              = ot_spinel_cca,
	.set_channel      = ot_spinel_set_channel,
	.filter           = ot_spinel_filter,
	.set_txpower      = ot_spinel_set_txpower,
	.tx               = ot_spinel_tx,
	.start            = ot_spinel_start,
	.stop             = ot_spinel_stop,
	.configure        = ot_spinel_configure,
};

#define ot_spinel_MTU 1280

NET_DEVICE_INIT(ot_spinel, CONFIG_IEEE802154_OPENTHREAD_HOST_SPINEL_DRV_NAME,
		ot_spinel_init, NULL, &ot_spinel_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ot_spinel_if_api,
		OPENTHREAD_L2, NET_L2_GET_CTX_TYPE(OPENTHREAD_L2),
		ot_spinel_MTU);
