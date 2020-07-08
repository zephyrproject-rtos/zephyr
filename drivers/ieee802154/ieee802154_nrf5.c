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
#include <debug/stack.h>

#include <soc.h>
#include <device.h>
#include <init.h>
#include <debug/stack.h>
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
	void (*irq_config_func)(const struct device *dev);
};

static struct nrf5_802154_data nrf5_data;

#define ACK_REQUEST_BYTE 1
#define ACK_REQUEST_BIT (1 << 5)
#define FRAME_PENDING_BYTE 1
#define FRAME_PENDING_BIT (1 << 4)

/* Convenience defines for RADIO */
#define NRF5_802154_DATA(dev) \
	((struct nrf5_802154_data * const)(dev)->data)

#define NRF5_802154_CFG(dev) \
	((const struct nrf5_802154_config * const)(dev)->config)

#if CONFIG_IEEE802154_VENDOR_OUI_ENABLE
#define IEEE802154_NRF5_VENDOR_OUI CONFIG_IEEE802154_VENDOR_OUI
#else
#define IEEE802154_NRF5_VENDOR_OUI (uint32_t)0xF4CE36
#endif

static void nrf5_get_eui64(uint8_t *mac)
{
	uint64_t factoryAddress;
	uint32_t index = 0;

	/* Set the MAC Address Block Larger (MA-L) formerly called OUI. */
	mac[index++] = (IEEE802154_NRF5_VENDOR_OUI >> 16) & 0xff;
	mac[index++] = (IEEE802154_NRF5_VENDOR_OUI >> 8) & 0xff;
	mac[index++] = IEEE802154_NRF5_VENDOR_OUI & 0xff;

	/* Use device identifier assigned during the production. */
	factoryAddress = (uint64_t)NRF_FICR->DEVICEID[0] << 32;
	factoryAddress |= NRF_FICR->DEVICEID[1];
	memcpy(mac + index, &factoryAddress, sizeof(factoryAddress) - index);
}

static void nrf5_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct nrf5_802154_data *nrf5_radio = (struct nrf5_802154_data *)arg1;
	struct net_pkt *pkt;
	struct nrf5_802154_rx_frame *rx_frame;
	uint8_t pkt_len;

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
		net_pkt_set_ieee802154_ack_fpb(pkt, rx_frame->ack_fpb);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
		struct net_ptp_time timestamp = {
			.second = rx_frame->time / USEC_PER_SEC,
			.nanosecond =
				(rx_frame->time % USEC_PER_SEC) * NSEC_PER_USEC
		};

		net_pkt_set_timestamp(pkt, &timestamp);
#endif

		LOG_DBG("Caught a packet (%u) (LQI: %u)",
			 pkt_len, rx_frame->lqi);

		if (net_recv_data(nrf5_radio->iface, pkt) < 0) {
			LOG_ERR("Packet dropped by NET stack");
			goto drop;
		}

		nrf_802154_buffer_free_raw(rx_frame->psdu);
		rx_frame->psdu = NULL;

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			log_stack_usage(&nrf5_radio->rx_thread);
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

static enum ieee802154_hw_caps nrf5_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER |
	       IEEE802154_HW_CSMA | IEEE802154_HW_2_4_GHZ |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_ENERGY_SCAN |
	       IEEE802154_HW_SLEEP_TO_TX;
}

static int nrf5_cca(const struct device *dev)
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

static int nrf5_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);

	LOG_DBG("%u", channel);

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	nrf_802154_channel_set(channel);

	return 0;
}

static int nrf5_energy_scan_start(const struct device *dev,
				  uint16_t duration,
				  energy_scan_done_cb_t done_cb)
{
	int err = 0;

	ARG_UNUSED(dev);

	if (nrf5_data.energy_scan_done == NULL) {
		nrf5_data.energy_scan_done = done_cb;

		if (nrf_802154_energy_detection(duration * 1000) == false) {
			nrf5_data.energy_scan_done = NULL;
			err = -EPERM;
		}
	} else {
		err = -EALREADY;
	}

	return err;
}

static int nrf5_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	uint8_t pan_id_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(pan_id, pan_id_le);
	nrf_802154_pan_id_set(pan_id_le);

	LOG_DBG("0x%x", pan_id);

	return 0;
}

static int nrf5_set_short_addr(const struct device *dev, uint16_t short_addr)
{
	uint8_t short_addr_le[2];

	ARG_UNUSED(dev);

	sys_put_le16(short_addr, short_addr_le);
	nrf_802154_short_address_set(short_addr_le);

	LOG_DBG("0x%x", short_addr);

	return 0;
}

