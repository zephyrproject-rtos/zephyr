/*
 * Copyright (c) 2024 A Labs GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ieee802154

#define LOG_MODULE_NAME ieee802154_esp32
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/debug/stack.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio_openthread.h>
#endif

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/random/random.h>

#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/irq.h>

#include "ieee802154_esp32.h"
#include <esp_ieee802154.h>
#include <esp_ieee802154_dev.h>
#include <esp_mac.h>

#define IEEE802154_ESP32_TX_TIMEOUT_MS (100)

static struct ieee802154_esp32_data esp32_data;

/* override weak function in components/ieee802154/esp_ieee802154.c of ESP-IDF */
void esp_ieee802154_receive_done(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info)
{
	struct net_pkt *pkt;
	uint8_t *payload;
	uint8_t len;
	int err;

	/* The ESP-IDF HAL handles FCS already and drops frames with bad checksum. The checksum at
	 * the end of a valid frame is replaced with RSSI and LQI values.
	 * Zephyr L2 expects only valid frames, so checksum is not needed for a re-check.
	 */
	if (IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		len = frame[0];
	} else {
		len = frame[0] - IEEE802154_FCS_LENGTH;
	}

#if defined(CONFIG_NET_BUF_DATA_SIZE)
	__ASSERT_NO_MSG(len <= CONFIG_NET_BUF_DATA_SIZE);
#endif

	payload = frame + 1;

	LOG_HEXDUMP_DBG(payload, len, "RX buffer:");

	pkt = net_pkt_rx_alloc_with_buffer(esp32_data.iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No pkt available");
		goto exit;
	}

	err = net_pkt_write(pkt, payload, len);
	if (err != 0) {
		LOG_ERR("Failed to write to a packet: %d", err);
		net_pkt_unref(pkt);
		goto exit;
	}

	net_pkt_set_ieee802154_lqi(pkt, frame_info->lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, frame_info->rssi);
	net_pkt_set_ieee802154_ack_fpb(pkt, frame_info->pending);

	err = net_recv_data(esp32_data.iface, pkt);
	if (err != 0) {
		LOG_ERR("RCV Packet dropped by NET stack: %d", err);
		net_pkt_unref(pkt);
	}

exit:
	esp_ieee802154_receive_handle_done(frame);
}

static enum ieee802154_hw_caps esp32_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * ESP32-C6 Datasheet:
	 * - CSMA/CA
	 * - active scan and energy detect
	 * - HW frame filter
	 * - HW auto acknowledge
	 * - HW auto frame pending
	 * - coordinated sampled listening (CSL)
	 */

	/* ToDo: Double-check and extend */
	return IEEE802154_HW_ENERGY_SCAN | IEEE802154_HW_FILTER | IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_CSMA | IEEE802154_HW_PROMISC | IEEE802154_RX_ON_WHEN_IDLE;
}

/* override weak function in components/ieee802154/esp_ieee802154.c of ESP-IDF */
void IRAM_ATTR esp_ieee802154_cca_done(bool channel_free)
{
	esp32_data.channel_free = channel_free;
	k_sem_give(&esp32_data.cca_wait);
}

static int esp32_cca(const struct device *dev)
{
	struct ieee802154_esp32_data *data = dev->data;
	int err;

	if (ieee802154_cca() != 0) {
		LOG_DBG("CCA failed");
		return -EBUSY;
	}

	err = k_sem_take(&data->cca_wait, K_MSEC(1000));
	if (err == -EAGAIN) {
		LOG_DBG("CCA timed out");
		return -EIO;
	}

	LOG_DBG("Channel free? %d", data->channel_free);

	return data->channel_free ? 0 : -EBUSY;
}

static int esp32_set_channel(const struct device *dev, uint16_t channel)
{
	int err;

	ARG_UNUSED(dev);

	LOG_DBG("Channel: %u", channel);

	if (channel > 26) {
		return -EINVAL;
	} else if (channel < 11) {
		return -ENOTSUP;
	}

	err = esp_ieee802154_set_channel(channel);

	return err == 0 ? 0 : -EIO;
}

static int esp32_filter(const struct device *dev, bool set, enum ieee802154_filter_type type,
			const struct ieee802154_filter *filter)
{
	int err;

	LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		err = esp_ieee802154_set_extended_address(filter->ieee_addr);
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		err = esp_ieee802154_set_short_address(filter->short_addr);
		break;
	case IEEE802154_FILTER_TYPE_PAN_ID:
		err = esp_ieee802154_set_panid(filter->pan_id);
		break;
	default:
		return -ENOTSUP;
	}

	return err == 0 ? 0 : -EIO;
}

