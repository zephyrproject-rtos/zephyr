/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "net/ieee802154/upipe/"
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <console/uart_pipe.h>
#include <net/ieee802154_radio.h>

#include "ieee802154_uart_pipe.h"

/** Singleton device used in uart pipe callback */
static struct device *upipe_dev;

static u8_t *upipe_rx(u8_t *buf, size_t *off)
{
	struct upipe_context *upipe = upipe_dev->driver_data;
	struct net_pkt *pkt = NULL;
	struct net_buf *frag = NULL;

	if (!upipe_dev) {
		goto done;
	}

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
		pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt) {
			SYS_LOG_DBG("No pkt available");
			goto flush;
		}

		frag = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!frag) {
			SYS_LOG_DBG("No fragment available");
			goto out;
		}

		net_pkt_frag_insert(pkt, frag);

		memcpy(frag->data, upipe->rx_buf, upipe->rx_len);
		net_buf_add(frag, upipe->rx_len);

		if (ieee802154_radio_handle_ack(upipe->iface, pkt) == NET_OK) {
			SYS_LOG_DBG("ACK packet handled");
			goto out;
		}

		SYS_LOG_DBG("Caught a packet (%u)", upipe->rx_len);
		if (net_recv_data(upipe->iface, pkt) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		goto flush;
out:
		net_pkt_unref(pkt);
flush:
		upipe->rx = false;
		upipe->rx_len = 0;
		upipe->rx_off = 0;
	}
done:
	*off = 0;

	return buf;
}

static enum ieee802154_hw_caps upipe_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_2_4_GHZ;
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
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_txpower(struct device *dev, s16_t dbm)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_tx(struct device *dev,
		    struct net_pkt *pkt,
		    struct net_buf *frag)
{
	u8_t *pkt_buf = frag->data - net_pkt_ll_reserve(pkt);
	u8_t len = net_pkt_ll_reserve(pkt) + frag->len;
	struct upipe_context *upipe = dev->driver_data;
	u8_t i, data;

	SYS_LOG_DBG("%p (%u)", frag, len);

	if (upipe->stopped) {
		return -EIO;
	}

	data = UART_PIPE_RADIO_15_4_FRAME_TYPE;
	uart_pipe_send(&data, 1);

	data = len;
	uart_pipe_send(&data, 1);

	for (i = 0; i < len; i++) {
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

	memset(upipe, 0, sizeof(struct upipe_context));

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

	UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
		      (u32_t *) ((void *)upipe->mac_addr+4));

	return upipe->mac_addr;
}

static void upipe_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct upipe_context *upipe = dev->driver_data;
	u8_t *mac = get_mac(dev);

	SYS_LOG_DBG("");

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	upipe_dev = dev;
	upipe->iface = iface;

	ieee802154_init(iface);
}

static struct upipe_context upipe_context_data;

static struct ieee802154_radio_api upipe_radio_api = {
	.iface_api.init		= upipe_iface_init,
	.iface_api.send		= ieee802154_radio_send,

	.get_capabilities	= upipe_get_capabilities,
	.cca			= upipe_cca,
	.set_channel		= upipe_set_channel,
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
