/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ieee802154_uart_pipe

#define LOG_MODULE_NAME ieee802154_uart_pipe
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>

#include <zephyr/drivers/uart_pipe.h>
#include <zephyr/net/ieee802154_radio.h>

#include "ieee802154_uart_pipe.h"

#define PAN_ID_OFFSET           3 /* Pan Id offset */
#define DEST_ADDR_OFFSET        5 /* Destination offset address*/
#define DEST_ADDR_TYPE_OFFSET   1 /* Destination address type */

#define DEST_ADDR_TYPE_MASK     0x0c /* Mask for destination address type */

#define DEST_ADDR_TYPE_SHORT    0x08 /* Short destination address type */
#define DEST_ADDR_TYPE_EXTENDED 0x0c /* Extended destination address type */

#define PAN_ID_SIZE           2    /* Size of Pan Id */
#define SHORT_ADDRESS_SIZE    2    /* Size of Short Mac Address */
#define EXTENDED_ADDRESS_SIZE 8    /* Size of Extended Mac Address */

/* Broadcast Short Address */
#define BROADCAST_ADDRESS    ((uint8_t [SHORT_ADDRESS_SIZE]) {0xff, 0xff})

static uint8_t dev_pan_id[PAN_ID_SIZE];             /* Device Pan Id */
static uint8_t dev_short_addr[SHORT_ADDRESS_SIZE];  /* Device Short Address */
static uint8_t dev_ext_addr[EXTENDED_ADDRESS_SIZE]; /* Device Extended Address */

/** Singleton device used in uart pipe callback */
static const struct device *upipe_dev;

#if defined(CONFIG_IEEE802154_UPIPE_HW_FILTER)

static bool received_dest_addr_matched(uint8_t *rx_buffer)
{
	struct upipe_context *upipe = upipe_dev->data;

	/* Check destination PAN Id */
	if (memcmp(&rx_buffer[PAN_ID_OFFSET],
		   dev_pan_id, PAN_ID_SIZE) != 0 &&
	    memcmp(&rx_buffer[PAN_ID_OFFSET],
		   BROADCAST_ADDRESS, PAN_ID_SIZE) != 0) {
		return false;
	}

	/* Check destination address */
	switch (rx_buffer[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK) {
	case DEST_ADDR_TYPE_SHORT:
		/* First check if the destination is broadcast */
		/* If not broadcast, check if length and address matches */
		if (memcmp(&rx_buffer[DEST_ADDR_OFFSET],
			   BROADCAST_ADDRESS,
			   SHORT_ADDRESS_SIZE) != 0 &&
		    (net_if_get_link_addr(upipe->iface)->len !=
		     SHORT_ADDRESS_SIZE ||
		     memcmp(&rx_buffer[DEST_ADDR_OFFSET],
			    dev_short_addr,
			    SHORT_ADDRESS_SIZE) != 0)) {
			return false;
		}
	break;

	case DEST_ADDR_TYPE_EXTENDED:
		/* If not broadcast, check if length and address matches */
		if (net_if_get_link_addr(upipe->iface)->len !=
		    EXTENDED_ADDRESS_SIZE ||
		    memcmp(&rx_buffer[DEST_ADDR_OFFSET],
			   dev_ext_addr, EXTENDED_ADDRESS_SIZE) != 0) {
			return false;
		}
		break;

	default:
		return false;
	}

	return true;
}

#endif

static uint8_t *upipe_rx(uint8_t *buf, size_t *off)
{
	struct net_pkt *pkt = NULL;
	struct upipe_context *upipe;

	if (!upipe_dev) {
		goto done;
	}

	upipe = upipe_dev->data;
	if (!upipe->rx && *buf == UART_PIPE_RADIO_15_4_FRAME_TYPE) {
		upipe->rx = true;
		goto done;
	}

	if (!upipe->rx_len) {
		if (*buf > IEEE802154_MAX_PHY_PACKET_SIZE) {
			goto flush;
		}

		upipe->rx_len = *buf;
		goto done;
	}

	upipe->rx_buf[upipe->rx_off++] = *buf;

	if (upipe->rx_len == upipe->rx_off) {
		pkt = net_pkt_rx_alloc_with_buffer(upipe->iface, upipe->rx_len,
						   AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_DBG("No pkt available");
			goto flush;
		}

		if (net_pkt_write(pkt, upipe->rx_buf, upipe->rx_len)) {
			LOG_DBG("No content read?");
			goto out;
		}

#if defined(CONFIG_IEEE802154_UPIPE_HW_FILTER)
		if (received_dest_addr_matched(pkt->buffer->data) == false) {
			LOG_DBG("Packet received is not addressed to me");
			goto out;
		}
#endif

		if (ieee802154_handle_ack(upipe->iface, pkt) == NET_OK) {
			LOG_DBG("ACK packet handled");
			goto out;
		}

		LOG_DBG("Caught a packet (%u)", upipe->rx_len);
		if (net_recv_data(upipe->iface, pkt) < 0) {
			LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		goto flush;
out:
		net_pkt_unref(pkt);
flush:
		upipe->rx = false;
		upipe->rx_len = 0U;
		upipe->rx_off = 0U;
	}
done:
	*off = 0;

	return buf;
}

static enum ieee802154_hw_caps upipe_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER;
}

static int upipe_cca(const struct device *dev)
{
	struct upipe_context *upipe = dev->data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	return 0;
}

static int upipe_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	uint8_t pan_id_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(pan_id, pan_id_le);
	memcpy(dev_pan_id, pan_id_le, PAN_ID_SIZE);

	return 0;
}