static int nrf5_set_ieee_addr(const struct device *dev,
			      const uint8_t *ieee_addr)
{
	ARG_UNUSED(dev);

	LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	nrf_802154_extended_address_set(ieee_addr);

	return 0;
}

static int nrf5_filter(const struct device *dev, bool set,
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

static int nrf5_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);

	LOG_DBG("%d", dbm);

	nrf_802154_tx_power_set(dbm);

	return 0;
}

static int handle_ack(struct nrf5_802154_data *nrf5_radio)
{
	uint8_t ack_len = nrf5_radio->ack_frame.psdu[0] - NRF5_FCS_LENGTH;
	struct net_pkt *ack_pkt;
	int err = 0;

	ack_pkt = net_pkt_alloc_with_buffer(nrf5_radio->iface, ack_len,
					    AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		err = -ENOMEM;
		goto free_nrf_ack;
	}

	/* Upper layers expect the frame to start at the MAC header, skip the
	 * PHY header (1 byte).
	 */
	if (net_pkt_write(ack_pkt, nrf5_radio->ack_frame.psdu + 1,
			  ack_len) < 0) {
		LOG_ERR("Failed to write to a packet.");
		err = -ENOMEM;
		goto free_net_ack;
	}

	net_pkt_set_ieee802154_lqi(ack_pkt, nrf5_radio->ack_frame.lqi);
	net_pkt_set_ieee802154_rssi(ack_pkt, nrf5_radio->ack_frame.rssi);

	net_pkt_cursor_init(ack_pkt);

	if (ieee802154_radio_handle_ack(nrf5_radio->iface, ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled - releasing.");
	}

free_net_ack:
	net_pkt_unref(ack_pkt);

free_nrf_ack:
	nrf_802154_buffer_free_raw(nrf5_radio->ack_frame.psdu);
	nrf5_radio->ack_frame.psdu = NULL;

	return err;
}

static void nrf5_tx_started(const struct device *dev,
			    struct net_pkt *pkt,
			    struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	if (nrf5_data.event_handler) {
		nrf5_data.event_handler(dev, IEEE802154_EVENT_TX_STARTED,
					(void *)frag);
	}
}

static int nrf5_tx(const struct device *dev,
		   enum ieee802154_tx_mode mode,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	uint8_t payload_len = frag->len;
	uint8_t *payload = frag->data;
	bool ret = true;

	LOG_DBG("%p (%u)", payload, payload_len);

	nrf5_radio->tx_psdu[0] = payload_len + NRF5_FCS_LENGTH;
	memcpy(nrf5_radio->tx_psdu + 1, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&nrf5_radio->tx_wait);

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
		ret = nrf_802154_transmit_raw(nrf5_radio->tx_psdu, false);
		break;
	case IEEE802154_TX_MODE_CCA:
		ret = nrf_802154_transmit_raw(nrf5_radio->tx_psdu, true);
		break;
	case IEEE802154_TX_MODE_CSMA_CA:
		nrf_802154_transmit_csma_ca_raw(nrf5_radio->tx_psdu);
		break;
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
	default:
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	if (!ret) {
		LOG_ERR("Cannot send frame");
		return -EIO;
	}

	nrf5_tx_started(dev, pkt, frag);

	LOG_DBG("Sending frame (ch:%d, txpower:%d)",
		nrf_802154_channel_get(), nrf_802154_tx_power_get());

	/* Wait for the callback from the radio driver. */
	k_sem_take(&nrf5_radio->tx_wait, K_FOREVER);

	LOG_DBG("Result: %d", nrf5_data.tx_result);

	if (nrf5_radio->tx_result == NRF_802154_TX_ERROR_NONE) {
		if (nrf5_radio->ack_frame.psdu == NULL) {
			/* No ACK was requested. */
			return 0;
		}

		/* Handle ACK packet. */
		return handle_ack(nrf5_radio);
	}

	return -EIO;
}

static int nrf5_start(const struct device *dev)
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

static int nrf5_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (!nrf_802154_sleep()) {
		LOG_ERR("Error while stopping radio");
		return -EIO;
	}

	LOG_DBG("nRF5 802154 radio stopped");

	return 0;
}

#ifndef CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT
static void nrf5_radio_irq(void *arg)
{
	ARG_UNUSED(arg);

	nrf_802154_radio_irq_handler();
}
#endif

static void nrf5_irq_config(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifndef CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT
	IRQ_CONNECT(RADIO_IRQn, NRF_802154_IRQ_PRIORITY,
		    nrf5_radio_irq, NULL, 0);
	irq_enable(RADIO_IRQn);
#endif
}

