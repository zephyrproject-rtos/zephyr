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
#include <net/net_pkt.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <net/openthread.h>
#endif

#include <misc/byteorder.h>
#include <string.h>
#include <random/rand32.h>

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

#define ACK_TIMEOUT K_MSEC(10)

/* Convenience defines for RADIO */
#define NRF5_802154_DATA(dev) \
	((struct nrf5_802154_data * const)(dev)->driver_data)

#define NRF5_802154_CFG(dev) \
	((struct nrf5_802154_config * const)(dev)->config->config_info)

static void nrf5_get_eui64(u8_t *mac)
{
	memcpy(mac, (const u32_t *)&NRF_FICR->DEVICEID, 8);
}

static void nrf5_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct device *dev = (struct device *)arg1;
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	struct net_buf *frag = NULL;
	struct net_pkt *pkt;
	u8_t pkt_len;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		pkt = NULL;

		SYS_LOG_DBG("Waiting for frame");
		k_sem_take(&nrf5_radio->rx_wait, K_FOREVER);

		SYS_LOG_DBG("Frame received");

		pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt) {
			SYS_LOG_ERR("No pkt available");
			goto out;
		}

		frag = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!frag) {
			SYS_LOG_ERR("No frag available");
			goto out;
		}

		net_pkt_frag_insert(pkt, frag);

		/* rx_mpdu contains length, psdu, fcs|lqi
		 * The last 2 bytes contain LQI or FCS, depending if
		 * automatic CRC handling is enabled or not, respectively.
		 */
		if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) ||
		    IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {

			pkt_len = nrf5_radio->rx_psdu[0];
		} else {
			pkt_len = nrf5_radio->rx_psdu[0] -  NRF5_FCS_LENGTH;
		}

		/* Skip length (first byte) and copy the payload */
		memcpy(frag->data, nrf5_radio->rx_psdu + 1, pkt_len);
		net_buf_add(frag, pkt_len);

		net_pkt_set_ieee802154_lqi(pkt, nrf5_radio->lqi);
		net_pkt_set_ieee802154_rssi(pkt, nrf5_radio->rssi);

		nrf_drv_radio802154_buffer_free(nrf5_radio->rx_psdu);

		SYS_LOG_DBG("Caught a packet (%u) (LQI: %u)",
			    pkt_len, nrf5_radio->lqi);

		if (net_recv_data(nrf5_radio->iface, pkt) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		net_analyze_stack("nRF5 rx stack",
				  K_THREAD_STACK_BUFFER(nrf5_radio->rx_stack),
				  K_THREAD_STACK_SIZEOF(nrf5_radio->rx_stack));
		continue;

out:
		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

/* Radio device API */

static enum ieee802154_hw_caps nrf5_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS |
		IEEE802154_HW_2_4_GHZ |
		IEEE802154_HW_TX_RX_ACK |
		IEEE802154_HW_FILTER;
}


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

static int nrf5_set_channel(struct device *dev, u16_t channel)
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

static int nrf5_set_pan_id(struct device *dev, u16_t pan_id)
{
	u8_t pan_id_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(pan_id, pan_id_le);
	nrf_drv_radio802154_pan_id_set(pan_id_le);

	SYS_LOG_DBG("0x%x", pan_id);
	return 0;
}

static int nrf5_set_short_addr(struct device *dev, u16_t short_addr)
{
	u8_t short_addr_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(short_addr, short_addr_le);
	nrf_drv_radio802154_short_address_set(short_addr_le);

	SYS_LOG_DBG("0x%x", short_addr);
	return 0;
}

static int nrf5_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	ARG_UNUSED(dev);

	SYS_LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	nrf_drv_radio802154_extended_address_set(ieee_addr);

	return 0;
}

static int nrf5_filter(struct device *dev,
		       bool set,
		       enum ieee802154_filter_type type,
		       const struct ieee802154_filter *filter)
{
	SYS_LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return nrf5_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return nrf5_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return nrf5_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

static int nrf5_set_txpower(struct device *dev, s16_t dbm)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	SYS_LOG_DBG("%d", dbm);
	nrf5_radio->txpower = dbm;

	return 0;
}

static int nrf5_tx(struct device *dev,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	u8_t payload_len = net_pkt_ll_reserve(pkt) + frag->len;
	u8_t *payload = frag->data - net_pkt_ll_reserve(pkt);

	SYS_LOG_DBG("%p (%u)", payload, payload_len);

	nrf5_radio->tx_success = false;
	nrf5_radio->tx_psdu[0] = payload_len + NRF5_FCS_LENGTH;

	memcpy(nrf5_radio->tx_psdu + 1, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&nrf5_radio->tx_wait);

	if (!nrf_drv_radio802154_transmit(nrf5_radio->tx_psdu,
					  nrf5_radio->channel,
					  nrf5_radio->txpower)) {
		SYS_LOG_ERR("Cannot send frame");
		return -EIO;
	}

	SYS_LOG_DBG("Sending frame (ch:%d, txpower:%d)",
		    nrf5_radio->channel,
		    nrf5_radio->txpower);

	/* Wait for ack to be received */
	if (k_sem_take(&nrf5_radio->tx_wait, ACK_TIMEOUT)) {
		SYS_LOG_DBG("ACK not received");
		nrf_drv_radio802154_receive(nrf5_radio->channel, true);

		return -EIO;
	}

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

	k_thread_create(&nrf5_radio->rx_thread, nrf5_radio->rx_stack,
			CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
			nrf5_rx_thread, dev, NULL, NULL,
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

void nrf_drv_radio802154_received(u8_t *p_data, s8_t power, s8_t lqi)
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

void nrf_drv_radio802154_energy_detected(s8_t result)
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

	.get_capabilities = nrf5_get_capabilities,
	.cca = nrf5_cca,
	.set_channel = nrf5_set_channel,
	.filter = nrf5_filter,
	.set_txpower = nrf5_set_txpower,
	.start = nrf5_start,
	.stop = nrf5_stop,
	.tx = nrf5_tx,
};

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU 125
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#endif

#if defined(CONFIG_NET_L2_IEEE802154) || defined(CONFIG_NET_L2_OPENTHREAD)
NET_DEVICE_INIT(nrf5_154_radio, CONFIG_IEEE802154_NRF5_DRV_NAME,
		nrf5_init, &nrf5_data, &nrf5_radio_cfg,
		CONFIG_IEEE802154_NRF5_INIT_PRIO,
		&nrf5_radio_api, L2,
		L2_CTX_TYPE, MTU);

NET_STACK_INFO_ADDR(RX, nrf5_154_radio,
		    CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
		    CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
		    nrf5_data.rx_stack, 0);
#else
DEVICE_AND_API_INIT(nrf5_154_radio, CONFIG_IEEE802154_NRF5_DRV_NAME,
		    nrf5_init, &nrf5_data, &nrf5_radio_cfg,
		    POST_KERNEL, CONFIG_IEEE802154_NRF5_INIT_PRIO,
		    &nrf5_radio_api);
#endif
