/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_uart_pipe
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

#include <console/uart_pipe.h>
#include <net/ieee802154_radio.h>

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

static u8_t dev_pan_id[PAN_ID_SIZE];             /* Device Pan Id */
static u8_t dev_short_addr[SHORT_ADDRESS_SIZE];  /* Device Short Address */
static u8_t dev_ext_addr[EXTENDED_ADDRESS_SIZE]; /* Device Extended Address */

/** Singleton device used in uart pipe callback */
static struct device *upipe_dev;

#if defined(CONFIG_IEEE802154_UPIPE_HW_FILTER)

static bool received_dest_addr_matched(u8_t *rx_buffer)
{
	struct upipe_context *upipe = upipe_dev->driver_data;

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

static u8_t *upipe_rx(u8_t *buf, size_t *off)
{
	struct net_pkt *pkt = NULL;
	struct upipe_context *upipe;

	if (!upipe_dev) {
		goto done;
	}

	upipe = upipe_dev->driver_data;
	if (!upipe->rx && *buf == UART_PIPE_RADIO_15_4_FRAME_TYPE) {
		upipe->rx = true;
		goto done;
	}

	if (!upipe->rx_len) {
		if (*buf > 127) {
			goto flush;
		}

		upipe->rx_len = *buf;
		goto done;
	}

	upipe->rx_buf[upipe->rx_off++] = *buf;

	if (upipe->rx_len == upipe->rx_off) {
		struct net_buf *frag;

		pkt = net_pkt_rx_alloc(K_NO_WAIT);
		if (!pkt) {
			LOG_DBG("No pkt available");
			goto flush;
		}

		frag = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!frag) {
			LOG_DBG("No fragment available");
			goto out;
		}

		net_pkt_frag_insert(pkt, frag);

		memcpy(frag->data, upipe->rx_buf, upipe->rx_len);
		net_buf_add(frag, upipe->rx_len);

#if defined(CONFIG_IEEE802154_UPIPE_HW_FILTER)
		if (received_dest_addr_matched(frag->data) == false) {
			LOG_DBG("Packet received is not addressed to me");
			goto out;
		}
#endif

		if (ieee802154_radio_handle_ack(upipe->iface, pkt) == NET_OK) {
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

static enum ieee802154_hw_caps upipe_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS |
		IEEE802154_HW_2_4_GHZ |
		IEEE802154_HW_FILTER;
}

static int upipe_cca(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_channel(struct device *dev, u16_t channel)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	return 0;
}

static int upipe_set_pan_id(struct device *dev, u16_t pan_id)
{
	u8_t pan_id_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(pan_id, pan_id_le);
	memcpy(dev_pan_id, pan_id_le, PAN_ID_SIZE);

	return 0;
}

static int upipe_set_short_addr(struct device *dev, u16_t short_addr)
{
	u8_t short_addr_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(short_addr, short_addr_le);
	memcpy(dev_short_addr, short_addr_le, SHORT_ADDRESS_SIZE);

	return 0;
}

static int upipe_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	ARG_UNUSED(dev);

	memcpy(dev_ext_addr, ieee_addr, EXTENDED_ADDRESS_SIZE);

	return 0;
}

static int upipe_filter(struct device *dev,
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

static int upipe_set_txpower(struct device *dev, s16_t dbm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dbm);

	return 0;
}

static int upipe_tx(struct device *dev,
		    struct net_pkt *pkt,
		    struct net_buf *frag)
{
	struct upipe_context *upipe = dev->driver_data;
	u8_t *pkt_buf = frag->data;
	u8_t len = frag->len;
	u8_t i, data;

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

static int upipe_start(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	if (!upipe->stopped) {
		return -EALREADY;
	}

	upipe->stopped = false;

	return 0;
}

static int upipe_stop(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EALREADY;
	}

	upipe->stopped = true;

	return 0;
}

static int upipe_init(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	(void)memset(upipe, 0, sizeof(struct upipe_context));

	uart_pipe_register(upipe->uart_pipe_buf, 1, upipe_rx);

	upipe_stop(dev);

	return 0;
}

static inline u8_t *get_mac(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	upipe->mac_addr[0] = 0x00;
	upipe->mac_addr[1] = 0x10;
	upipe->mac_addr[2] = 0x20;
	upipe->mac_addr[3] = 0x30;

#if defined(CONFIG_IEEE802154_UPIPE_RANDOM_MAC)
	UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
		      (u32_t *) ((u8_t *)upipe->mac_addr+4));
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
	struct device *dev = net_if_get_device(iface);
	struct upipe_context *upipe = dev->driver_data;
	u8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	upipe_dev = dev;
	upipe->iface = iface;

	ieee802154_init(iface);
}

static struct upipe_context upipe_context_data;

static struct ieee802154_radio_api upipe_radio_api = {
	.iface_api.init		= upipe_iface_init,

	.get_capabilities	= upipe_get_capabilities,
	.cca			= upipe_cca,
	.set_channel		= upipe_set_channel,
	.filter			= upipe_filter,
	.set_txpower		= upipe_set_txpower,
	.tx			= upipe_tx,
	.start			= upipe_start,
	.stop			= upipe_stop,
};

NET_DEVICE_INIT(upipe_15_4, CONFIG_IEEE802154_UPIPE_DRV_NAME,
		upipe_init, &upipe_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&upipe_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