static int nrf5_init(const struct device *dev)
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
			nrf5_rx_thread, nrf5_radio, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_thread_name_set(&nrf5_radio->rx_thread, "nrf5_rx");

	LOG_INF("nRF5 802154 radio initialized");

	return 0;
}

static void nrf5_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	nrf5_get_eui64(nrf5_radio->mac);
	net_if_set_link_addr(iface, nrf5_radio->mac, sizeof(nrf5_radio->mac),
			     NET_LINK_IEEE802154);

	nrf5_radio->iface = iface;

	ieee802154_init(iface);
}

static int nrf5_configure(const struct device *dev,
			  enum ieee802154_config_type type,
			  const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		if (config->auto_ack_fpb.enabled) {
			switch (config->auto_ack_fpb.mode) {
			case IEEE802154_FPB_ADDR_MATCH_THREAD:
				nrf_802154_src_addr_matching_method_set(
					NRF_802154_SRC_ADDR_MATCH_THREAD);
				break;

			case IEEE802154_FPB_ADDR_MATCH_ZIGBEE:
				nrf_802154_src_addr_matching_method_set(
					NRF_802154_SRC_ADDR_MATCH_ZIGBEE);
				break;

			default:
				return -EINVAL;
			}
		}

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

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		nrf_802154_pan_coord_set(config->pan_coordinator);
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		nrf_802154_promiscuous_set(config->promiscuous);
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		nrf5_data.event_handler = config->event_handler;

	default:
		return -EINVAL;
	}

	return 0;
}

/* nRF5 radio driver callbacks */

void nrf_802154_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi,
				       uint32_t time)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(nrf5_data.rx_frames); i++) {
		if (nrf5_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		nrf5_data.rx_frames[i].psdu = data;
		nrf5_data.rx_frames[i].time = time;
		nrf5_data.rx_frames[i].rssi = power;
		nrf5_data.rx_frames[i].lqi = lqi;

		if (data[ACK_REQUEST_BYTE] & ACK_REQUEST_BIT) {
			nrf5_data.rx_frames[i].ack_fpb =
						nrf5_data.last_frame_ack_fpb;
		} else {
			nrf5_data.rx_frames[i].ack_fpb = false;
		}

		nrf5_data.last_frame_ack_fpb = false;

		k_fifo_put(&nrf5_data.rx_fifo, &nrf5_data.rx_frames[i]);

		return;
	}

	__ASSERT(false, "Not enough rx frames allocated for 15.4 driver");
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error)
{
	nrf5_data.last_frame_ack_fpb = false;
}

void nrf_802154_tx_ack_started(const uint8_t *data)
{
	nrf5_data.last_frame_ack_fpb =
				data[FRAME_PENDING_BYTE] & FRAME_PENDING_BIT;
}

void nrf_802154_transmitted_raw(const uint8_t *frame, uint8_t *ack,
				int8_t power, uint8_t lqi)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(power);
	ARG_UNUSED(lqi);

	nrf5_data.tx_result = NRF_802154_TX_ERROR_NONE;
	nrf5_data.ack_frame.psdu = ack;
	nrf5_data.ack_frame.rssi = power;
	nrf5_data.ack_frame.lqi = lqi;

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

void nrf_802154_energy_detected(uint8_t result)
{
	if (nrf5_data.energy_scan_done != NULL) {
		int16_t dbm;
		energy_scan_done_cb_t callback = nrf5_data.energy_scan_done;

		nrf5_data.energy_scan_done = NULL;
		dbm = nrf_802154_dbm_from_energy_level_calculate(result);
		callback(net_if_get_device(nrf5_data.iface), dbm);
	}
}

void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	if (nrf5_data.energy_scan_done != NULL) {
		energy_scan_done_cb_t callback = nrf5_data.energy_scan_done;

		nrf5_data.energy_scan_done = NULL;
		callback(net_if_get_device(nrf5_data.iface), SHRT_MAX);
	}
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
	.ed_scan = nrf5_energy_scan_start,
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
		nrf5_init, device_pm_control_nop, &nrf5_data, &nrf5_radio_cfg,
		CONFIG_IEEE802154_NRF5_INIT_PRIO,
		&nrf5_radio_api, L2,
		L2_CTX_TYPE, MTU);
#else
DEVICE_AND_API_INIT(nrf5_154_radio, CONFIG_IEEE802154_NRF5_DRV_NAME,
		    nrf5_init, &nrf5_data, &nrf5_radio_cfg,
		    POST_KERNEL, CONFIG_IEEE802154_NRF5_INIT_PRIO,
		    &nrf5_radio_api);
#endif
