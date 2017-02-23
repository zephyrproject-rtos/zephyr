/* ieee802154_nrf5.c - nRF5 802.15.4 driver */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "dev/nrf5_802154"
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/nbuf.h>

#include <misc/byteorder.h>
#include <string.h>
#include <rand32.h>

#include <net/ieee802154_radio.h>
#include <drivers/clock_control/nrf5_clock_control.h>
#include <clock_control.h>

#include "nrf52840.h"
#include "ieee802154_nrf5.h"
#include "nrf_drv_radio802154.h"

struct nrf5_802154_config {
	void (*irq_config_func)(struct device *dev);
};

static struct nrf5_802154_data nrf5_data;

/* Convenience defines for RADIO */
#define NRF5_802154_DATA(dev) \
	((struct nrf5_802154_data * const)(dev)->driver_data)

#define NRF5_802154_CFG(dev) \
	((struct nrf5_802154_config * const)(dev)->config->config_info)

static void nrf5_get_eui64(uint8_t *mac)
{
	memcpy(mac, (const uint32_t *)&NRF_FICR->DEVICEID, 8);
}

static void nrf5_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct device *dev = (struct device *)arg1;
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	struct net_buf *pkt_buf = NULL;
	enum net_verdict ack_result;
	struct net_buf *buf;
	uint8_t pkt_len;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		buf = NULL;

		SYS_LOG_DBG("Waiting for frame");
		k_sem_take(&nrf5_radio->rx_wait, K_FOREVER);

		SYS_LOG_DBG("Frame received");

		buf = net_nbuf_get_reserve_rx(0, K_NO_WAIT);
		if (!buf) {
			SYS_LOG_ERR("No buf available");
			goto out;
		}

		pkt_buf = net_nbuf_get_reserve_data(0, K_NO_WAIT);
		if (!pkt_buf) {
			SYS_LOG_ERR("No pkt_buf available");
			goto out;
		}

		net_buf_frag_insert(buf, pkt_buf);

		/* rx_mpdu contains length, psdu, [fcs], lqi
		 * FCS filed (2 bytes) is not present if CRC is enabled
		 */
		pkt_len = nrf5_radio->rx_psdu[0] -  NRF5_FCS_LENGTH;

		/* Skip length (first byte) and copy the payload */
		memcpy(pkt_buf->data, nrf5_radio->rx_psdu + 1, pkt_len);
		net_buf_add(pkt_buf, pkt_len);

		nrf_drv_radio802154_buffer_free(nrf5_radio->rx_psdu);

		ack_result = ieee802154_radio_handle_ack(nrf5_radio->iface,
							 buf);
		if (ack_result == NET_OK) {
			SYS_LOG_DBG("ACK packet handled");
			goto out;
		}

		SYS_LOG_DBG("Caught a packet (%u) (LQI: %u)",
			    pkt_len, nrf5_radio->lqi);

		if (net_recv_data(nrf5_radio->iface, buf) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		net_analyze_stack("nRF5 rx stack",
				  (unsigned char *)nrf5_radio->rx_stack,
				  CONFIG_IEEE802154_NRF5_RX_STACK_SIZE);
		continue;

out:
		if (buf) {
			net_buf_unref(buf);
		}
	}
}

/* Radio device API */

static int nrf5_cca(struct device *dev)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	/* Current implementation of the NRF5 radio driver doesn't provide an
	 * explicit API to perform CCA. However, Mode1 CCA (energy above
	 * threshold), can be achieved using energy detection function.
	 */
	if (!nrf_drv_radio802154_energy_detection(nrf5_radio->channel, 128)) {
		return -EBUSY;
	}

	/* The nRF driver guarantees that a callback will be called once
	 * the ED function is done, thus unlocking the semaphore.
	 */
	k_sem_take(&nrf5_radio->cca_wait, K_FOREVER);
	SYS_LOG_DBG("CCA: %d", nrf5_radio->channel_ed);

	if (nrf5_radio->channel_ed > CONFIG_IEEE802154_NRF5_CCA_ED_THRESHOLD) {
		return -EBUSY;
	}

	return 0;
}

static int nrf5_set_channel(struct device *dev, uint16_t channel)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	SYS_LOG_DBG("%u", channel);

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	if (!nrf_drv_radio802154_receive(channel, false)) {
		return -EBUSY;
	}

	nrf5_radio->channel = channel;
	return 0;
}

static int nrf5_set_pan_id(struct device *dev, uint16_t pan_id)
{
	uint8_t pan_id_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(pan_id, pan_id_le);
	nrf_drv_radio802154_pan_id_set(pan_id_le);

	SYS_LOG_DBG("0x%x", pan_id);
	return 0;
}

static int nrf5_set_short_addr(struct device *dev, uint16_t short_addr)
{
	uint8_t short_addr_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(short_addr, short_addr_le);
	nrf_drv_radio802154_short_address_set(short_addr_le);

	SYS_LOG_DBG("0x%x", short_addr);
	return 0;
}

static int nrf5_set_ieee_addr(struct device *dev, const uint8_t *ieee_addr)
{
	ARG_UNUSED(dev);

	SYS_LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	nrf_drv_radio802154_extended_address_set(ieee_addr);

	return 0;
}

static int nrf5_set_txpower(struct device *dev, int16_t dbm)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	SYS_LOG_DBG("%d", dbm);
	nrf5_radio->txpower = dbm;

	return 0;
}