static int upipe_set_short_addr(const struct device *dev, uint16_t short_addr)
{
	uint8_t short_addr_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(short_addr, short_addr_le);
	memcpy(dev_short_addr, short_addr_le, SHORT_ADDRESS_SIZE);

	return 0;
}

static int upipe_set_ieee_addr(const struct device *dev,
			       const uint8_t *ieee_addr)
{
	ARG_UNUSED(dev);

	memcpy(dev_ext_addr, ieee_addr, EXTENDED_ADDRESS_SIZE);

	return 0;
}

static int upipe_filter(const struct device *dev,
			bool set,
			enum ieee802154_filter_type type,
			const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return upipe_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return upipe_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return upipe_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

static int upipe_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dbm);

	return 0;
}

static int upipe_tx(const struct device *dev,
		    enum ieee802154_tx_mode mode,
		    struct net_pkt *pkt,
		    struct net_buf *frag)
{
	struct upipe_context *upipe = dev->data;
	uint8_t *pkt_buf = frag->data;
	uint8_t len = frag->len;
	uint8_t i, data;

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	LOG_DBG("%p (%u)", frag, len);

	if (upipe->stopped) {
		return -EIO;
	}

	data = UART_PIPE_RADIO_15_4_FRAME_TYPE;
	uart_pipe_send(&data, 1);

	data = len;
	uart_pipe_send(&data, 1);

	for (i = 0U; i < len; i++) {
		uart_pipe_send(pkt_buf+i, 1);
	}

	return 0;
}

static int upipe_start(const struct device *dev)
{
	struct upipe_context *upipe = dev->data;

	if (!upipe->stopped) {
		return -EALREADY;
	}

	upipe->stopped = false;

	return 0;
}

static int upipe_stop(const struct device *dev)
{
	struct upipe_context *upipe = dev->data;

	if (upipe->stopped) {
		return -EALREADY;
	}

	upipe->stopped = true;

	return 0;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

/* API implementation: attr_get */
static int upipe_attr_get(const struct device *dev, enum ieee802154_attr attr,
			  struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

static int upipe_init(const struct device *dev)
{
	struct upipe_context *upipe = dev->data;

	(void)memset(upipe, 0, sizeof(struct upipe_context));

	uart_pipe_register(upipe->uart_pipe_buf, 1, upipe_rx);

	upipe_stop(dev);

	return 0;
}

static inline uint8_t *get_mac(const struct device *dev)
{
	struct upipe_context *upipe = dev->data;

	upipe->mac_addr[0] = 0x00;
	upipe->mac_addr[1] = 0x10;
	upipe->mac_addr[2] = 0x20;
	upipe->mac_addr[3] = 0x30;

#if defined(CONFIG_IEEE802154_UPIPE_RANDOM_MAC)
	sys_rand_get(&upipe->mac_addr[4], 4U);
#else
	upipe->mac_addr[4] = CONFIG_IEEE802154_UPIPE_MAC4;
	upipe->mac_addr[5] = CONFIG_IEEE802154_UPIPE_MAC5;
	upipe->mac_addr[6] = CONFIG_IEEE802154_UPIPE_MAC6;
	upipe->mac_addr[7] = CONFIG_IEEE802154_UPIPE_MAC7;
#endif

	return upipe->mac_addr;
}

static void upipe_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct upipe_context *upipe = dev->data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	upipe_dev = dev;
	upipe->iface = iface;

	ieee802154_init(iface);
}

static struct upipe_context upipe_context_data;

static const struct ieee802154_radio_api upipe_radio_api = {
	.iface_api.init		= upipe_iface_init,

	.get_capabilities	= upipe_get_capabilities,
	.cca			= upipe_cca,
	.set_channel		= upipe_set_channel,
	.filter			= upipe_filter,
	.set_txpower		= upipe_set_txpower,
	.tx			= upipe_tx,
	.start			= upipe_start,
	.stop			= upipe_stop,
	.attr_get		= upipe_attr_get,
};

NET_DEVICE_DT_INST_DEFINE(0, upipe_init, NULL, &upipe_context_data, NULL,
			  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &upipe_radio_api,
			  IEEE802154_L2, NET_L2_GET_CTX_TYPE(IEEE802154_L2),
			  125);
