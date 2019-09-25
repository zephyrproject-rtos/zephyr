/* ieee802154_nrf5.c - nRF5 802.15.4 driver */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_nrf5
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <soc.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <net/openthread.h>
#endif

#include <sys/byteorder.h>
#include <string.h>
#include <random/rand32.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_nrf5.h"
#include "nrf_802154.h"

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
	struct net_pkt *pkt;
	struct nrf5_802154_rx_frame *rx_frame;
	u8_t pkt_len;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		pkt = NULL;
		rx_frame = NULL;

		LOG_DBG("Waiting for frame");

		rx_frame = k_fifo_get(&nrf5_radio->rx_fifo, K_FOREVER);

		__ASSERT_NO_MSG(rx_frame->psdu);

		/* rx_mpdu contains length, psdu, fcs|lqi
		 * The last 2 bytes contain LQI or FCS, depending if
		 * automatic CRC handling is enabled or not, respectively.
		 */
		if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) ||
		    IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
			pkt_len = rx_frame->psdu[0];
		} else {
			pkt_len = rx_frame->psdu[0] -  NRF5_FCS_LENGTH;
		}

		__ASSERT_NO_MSG(pkt_len <= CONFIG_NET_BUF_DATA_SIZE);

		LOG_DBG("Frame received");

		pkt = net_pkt_alloc_with_buffer(nrf5_radio->iface, pkt_len,
						AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("No pkt available");
			goto drop;
		}

		if (net_pkt_write(pkt, rx_frame->psdu + 1, pkt_len)) {
			goto drop;
		}

		net_pkt_set_ieee802154_lqi(pkt, rx_frame->lqi);
		net_pkt_set_ieee802154_rssi(pkt, rx_frame->rssi);

		LOG_DBG("Caught a packet (%u) (LQI: %u)",
			 pkt_len, rx_frame->lqi);

		if (net_recv_data(nrf5_radio->iface, pkt) < 0) {
			LOG_ERR("Packet dropped by NET stack");
			goto drop;
		}

		nrf_802154_buffer_free_raw(rx_frame->psdu);
		rx_frame->psdu = NULL;

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			net_analyze_stack(
				"nRF5 rx stack",
				Z_THREAD_STACK_BUFFER(nrf5_radio->rx_stack),
				K_THREAD_STACK_SIZEOF(nrf5_radio->rx_stack));
		}

		continue;

drop:
		nrf_802154_buffer_free_raw(rx_frame->psdu);
		rx_frame->psdu = NULL;

		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

/* Radio device API */

static enum ieee802154_hw_caps nrf5_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_2_4_GHZ |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_FILTER;
}


static int nrf5_cca(struct device *dev)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	if (!nrf_802154_cca()) {
		LOG_DBG("CCA failed");
		return -EBUSY;
	}

	/* The nRF driver guarantees that a callback will be called once
	 * the CCA function is done, thus unlocking the semaphore.
	 */
	k_sem_take(&nrf5_radio->cca_wait, K_FOREVER);

	LOG_DBG("Channel free? %d", nrf5_radio->channel_free);

	return nrf5_radio->channel_free ? 0 : -EBUSY;
}

static int nrf5_set_channel(struct device *dev, u16_t channel)
{
	ARG_UNUSED(dev);

	LOG_DBG("%u", channel);

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	nrf_802154_channel_set(channel);

	return 0;
}

static int nrf5_set_pan_id(struct device *dev, u16_t pan_id)
{
	u8_t pan_id_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(pan_id, pan_id_le);
	nrf_802154_pan_id_set(pan_id_le);

	LOG_DBG("0x%x", pan_id);

	return 0;
}

static int nrf5_set_short_addr(struct device *dev, u16_t short_addr)
{
	u8_t short_addr_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(short_addr, short_addr_le);
	nrf_802154_short_address_set(short_addr_le);

	LOG_DBG("0x%x", short_addr);

	return 0;
}

static int nrf5_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	ARG_UNUSED(dev);

	LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	nrf_802154_extended_address_set(ieee_addr);

	return 0;
}

static int nrf5_filter(struct device *dev, bool set,
		       enum ieee802154_filter_type type,
		       const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

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
	ARG_UNUSED(dev);

	LOG_DBG("%d", dbm);

	nrf_802154_tx_power_set(dbm);

	return 0;
}

static int nrf5_tx(struct device *dev,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	u8_t payload_len = frag->len;
	u8_t *payload = frag->data;

	LOG_DBG("%p (%u)", payload, payload_len);

	nrf5_radio->tx_psdu[0] = payload_len + NRF5_FCS_LENGTH;
	memcpy(nrf5_radio->tx_psdu + 1, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&nrf5_radio->tx_wait);

	if (!nrf_802154_transmit_raw(nrf5_radio->tx_psdu, false)) {
		LOG_ERR("Cannot send frame");
		return -EIO;
	}

	LOG_DBG("Sending frame (ch:%d, txpower:%d)",
		nrf_802154_channel_get(), nrf_802154_tx_power_get());

	/* Wait for ack to be received */
	if (k_sem_take(&nrf5_radio->tx_wait, ACK_TIMEOUT)) {
		LOG_DBG("ACK not received");

		if (!nrf_802154_receive()) {
			LOG_ERR("Failed to switch back to receive state");
		}

		return -EIO;
	}

	LOG_DBG("Result: %d", nrf5_data.tx_result);

	if (nrf5_radio->tx_result == NRF_802154_TX_ERROR_NONE) {
		/* ACK frame not used currently. */
		if (nrf5_radio->ack != NULL) {
			nrf_802154_buffer_free_raw(nrf5_radio->ack);
		}

		nrf5_radio->ack = NULL;

		return 0;
	}

	return -EIO;
}