static int esp32_set_txpower(const struct device *dev, int16_t dbm)
{
	int err;

	ARG_UNUSED(dev);

	LOG_DBG("TX power: %u dBm", dbm);

	if (dbm > CONFIG_ESP32_PHY_MAX_TX_POWER) {
		return -EINVAL;
	}

	err = esp_ieee802154_set_txpower(dbm);

	return err == 0 ? 0 : -EIO;
}

static int handle_ack(struct ieee802154_esp32_data *data)
{
	uint8_t ack_len;
	struct net_pkt *ack_pkt;
	int err = 0;

	if (data->ack_frame == NULL || data->ack_frame_info == NULL) {
		/* no ACK received, nothing to do */
		return 0;
	}

	if (IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		ack_len = data->ack_frame[0];
	} else {
		ack_len = data->ack_frame[0] - IEEE802154_FCS_LENGTH;
	}

	ack_pkt = net_pkt_rx_alloc_with_buffer(data->iface, ack_len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		err = -ENOMEM;
		goto free_esp_ack;
	}

	/* Upper layers expect the frame to start at the MAC header, skip the
	 * PHY header (PHR byte containing the length).
	 */
	if (net_pkt_write(ack_pkt, data->ack_frame + 1, ack_len) < 0) {
		LOG_ERR("Failed to write to a packet.");
		err = -ENOMEM;
		goto free_net_ack;
	}

	net_pkt_set_ieee802154_lqi(ack_pkt, data->ack_frame_info->lqi);
	net_pkt_set_ieee802154_rssi_dbm(ack_pkt, data->ack_frame_info->rssi);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	net_pkt_set_timestamp_ns(ack_pkt, data->ack_frame_info->time * NSEC_PER_USEC);
#endif

	net_pkt_cursor_init(ack_pkt);

	if (ieee802154_handle_ack(data->iface, ack_pkt) != NET_OK) {
		LOG_INF("ACK packet not handled - releasing.");
	}

free_net_ack:
	net_pkt_unref(ack_pkt);

free_esp_ack:
	esp_ieee802154_receive_handle_done(data->ack_frame);
	data->ack_frame = NULL;

	return err;
}

/* override weak function in components/ieee802154/esp_ieee802154.c of ESP-IDF */
void IRAM_ATTR esp_ieee802154_transmit_done(const uint8_t *tx_frame, const uint8_t *ack_frame,
					    esp_ieee802154_frame_info_t *ack_frame_info)
{
	esp32_data.ack_frame = ack_frame;
	esp32_data.ack_frame_info = ack_frame_info;

	k_sem_give(&esp32_data.tx_wait);
}

/* override weak function in components/ieee802154/esp_ieee802154.c of ESP-IDF */
void IRAM_ATTR esp_ieee802154_transmit_failed(const uint8_t *frame, esp_ieee802154_tx_error_t error)
{
	k_sem_give(&esp32_data.tx_wait);
}

static int esp32_tx(const struct device *dev, enum ieee802154_tx_mode tx_mode, struct net_pkt *pkt,
		    struct net_buf *frag)
{
	struct ieee802154_esp32_data *data = dev->data;
	uint8_t payload_len = frag->len;
	uint8_t *payload = frag->data;
	uint64_t net_time_us;
	int err;

	if (payload_len > IEEE802154_MTU) {
		LOG_ERR("Payload too large: %d", payload_len);
		return -EMSGSIZE;
	}

	LOG_HEXDUMP_DBG(payload, payload_len, "TX buffer:");

	data->tx_psdu[0] = payload_len + IEEE802154_FCS_LENGTH;
	memcpy(data->tx_psdu + 1, payload, payload_len);

	k_sem_reset(&data->tx_wait);

	switch (tx_mode) {
	case IEEE802154_TX_MODE_DIRECT:
		err = esp_ieee802154_transmit(data->tx_psdu, false);
		break;
	case IEEE802154_TX_MODE_CSMA_CA:
		/*
		 * The second parameter of esp_ieee802154_transmit is called CCA, but actually
		 * means CSMA/CA (see also ESP-IDF implementation of OpenThread interface).
		 */
		err = esp_ieee802154_transmit(data->tx_psdu, true);
		break;
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
		/**
		 * The Espressif HAL functions seem to expect a system uptime in us stored as
		 * uint32_t, which would overflow already after 1.2 hours. In addition to that, the
		 * network time from PTP, which is returned by net_pkt_timestamp_ns, will most
		 * probably have a different basis. Anyway, time-based transfers are required for
		 * some Thread features, so this will have to be fixed in the future.
		 *
		 * See also:
		 * - include/zephyr/net/net_time.h
		 * - ../modules/hal/espressif/components/ieee802154/driver/esp_ieee802154_dev.c
		 */
		net_time_us = net_pkt_timestamp_ns(pkt) / NSEC_PER_USEC;
		err = esp_ieee802154_transmit_at(data->tx_psdu,
						 tx_mode == IEEE802154_TX_MODE_TXTIME_CCA,
						 (uint32_t)net_time_us);
		break;
	default:
		LOG_ERR("TX mode %d not supported", tx_mode);
		return -ENOTSUP;
	}

