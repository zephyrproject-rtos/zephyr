/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc1101

#define LOG_MODULE_NAME ieee802154_cc1101
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <sys/byteorder.h>
#include <string.h>
#include <random/rand32.h>

#include <drivers/rf.h.h>
#include <net/ieee802154_radio.h>

#include "ieee802154_cc1101.h"



static inline uint8_t *get_mac(const struct device *dev)
{
	struct cc1101_context *cc1101 = dev->data;

#if defined(CONFIG_IEEE802154_CC1200_RANDOM_MAC)
	uint32_t *ptr = (uint32_t *)(cc1101->mac_addr + 4);

	UNALIGNED_PUT(sys_rand32_get(), ptr);

	cc1100->mac_addr[7] = (cc1200->mac_addr[7] & ~0x01) | 0x02;
#else
	cc1101->mac_addr[4] = CONFIG_IEEE802154_CC1101_MAC4;
	cc1101->mac_addr[5] = CONFIG_IEEE802154_CC1101_MAC5;
	cc1101->mac_addr[6] = CONFIG_IEEE802154_CC1101_MAC6;
	cc1101->mac_addr[7] = CONFIG_IEEE802154_CC1101_MAC7;
#endif

	cc1101->mac_addr[0] = 0x00;
	cc1101->mac_addr[1] = 0x12;
	cc1101->mac_addr[2] = 0x4b;
	cc1101->mac_addr[3] = 0x00;

	return cc1101->mac_addr;
}

static int cc1101_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_SUB_GHZ;
}

static int cc1101_cca(const struct device *dev)
{
	int ret;

	ret = rf_device_get(dev, RF_DEVICE_CCA, NULL);

	return ret;
}

static int cc1101_set_channel(const struct device *dev, uint16_t channel)
{
	int ret;

	ret = rf_device_set(dev, RF_DEVICE_CHANNEL, (void *)&channel);

	return ret;
}

static int cc1101_set_txpower(const struct device *dev, int16_t dbm)
{
	int ret;

	ret = rf_device_set(dev, RF_OUTPUT_POWER, (void *)&dbm);

	return ret;
}

static int cc1101_tx(const struct device *dev,
		     enum ieee802154_tx_mode mode,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	int ret;

	ret = rf_send(dev, ...);

	return ret;
}

static int cc1101_start(const struct device *dev)
{
	int ret;

	ret = rf_device_set(dev, RF_OPERATING_MODE, RF_MODE_RX);

	return ret;
}

static int cc1101_stop(const struct device *dev)
{
	int ret;

	ret = rf_device_set(dev, RF_OPERATING_MODE, RF_MODE_SLEEP);

	return ret;
}


/**
 * when using k_event objects
 */
static void cc1101_rx_thread(const struct device *dev)
{
	int ret;
	union rf_packet pkt;
	struct cc1101_context *data = dev->data;

	while (1){
		k_event_wait(data->events, RF_EVENT_RECV_DONE, true, K_FOREVER);

		ret = rf_recv(data->rf_dev, &pkt);
		if(ret){
			return;
		}

		// send pkt up the stack
	}
}

/**
 * when using event callback cc1101_rf_event_cb below
 */
static void cc1101_rx(const struct device *rf_dev)
{
	struct cc1101_context *ctx =
		CONTAINER_OF(rf_dev, struct cc1101_ctx, rf_dev);
	int ret;
	union rf_packet pkt;

	ret = rf_recv(rf_dev, &pkt);
	if(ret){
		return;
	}

	// send pkt up the stack
}

static void cc1101_rf_event_cb(const struct device *rf_dev, enum rf_event event, void *event_params)
{
        switch(event)
        {
        case RF_EVENT_STUCK:
                LOG_DBG("Stuck!\n");
                break;
        case RF_EVENT_RECV_READY:
                LOG_DBG("Recv ready!\n");
                break;
        case RF_EVENT_SEND_READY:
                LOG_DBG("Send ready!\n");
                break;
        case RF_EVENT_RECV_DONE:
		/* send packet up the stack */
		cc1101_rx(rf_dev);
                break;
        case RF_EVENT_SEND_DONE:
                LOG_DBG("Send done!\n");
                break;
        case RF_EVENT_CHANNEL_CLEAR:
                LOG_DBG("Channel Clear!\n");
                break;
        case RF_EVENT_WAKEUP:
                LOG_DBG("WAKEUP!\n");
                break;
        default:
                LOG_DBG("UNKNOWN EVENT\n");
        }
}

static int cc1101_init(const struct device *dev)
{
	int ret;
	struct cc1101_context *ctx = dev->data;

	/**
	 * Is there a nicer way to do this?
	 */
	ctx->rf_dev = device_get_binding(CONFIG_NET_CONFIG_IEEE802154_DEV_NAME);

	ret = rf_device_set(rf_dev, RF_DEVICE_SET_EVENT_CB, cc1101_rf_event_cb);

	return ret;
}

static void cc1101_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cc1101_context *data = dev->data;
	uint8_t *mac = get_mac(dev);


	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	cc1101->iface = iface;

	ieee802154_init(iface);
}

static struct cc1101_context cc1101_context_data;

static struct ieee802154_radio_api cc1101_radio_api = {
	.iface_api.init	= cc1101_iface_init,

	.get_capabilities	= cc1101_get_capabilities,
	.cca			= cc1101_cca,
	.set_channel		= cc1101_set_channel,
	.set_txpower		= cc1101_set_txpower,
	.tx			= cc1101_tx,
	.start			= cc1101_start,
	.stop			= cc1101_stop,
};

NET_DEVICE_INIT(cc1101, CONFIG_IEEE802154_CC1101_DRV_NAME,
		cc1101_init, NULL,
		&cc1101_context_data, NULL,
		CONFIG_IEEE802154_CC1101_INIT_PRIO,
		&cc1101_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