static int nrf5_tx(struct device *dev,
		   struct net_buf *buf,
		   struct net_buf *frag)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	uint8_t payload_len = net_nbuf_ll_reserve(buf) + frag->len;
	uint8_t *payload = frag->data - net_nbuf_ll_reserve(buf);

	SYS_LOG_DBG("%p (%u)", payload, payload_len);

	nrf5_radio->tx_success = false;
	nrf5_radio->tx_psdu[0] = payload_len + NRF5_FCS_LENGTH;

	memcpy(nrf5_radio->tx_psdu + 1, payload, payload_len);

	if (!nrf_drv_radio802154_transmit(nrf5_radio->tx_psdu,
					  nrf5_radio->channel,
					  nrf5_radio->txpower)) {
		SYS_LOG_ERR("Cannot send frame");
		return -EIO;
	}

	SYS_LOG_DBG("Sending frame (ch:%d, txpower:%d)",
		    nrf5_radio->channel,
		    nrf5_radio->txpower);

	/* The nRF driver guarantees that either
	 * nrf_drv_radio802154_transmitted() or
	 * nrf_drv_radio802154_energy_detected()
	 * callback is called, thus unlocking the semaphore.
	 */
	k_sem_take(&nrf5_radio->tx_wait, K_FOREVER);

	SYS_LOG_DBG("Result: %d", nrf5_data.tx_success);

	return nrf5_radio->tx_success ? 0 : -EBUSY;
}

static int nrf5_start(struct device *dev)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	nrf_drv_radio802154_receive(nrf5_radio->channel, false);
	SYS_LOG_DBG("nRF5 802154 radio started (channel: %d)",
		    nrf5_radio->channel);

	return 0;
}

static int nrf5_stop(struct device *dev)
{
	ARG_UNUSED(dev);

	if (!nrf_drv_radio802154_sleep()) {
		SYS_LOG_ERR("Error while stopping radio");
		return -EIO;
	}

	SYS_LOG_DBG("nRF5 802154 radio stopped");

	return 0;
}

static uint8_t nrf5_get_lqi(struct device *dev)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	return nrf5_radio->lqi;
}

static void nrf5_radio_irq(void *arg)
{
	ARG_UNUSED(arg);

	nrf_drv_radio802154_irq_handler();
}

static void nrf5_config(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(NRF5_IRQ_RADIO_IRQn, 0, nrf5_radio_irq, NULL, 0);
	irq_enable(NRF5_IRQ_RADIO_IRQn);
}

static int nrf5_init(struct device *dev)
{
	const struct nrf5_802154_config *nrf5_radio_cfg = NRF5_802154_CFG(dev);
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	struct device *clk_m16;

	k_sem_init(&nrf5_radio->rx_wait, 0, 1);
	k_sem_init(&nrf5_radio->tx_wait, 0, 1);
	k_sem_init(&nrf5_radio->cca_wait, 0, 1);

	clk_m16 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clk_m16) {
		return -ENODEV;
	}

	clock_control_on(clk_m16, NULL);

	nrf_drv_radio802154_init();

	nrf5_radio_cfg->irq_config_func(dev);

	k_thread_spawn(nrf5_radio->rx_stack,
		       CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
		       nrf5_rx_thread,
		       dev, NULL, NULL,
		       K_PRIO_COOP(2), 0, 0);

	SYS_LOG_INF("nRF5 802154 radio initialized");

	return 0;
}

static void nrf5_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	SYS_LOG_DBG("");

	nrf5_get_eui64(nrf5_radio->mac);
	net_if_set_link_addr(iface,
			     nrf5_radio->mac,
			     sizeof(nrf5_radio->mac),
			     NET_LINK_IEEE802154);

	nrf5_radio->iface = iface;
	ieee802154_init(iface);
}

/* nRF5 radio driver callbacks */

void nrf_drv_radio802154_received(uint8_t *p_data, int8_t power, int8_t lqi)
{
	nrf5_data.rx_psdu = p_data;
	nrf5_data.rssi = power;
	nrf5_data.lqi = lqi;

	k_sem_give(&nrf5_data.rx_wait);
}

void nrf_drv_radio802154_transmitted(bool pending_bit)
{
	ARG_UNUSED(pending_bit);

	nrf5_data.tx_success = true;
	k_sem_give(&nrf5_data.tx_wait);
}

void nrf_drv_radio802154_busy_channel(void)
{
	k_sem_give(&nrf5_data.tx_wait);
}

void nrf_drv_radio802154_energy_detected(int8_t result)
{
	nrf5_data.channel_ed = result;
	k_sem_give(&nrf5_data.cca_wait);
}

static const struct nrf5_802154_config nrf5_radio_cfg = {
	.irq_config_func = nrf5_config,
};

static struct ieee802154_radio_api nrf5_radio_api = {
	.iface_api.init = nrf5_iface_init,
	.iface_api.send = ieee802154_radio_send,

	.cca = nrf5_cca,
	.set_channel = nrf5_set_channel,
	.set_pan_id = nrf5_set_pan_id,
	.set_short_addr = nrf5_set_short_addr,
	.set_ieee_addr = nrf5_set_ieee_addr,
	.set_txpower = nrf5_set_txpower,
	.start = nrf5_start,
	.stop = nrf5_stop,
	.tx = nrf5_tx,
	.get_lqi = nrf5_get_lqi,
};

NET_DEVICE_INIT(nrf5_154_radio, CONFIG_IEEE802154_NRF5_DRV_NAME,
		nrf5_init, &nrf5_data, &nrf5_radio_cfg,
		CONFIG_IEEE802154_NRF5_INIT_PRIO,
		&nrf5_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);

NET_STACK_INFO_ADDR(RX, nrf5_154_radio,
		    CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
		    CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
		    ((struct nrf5_802154_data *)
		    (&__device_nrf5_154_radio))->rx_stack, 0);
