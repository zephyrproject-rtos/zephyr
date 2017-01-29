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
#include <net/nbuf.h>

#include <console/uart_pipe.h>
#include <net/ieee802154_radio.h>

#include "ieee802154_uart_pipe.h"

/** Singleton device used in uart pipe callback */
static struct device *upipe_dev;

static uint8_t *upipe_rx(uint8_t *buf, size_t *off)
{
	struct upipe_context *upipe = upipe_dev->driver_data;
	struct net_buf *pkt_buf = NULL;
	struct net_buf *nbuf = NULL;

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
		nbuf = net_nbuf_get_reserve_rx(0);
		if (!nbuf) {
			SYS_LOG_DBG("No buf available");
			goto flush;
		}

		pkt_buf = net_nbuf_get_reserve_data(0);
		if (!pkt_buf) {
			SYS_LOG_DBG("No fragment available");
			goto out;
		}

		net_buf_frag_insert(nbuf, pkt_buf);

		memcpy(pkt_buf->data, upipe->rx_buf, upipe->rx_len - 2);
		net_buf_add(pkt_buf, upipe->rx_len - 2);

		if (ieee802154_radio_handle_ack(upipe->iface, nbuf) == NET_OK) {
			SYS_LOG_DBG("ACK packet handled");
			goto out;
		}

		SYS_LOG_DBG("Caught a packet (%u)", upipe->rx_len);
		if (net_recv_data(upipe->iface, nbuf) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		goto flush;
out:
		net_buf_unref(nbuf);
flush:
		upipe->rx = false;
		upipe->rx_len = 0;
		upipe->rx_off = 0;
	}
done:
	*off = 0;

	return buf;
}

static int upipe_cca(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_channel(struct device *dev, uint16_t channel)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_pan_id(struct device *dev, uint16_t pan_id)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_short_addr(struct device *dev, uint16_t short_addr)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_ieee_addr(struct device *dev, const uint8_t *ieee_addr)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_set_txpower(struct device *dev, int16_t dbm)
{
	struct upipe_context *upipe = dev->driver_data;

	if (upipe->stopped) {
		return -EIO;
	}

	return 0;
}

static int upipe_tx(struct device *dev,
		    struct net_buf *buf,
		    struct net_buf *frag)
{
	uint8_t *pkt_buf = frag->data - net_nbuf_ll_reserve(buf);
	uint8_t len = net_nbuf_ll_reserve(buf) + frag->len;
	struct upipe_context *upipe = dev->driver_data;
	uint8_t i, data;

	SYS_LOG_DBG("%p (%u)", frag, len);

	if (upipe->stopped) {
		return -EIO;
	}

	data = UART_PIPE_RADIO_15_4_FRAME_TYPE;
	uart_pipe_send(&data, 1);

	data = len + 2;
	uart_pipe_send(&data, 1);

	for (i = 0; i < len; i++) {
		uart_pipe_send(pkt_buf+i, 1);
	}

	/* The 2 dummy bytes representing FCS */
	data = 0xFF;
	uart_pipe_send(&data, 1);
	uart_pipe_send(&data, 1);

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

static inline uint8_t *get_mac(struct device *dev)
{
	struct upipe_context *upipe = dev->driver_data;

	upipe->mac_addr[0] = 0x00;
	upipe->mac_addr[1] = 0x10;
	upipe->mac_addr[2] = 0x20;
	upipe->mac_addr[3] = 0x30;

	UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
		      (uint32_t *) ((void *)upipe->mac_addr+4));

	return upipe->mac_addr;
}

static void upipe_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct upipe_context *upipe = dev->driver_data;
	uint8_t *mac = get_mac(dev);

	SYS_LOG_DBG("");

	net_if_set_link_addr(iface, mac, 8);

	upipe_dev = dev;
	upipe->iface = iface;

	ieee802154_init(iface);
}

static struct upipe_context upipe_context_data;

static struct ieee802154_radio_api upipe_radio_api = {
	.iface_api.init		= upipe_iface_init,
	.iface_api.send		= ieee802154_radio_send,

	.cca			= upipe_cca,
	.set_channel		= upipe_set_channel,
	.set_pan_id		= upipe_set_pan_id,
	.set_short_addr		= upipe_set_short_addr,
	.set_ieee_addr		= upipe_set_ieee_addr,
	.set_txpower		= upipe_set_txpower,
	.tx			= upipe_tx,
	.start			= upipe_start,
	.stop			= upipe_stop,
};

NET_DEVICE_INIT(upipe_15_4, CONFIG_UPIPE_15_4_DRV_NAME,
		upipe_init, &upipe_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&upipe_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