static int nrf5_start(struct device *dev)
{
	ARG_UNUSED(dev);

	if (!nrf_802154_receive()) {
		LOG_ERR("Failed to enter receive state");
		return -EIO;
	}

	LOG_DBG("nRF5 802154 radio started (channel: %d)",
		nrf_802154_channel_get());

	return 0;
}

static int nrf5_stop(struct device *dev)
{
	ARG_UNUSED(dev);

	if (!nrf_802154_sleep()) {
		LOG_ERR("Error while stopping radio");
		return -EIO;
	}

	LOG_DBG("nRF5 802154 radio stopped");

	return 0;
}

static void nrf5_radio_irq(void *arg)
{
	ARG_UNUSED(arg);

	nrf_802154_radio_irq_handler();
}

static void nrf5_irq_config(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(RADIO_IRQn, NRF_802154_IRQ_PRIORITY,
		    nrf5_radio_irq, NULL, 0);
	irq_enable(RADIO_IRQn);
}

static int nrf5_init(struct device *dev)
{
	const struct nrf5_802154_config *nrf5_radio_cfg = NRF5_802154_CFG(dev);
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	k_fifo_init(&nrf5_radio->rx_fifo);
	k_sem_init(&nrf5_radio->tx_wait, 0, 1);
	k_sem_init(&nrf5_radio->cca_wait, 0, 1);

	nrf_802154_init();

	nrf5_radio_cfg->irq_config_func(dev);

	k_thread_create(&nrf5_radio->rx_thread, nrf5_radio->rx_stack,
			CONFIG_IEEE802154_NRF5_RX_STACK_SIZE,
			nrf5_rx_thread, dev, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_thread_name_set(&nrf5_radio->rx_thread, "802154 RX");

	LOG_INF("nRF5 802154 radio initialized");

	return 0;
}

static void nrf5_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	nrf5_get_eui64(nrf5_radio->mac);
	net_if_set_link_addr(iface, nrf5_radio->mac, sizeof(nrf5_radio->mac),
			     NET_LINK_IEEE802154);

	nrf5_radio->iface = iface;

	ieee802154_init(iface);
}

int nrf5_configure(struct device *dev, enum ieee802154_config_type type,
		   const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		nrf_802154_auto_pending_bit_set(config->auto_ack_fpb.enabled);
		break;

	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.enabled) {
			if (!nrf_802154_pending_bit_for_addr_set(
						config->ack_fpb.addr,
						config->ack_fpb.extended)) {
				return -ENOMEM;
			}

			break;
		}

		if (config->ack_fpb.addr != NULL) {
			if (!nrf_802154_pending_bit_for_addr_clear(
						config->ack_fpb.addr,
						config->ack_fpb.extended)) {
				return -ENOENT;
			}
		} else {
			nrf_802154_pending_bit_for_addr_reset(
						config->ack_fpb.extended);
		}

		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* nRF5 radio driver callbacks */

void nrf_802154_received_raw(uint8_t *data, int8_t power, uint8_t lqi)
{
	for (u32_t i = 0; i < ARRAY_SIZE(nrf5_data.rx_frames); i++) {
		if (nrf5_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		nrf5_data.rx_frames[i].psdu = data;
		nrf5_data.rx_frames[i].rssi = power;
		nrf5_data.rx_frames[i].lqi = lqi;

		k_fifo_put(&nrf5_data.rx_fifo, &nrf5_data.rx_frames[i]);

		return;
	}

	__ASSERT(false, "Not enough rx frames allocated for 15.4 driver");
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error)
{
	/* Intentionally empty. */
}

void nrf_802154_transmitted_raw(const uint8_t *frame, uint8_t *ack,
				int8_t power, uint8_t lqi)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(power);
	ARG_UNUSED(lqi);

	nrf5_data.tx_result = NRF_802154_TX_ERROR_NONE;
	nrf5_data.ack = ack;

	k_sem_give(&nrf5_data.tx_wait);
}

void nrf_802154_transmit_failed(const uint8_t *frame,
				nrf_802154_tx_error_t error)
{
	ARG_UNUSED(frame);

	nrf5_data.tx_result = error;

	k_sem_give(&nrf5_data.tx_wait);
}

void nrf_802154_cca_done(bool channel_free)
{
	nrf5_data.channel_free = channel_free;

	k_sem_give(&nrf5_data.cca_wait);
}

void nrf_802154_cca_failed(nrf_802154_cca_error_t error)
{
	ARG_UNUSED(error);

	nrf5_data.channel_free = false;

	k_sem_give(&nrf5_data.cca_wait);
}

static const struct nrf5_802154_config nrf5_radio_cfg = {
	.irq_config_func = nrf5_irq_config,
};

static struct ieee802154_radio_api nrf5_radio_api = {
	.iface_api.init = nrf5_iface_init,

	.get_capabilities = nrf5_get_capabilities,
	.cca = nrf5_cca,
	.set_channel = nrf5_set_channel,
	.filter = nrf5_filter,
	.set_txpower = nrf5_set_txpower,
	.start = nrf5_start,
	.stop = nrf5_stop,
	.tx = nrf5_tx,
	.configure = nrf5_configure,
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