	err = k_sem_take(&data->tx_wait, K_MSEC(IEEE802154_ESP32_TX_TIMEOUT_MS));

	if (err != 0) {
		LOG_ERR("TX timeout");
	} else {
		handle_ack(data);
	}

	return err == 0 ? 0 : -EIO;
}

static int esp32_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (esp_ieee802154_receive() != 0) {
		LOG_ERR("Failed to start radio");
		return -EIO;
	}

	return 0;
}

static int esp32_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (esp_ieee802154_sleep() != 0) {
		LOG_ERR("Failed to stop radio");
		return -EIO;
	}

	return 0;
}

/* override weak function in components/ieee802154/esp_ieee802154.c of ESP-IDF */
void IRAM_ATTR esp_ieee802154_energy_detect_done(int8_t power)
{
	energy_scan_done_cb_t callback;
	const struct device *dev;

	if (esp32_data.energy_scan_done == NULL) {
		return;
	}

	callback = esp32_data.energy_scan_done;
	esp32_data.energy_scan_done = NULL;
	dev = net_if_get_device(esp32_data.iface);
	callback(dev, power);
}

static int esp32_ed_scan(const struct device *dev, uint16_t duration, energy_scan_done_cb_t done_cb)
{
	ARG_UNUSED(dev);

	int err = 0;

	if (esp32_data.energy_scan_done == NULL) {
		esp32_data.energy_scan_done = done_cb;

		/* The duration of energy detection, in symbol unit (16 us) */
		if (esp_ieee802154_energy_detect(duration * USEC_PER_MSEC / US_PER_SYMBLE) != 0) {
			esp32_data.energy_scan_done = NULL;
			err = -EBUSY;
		}
	} else {
		err = -EALREADY;
	}

	return err;
}

static int esp32_configure(const struct device *dev, enum ieee802154_config_type type,
			   const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	switch (type) {
	/* IEEE802154_CONFIG_ACK_FPB */
	/* IEEE802154_CONFIG_EVENT_HANDLER */
	case IEEE802154_CONFIG_PROMISCUOUS:
		esp_ieee802154_set_promiscuous(config->promiscuous);
		break;
	case IEEE802154_CONFIG_RX_ON_WHEN_IDLE:
		esp_ieee802154_set_rx_when_idle(config->rx_on_when_idle);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

static int esp32_attr_get(const struct device *dev, enum ieee802154_attr attr,
			  struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

static int esp32_init(const struct device *dev)
{
	struct ieee802154_esp32_data *data = dev->data;

	k_sem_init(&data->cca_wait, 0, 1);
	k_sem_init(&data->tx_wait, 0, 1);

	if (esp_ieee802154_enable() != 0) {
		LOG_ERR("IEEE 802154 enabling failed!");
		return -EIO;
	}

	/* Default radio settings */
	esp_ieee802154_set_promiscuous(false);
	esp_ieee802154_set_rx_when_idle(true);

	LOG_INF("IEEE 802154 radio initialized");

	return 0;
}

static void esp32_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct ieee802154_esp32_data *data = dev->data;

	esp_efuse_mac_get_default(data->mac);
	net_if_set_link_addr(iface, data->mac, sizeof(data->mac), NET_LINK_IEEE802154);

	data->iface = iface;

	ieee802154_init(iface);
}

static const struct ieee802154_radio_api esp32_radio_api = {
	.iface_api.init = esp32_iface_init,

	.get_capabilities = esp32_get_capabilities,
	.cca              = esp32_cca,
	.set_channel      = esp32_set_channel,
	.filter           = esp32_filter,
	.set_txpower      = esp32_set_txpower,
	.tx               = esp32_tx,
	.start            = esp32_start,
	.stop             = esp32_stop,
	.ed_scan          = esp32_ed_scan,
	.configure        = esp32_configure,
	.attr_get         = esp32_attr_get,
};

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#endif

#if defined(CONFIG_NET_L2_PHY_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, esp32_init, NULL, &esp32_data, NULL,
			  CONFIG_IEEE802154_ESP32_INIT_PRIO, &esp32_radio_api, L2,
			  L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, esp32_init, NULL, &esp32_data, NULL, POST_KERNEL,
		      CONFIG_IEEE802154_ESP32_INIT_PRIO, &esp32_radio_api);
#endif
