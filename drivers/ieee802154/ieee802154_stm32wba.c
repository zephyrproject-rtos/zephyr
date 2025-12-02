/* ieee802154_stm32wba.c - STM32WBxx 802.15.4 driver */

/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <errno.h>
#include <string.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/byteorder.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio_openthread.h>
#endif /* CONFIG_NET_L2_OPENTHREAD */

#ifdef CONFIG_PM_DEVICE
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include "app_conf.h"
#include "linklayer_plat.h"
#include <stm32wbaxx_ll_bus.h>
#include <stm32wbaxx_ll_pwr.h>
#endif

#include <linklayer_plat_local.h>
#include "ieee802154_stm32wba.h"
#include <stm32wba_802154_intf.h>

#define DT_DRV_COMPAT st_stm32wba_ieee802154

#define LOG_MODULE_NAME ieee802154_stm32wba

#define LOG_LEVEL _LOG_LEVEL_RESOLVE(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)

LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

extern uint32_t llhwc_cmn_is_dp_slp_enabled(void);

static struct stm32wba_802154_data_t stm32wba_802154_data;

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

#define NSEC_PER_TEN_SYMBOLS (10 * IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS)

#define MAX_CSMA_BACKOFF 4
#define MAX_FRAME_RETRY  3
#define CCA_THRESHOLD    (-70)

static void stm32wba_802154_receive_done(uint8_t *p_buffer,
					 stm32wba_802154_ral_receive_done_metadata_t *p_metadata);
void stm32wba_802154_tx_ack_started(bool ack_fpb, bool ack_seb);
static void stm32wba_802154_transmit_done(
				uint8_t *p_frame,
				stm32wba_802154_ral_tx_error_t error,
				const stm32wba_802154_ral_transmit_done_metadata_t *p_metadata);
static void stm32wba_802154_cca_done(uint8_t error);
static void stm32wba_802154_energy_scan_done(int8_t rssi_result);

static const struct device *stm32wba_802154_get_device(void)
{
	LOG_DBG("Getting device instance");

	return net_if_get_device(stm32wba_802154_data.iface);
}

static void stm32wba_802154_get_eui64(uint8_t *mac)
{
	stm32wba_802154_ral_eui64_get(mac);

	LOG_DBG("Device EUI64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
}

static void stm32wba_802154_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct stm32wba_802154_data_t *stm32wba_radio = (struct stm32wba_802154_data_t *)arg1;
	struct net_pkt *pkt;
	struct stm32wba_802154_rx_frame *rx_frame;
	uint8_t pkt_len;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("RX thread started");

	while (1) {
		pkt = NULL;

		rx_frame = k_fifo_get(&stm32wba_radio->rx_fifo, K_FOREVER);

		__ASSERT_NO_MSG(rx_frame->psdu != NULL);

		/* Depending on the net L2 layer, the FCS may be included in length or not */
		if (IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
			pkt_len = rx_frame->length;
		} else {
			pkt_len = rx_frame->length - IEEE802154_FCS_LENGTH;
		}

#if defined(CONFIG_NET_BUF_DATA_SIZE)
		__ASSERT_NO_MSG(pkt_len <= CONFIG_NET_BUF_DATA_SIZE);
#endif

		LOG_DBG("Frame received - sequence nb: %u, length: %u", rx_frame->psdu[2],
			pkt_len);

		/* Block the RX thread until net_pkt is available, so that we
		 * don't drop already ACKed frame in case of temporary net_pkt
		 * scarcity. The STM32WBA 802154 radio driver will accumulate any
		 * incoming frames until it runs out of internal buffers (and
		 * thus stops acknowledging consecutive frames).
		 */
		pkt = net_pkt_rx_alloc_with_buffer(stm32wba_radio->iface, pkt_len,
						   AF_UNSPEC, 0, K_FOREVER);

		if (net_pkt_write(pkt, rx_frame->psdu, pkt_len) != 0) {
			LOG_ERR("Failed to write packet data");
			net_pkt_unref(pkt);
		} else {
			net_pkt_set_ieee802154_lqi(pkt, rx_frame->lqi);
			net_pkt_set_ieee802154_rssi_dbm(pkt, rx_frame->rssi);
			net_pkt_set_ieee802154_ack_fpb(pkt, rx_frame->ack_fpb);

#if defined(CONFIG_NET_L2_OPENTHREAD)
			net_pkt_set_ieee802154_ack_seb(pkt, rx_frame->ack_seb);
#endif /* CONFIG_NET_L2_OPENTHREAD */

			if (net_recv_data(stm32wba_radio->iface, pkt) < 0) {
				LOG_ERR("Packet dropped by NET stack");
				net_pkt_unref(pkt);
			} else {
				if (LOG_LEVEL >= LOG_LEVEL_DBG) {
					log_stack_usage(&stm32wba_radio->rx_thread);
				}
			}
		}
		rx_frame->psdu = NULL;
	}
}

static void stm32wba_802154_receive_failed(stm32wba_802154_ral_rx_error_t error)
{
	const struct device *dev = stm32wba_802154_get_device();
	enum ieee802154_rx_fail_reason reason;

	if (error == STM32WBA_802154_RAL_RX_ERROR_NO_BUFFERS) {
		reason = IEEE802154_RX_FAIL_NOT_RECEIVED;
	} else {
		reason = IEEE802154_RX_FAIL_OTHER;
	}

	if (IS_ENABLED(CONFIG_IEEE802154_STM32WBA_LOG_RX_FAILURES)) {
		LOG_INF("Receive failed, error = %d", error);
	}

	stm32wba_802154_data.last_frame_ack_fpb = false;
	stm32wba_802154_data.last_frame_ack_seb = false;

	if (stm32wba_802154_data.event_handler != NULL) {
		stm32wba_802154_data.event_handler(dev, IEEE802154_EVENT_RX_FAILED, &reason);
	}
}

#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
static int stm32wba_802154_configure_extended(enum ieee802154_stm32wba_config_type type,
					      const struct ieee802154_stm32wba_config *config)
{
	stm32wba_802154_ral_error_t ret;

	switch (type) {
	case IEEE802154_STM32WBA_CONFIG_CCA_THRESHOLD:
		LOG_DBG("Setting CCA_THRESHOLD: %d", config->cca_thr);
		ret = stm32wba_802154_ral_set_cca_energy_detect_threshold(config->cca_thr);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			return -EIO;
		}
		break;

	case IEEE802154_STM32WBA_CONFIG_CONTINUOUS_RECEPTION:
		LOG_DBG("Setting CONTINUOUS_RECEPTION: %u", config->en_cont_rec);
		stm32wba_802154_ral_set_continuous_reception(config->en_cont_rec);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_FRAME_RETRIES:
		LOG_DBG("Setting MAX_FRAME_RETRIES: %u", config->max_frm_retries);
		stm32wba_802154_ral_set_max_frame_retries(config->max_frm_retries);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_CSMA_FRAME_RETRIES:
		LOG_DBG("Setting MAX_CSMA_FRAME_RETRIES: %u", config->max_csma_frm_retries);
		stm32wba_802154_ral_set_max_csma_frame_retries(config->max_csma_frm_retries);
		break;

	case IEEE802154_STM32WBA_CONFIG_MIN_CSMA_BE:
		LOG_DBG("Setting MIN_CSMA_BE: %u", config->min_csma_be);
		stm32wba_802154_ral_set_min_csma_be(config->min_csma_be);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BE:
		LOG_DBG("Setting MAX_CSMA_BE: %u", config->max_csma_be);
		stm32wba_802154_ral_set_max_csma_be(config->max_csma_be);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BACKOFF:
		LOG_DBG("Setting MAX_CSMA_BACKOFF: %u", config->max_csma_backoff);
		stm32wba_802154_ral_set_max_csma_backoff(config->max_csma_backoff);
		break;

	case IEEE802154_STM32WBA_CONFIG_IMPLICIT_BROADCAST:
		LOG_DBG("Setting IMPLICIT_BROADCAST: %u", config->impl_brdcast);
		stm32wba_802154_ral_set_implicitbroadcast(config->impl_brdcast);
		break;

	case IEEE802154_STM32WBA_CONFIG_ANTENNA_DIV:
		LOG_DBG("Setting ANTENNA_DIV: %u", config->ant_div);
		ret = stm32wba_802154_ral_set_ant_div_enable(config->ant_div);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			return -EIO;
		}
		break;

	case IEEE802154_STM32WBA_CONFIG_RADIO_RESET:
		LOG_DBG("Setting RADIO_RESET");
		ret = stm32wba_802154_ral_radio_reset();
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			return -EIO;
		}
		break;

	default:
		LOG_ERR("Unsupported configuration type: %d", type);
		return -EINVAL;
	}

	return 0;
}

static int stm32wba_802154_attr_get_extended(enum ieee802154_stm32wba_attr attr,
					     struct ieee802154_stm32wba_attr_value *value)
{
	stm32wba_802154_ral_error_t ret;

	switch ((uint32_t)attr) {
	case IEEE802154_STM32WBA_ATTR_CCA_THRESHOLD:
		static int8_t l_cca_thr;

		LOG_DBG("Getting CCA_THRESHOLD attribute");
		ret = stm32wba_802154_ral_get_cca_energy_detect_threshold(&l_cca_thr);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			return -ENOENT;
		}
		value->cca_thr = &l_cca_thr;
		break;

	case IEEE802154_STM32WBA_ATTR_IEEE_EUI64:
		uint8_t l_eui64[sizeof(value->eui64)];

		LOG_DBG("Getting IEEE_EUI64 attribute");
		stm32wba_802154_get_eui64(l_eui64);
		memcpy(value->eui64, l_eui64, sizeof(l_eui64));
		break;

	case IEEE802154_STM32WBA_ATTR_TX_POWER:
		static uint8_t l_tx_power;

		LOG_DBG("Getting TX_POWER attribute");
		ret = stm32wba_802154_ral_tx_power_get(&l_tx_power);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			return -ENOENT;
		}
		value->tx_power = &l_tx_power;
		break;

	case IEEE802154_STM32WBA_ATTR_RAND_NUM:
		static uint8_t l_rand_num;

		LOG_DBG("Getting RAND_NUM attribute");
		ret = stm32wba_802154_ral_mac_gen_rnd_num(&l_rand_num, 1, true);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			return -ENOENT;
		}
		value->rand_num = &l_rand_num;
		break;

	default:
		LOG_ERR("Unsupported attribute: %u", attr);
		return -ENOENT;
	}

	return 0;
}
#endif


/* Radio device API */

static enum ieee802154_hw_caps stm32wba_802154_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return IEEE802154_HW_ENERGY_SCAN |
		IEEE802154_HW_FCS |
		IEEE802154_HW_FILTER |
		IEEE802154_HW_PROMISC |
#ifdef CONFIG_IEEE802154_STM32WBA_CSMA_CA_ENABLED
		IEEE802154_HW_CSMA |
#endif
		IEEE802154_HW_TX_RX_ACK |
		IEEE802154_HW_RETRANSMISSION |
		IEEE802154_HW_RX_TX_ACK |
#if (STM32WBA_802154_CSL_TRANSMITTER_ENABLE == 1)
		IEEE802154_HW_TXTIME |
#endif
#if (STM32WBA_802154_CSL_RECEIVER_ENABLE == 1)
		IEEE802154_HW_RXTIME |
#endif
		IEEE802154_HW_SLEEP_TO_TX |
		IEEE802154_RX_ON_WHEN_IDLE;
}

static int stm32wba_802154_cca(const struct device *dev)
{
	struct stm32wba_802154_data_t *stm32wba_radio = dev->data;

	if (stm32wba_802154_ral_cca() != STM32WBA_802154_RAL_ERROR_NONE) {
		LOG_DBG("CCA failed");
		return -EBUSY;
	}

	/* The STM32WBA driver guarantees that a callback will be called once
	 * the CCA function is done, thus unlocking the semaphore.
	 */
	k_sem_take(&stm32wba_radio->cca_wait, K_FOREVER);

	LOG_DBG("Channel free? %d", stm32wba_radio->channel_free);

	return stm32wba_radio->channel_free ? 0 : -EBUSY;
}

static int stm32wba_802154_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);

	if (channel < drv_attr.phy_channel_range.from_channel ||
	    channel > drv_attr.phy_channel_range.to_channel) {
		LOG_ERR("Invalid channel: %u (valid range: %u to %u)", channel,
			drv_attr.phy_channel_range.from_channel,
			drv_attr.phy_channel_range.to_channel);
		return channel < drv_attr.phy_channel_range.from_channel ? -ENOTSUP : -EINVAL;
	}

	LOG_DBG("Setting channel %u", channel);
	stm32wba_802154_ral_set_channel(channel);

	return 0;
}

static int stm32wba_802154_energy_scan_start(const struct device *dev,
					     uint16_t duration,
					     energy_scan_done_cb_t done_cb)
{
	int err = 0;
	stm32wba_802154_ral_error_t ret;

	ARG_UNUSED(dev);

	LOG_DBG("Starting energy scan with duration: %u ms", duration);

	if (stm32wba_802154_data.energy_scan_done_cb == NULL) {
		stm32wba_802154_data.energy_scan_done_cb = done_cb;
		ret = stm32wba_802154_ral_energy_detection(duration);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			LOG_ERR("Energy detection failed, device is busy");
			stm32wba_802154_data.energy_scan_done_cb = NULL;
			err = -EBUSY;
		}
	} else {
		LOG_ERR("Energy scan already in progress");
		err = -EALREADY;
	}

	return err;
}

static int stm32wba_802154_filter(const struct device *dev, bool set,
				  enum ieee802154_filter_type type,
				  const struct ieee802154_filter *filter)
{
	int err = 0;

	if (!set) {
		LOG_ERR("Filter unset, operation is not supported");
		return -ENOTSUP;
	}

	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		LOG_DBG("Setting extended address filter to "
			"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
			filter->ieee_addr[0], filter->ieee_addr[1], filter->ieee_addr[2],
			filter->ieee_addr[3], filter->ieee_addr[4], filter->ieee_addr[5],
			filter->ieee_addr[6], filter->ieee_addr[7]);
		stm32wba_802154_ral_extended_address_set(filter->ieee_addr);
		break;

	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		LOG_DBG("Setting short address filter to 0x%04x", filter->short_addr);
		stm32wba_802154_ral_short_address_set(filter->short_addr);
		break;

	case IEEE802154_FILTER_TYPE_PAN_ID:
		LOG_DBG("Setting PAN ID filter to 0x%04x", filter->pan_id);
		stm32wba_802154_ral_pan_id_set(filter->pan_id);
		break;

	default:
		LOG_ERR("Unsupported filter type: %u", type);
		err = -ENOTSUP;
		break;
	}

	return err;
}

static int stm32wba_802154_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);

	if (!IN_RANGE(dbm, STM32WBA_PWR_MIN, STM32WBA_PWR_MAX)) {
		LOG_ERR("Invalid TX power: %d dBm (valid range: %d to %d dBm)",
			dbm, STM32WBA_PWR_MIN, STM32WBA_PWR_MAX);
		return -EINVAL;
	}

	stm32wba_802154_ral_tx_power_set(dbm);
	stm32wba_802154_data.txpwr = dbm;

	LOG_DBG("Setting TX power to %d dBm", dbm);

	return 0;
}

static int handle_ack(struct stm32wba_802154_data_t *stm32wba_radio)
{
	uint8_t ack_len;
	struct net_pkt *ack_pkt;
	int err = 0;

	if (IS_ENABLED(CONFIG_IEEE802154_L2_PKT_INCL_FCS)) {
		ack_len = stm32wba_radio->ack_frame.length;
	} else {
		ack_len = stm32wba_radio->ack_frame.length - IEEE802154_FCS_LENGTH;
	}

	ack_pkt = net_pkt_rx_alloc_with_buffer(stm32wba_radio->iface, ack_len,
					       AF_UNSPEC, 0, K_NO_WAIT);
	if (ack_pkt == NULL) {
		LOG_ERR("No free packet available.");
		return -ENOMEM;
	}

	if (net_pkt_write(ack_pkt, stm32wba_radio->ack_frame.psdu, ack_len) < 0) {
		LOG_ERR("Failed to write to a packet.");
		err = -ENOMEM;
		goto free_net_ack;
	}

	net_pkt_set_ieee802154_lqi(ack_pkt, stm32wba_radio->ack_frame.lqi);
	net_pkt_set_ieee802154_rssi_dbm(ack_pkt, stm32wba_radio->ack_frame.rssi);

	net_pkt_cursor_init(ack_pkt);

	if (ieee802154_handle_ack(stm32wba_radio->iface, ack_pkt) != NET_OK) {
		LOG_WRN("ACK packet not handled - releasing.");
	} else {
		LOG_DBG("ACK packet received - sequence nb: %u",
			stm32wba_radio->ack_frame.psdu[2]);
	}

free_net_ack:
	net_pkt_unref(ack_pkt);

	return err;
}

static void stm32wba_802154_tx_started(const struct device *dev,
				       struct net_pkt *pkt,
				       struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	if (stm32wba_802154_data.event_handler != NULL) {
		stm32wba_802154_data.event_handler(dev, IEEE802154_EVENT_TX_STARTED, frag);
	}
}

static stm32wba_802154_ral_error_t stm32wba_802154_transmit(struct net_pkt *pkt,
					       uint8_t *payload, uint8_t payload_len, bool cca)
{
	LOG_DBG("TX frame - sequence nb: %u, length: %u", payload[2], payload_len);

	stm32wba_802154_ral_transmit_metadata_t metadata = {
		.is_secured = net_pkt_ieee802154_frame_secured(pkt),
		.dynamic_data_is_set = net_pkt_ieee802154_mac_hdr_rdy(pkt),
		.cca = cca,
		.tx_power = stm32wba_802154_data.txpwr,
		.tx_channel = stm32wba_802154_ral_channel_get()
	};

	return stm32wba_802154_ral_transmit(payload, payload_len, &metadata);
}

static int stm32wba_802154_tx(const struct device *dev,
			      enum ieee802154_tx_mode mode,
			      struct net_pkt *pkt,
			      struct net_buf *frag)
{
	uint8_t payload_len = frag->len + IEEE802154_FCS_LENGTH;
	uint8_t *payload = frag->data;
	stm32wba_802154_ral_error_t err;

	if (payload_len > IEEE802154_MTU + IEEE802154_FCS_LENGTH) {
		LOG_ERR("Payload too large: %d", payload_len);
		return -EMSGSIZE;
	}

	memcpy(stm32wba_802154_data.tx_psdu, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&stm32wba_802154_data.tx_wait);

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
	case IEEE802154_TX_MODE_CCA:
		err = stm32wba_802154_transmit(pkt,
					       stm32wba_802154_data.tx_psdu,
					       payload_len,
					       false);
		break;
#ifdef CONFIG_IEEE802154_STM32WBA_CSMA_CA_ENABLED
	case IEEE802154_TX_MODE_CSMA_CA:
		err = stm32wba_802154_transmit(pkt,
					       stm32wba_802154_data.tx_psdu,
					       payload_len,
					       true);
		break;
#endif
	default:
		LOG_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	if (err != STM32WBA_802154_RAL_ERROR_NONE) {
		LOG_ERR("Cannot send frame");
		return -EIO;
	}

	stm32wba_802154_tx_started(dev, pkt, frag);

	/* Wait for the callback from the radio driver. */
	k_sem_take(&stm32wba_802154_data.tx_wait, K_FOREVER);

	LOG_DBG("Transmit done, result: %d", stm32wba_802154_data.tx_result);

	net_pkt_set_ieee802154_frame_secured(pkt, stm32wba_802154_data.tx_frame_is_secured);
	net_pkt_set_ieee802154_mac_hdr_rdy(pkt, stm32wba_802154_data.tx_frame_mac_hdr_rdy);

	switch (stm32wba_802154_data.tx_result) {
	case STM32WBA_802154_RAL_TX_ERROR_NONE:
		if (stm32wba_802154_data.ack_frame.psdu == NULL) {
			/* No ACK was requested. */
			return 0;
		}
		/* Handle ACK packet. */
		return handle_ack(&stm32wba_802154_data);
	case STM32WBA_802154_RAL_TX_ERROR_NO_MEM:
		return -ENOBUFS;
	case STM32WBA_802154_RAL_TX_ERROR_BUSY_CHANNEL:
		return -EBUSY;
	case STM32WBA_802154_RAL_TX_ERROR_NO_ACK:
		return -ENOMSG;
	case STM32WBA_802154_RAL_TX_ERROR_ABORTED:
	default:
		return -EIO;
	}
}

static net_time_t stm32wba_802154_get_time(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (net_time_t)stm32wba_802154_ral_time_get() * NSEC_PER_USEC;
}

static uint8_t stm32wba_802154_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_STM32WBA_DELAY_TRX_ACC;
}

static int stm32wba_802154_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	stm32wba_802154_ral_tx_power_set(stm32wba_802154_data.txpwr);

	if (stm32wba_802154_ral_receive() != STM32WBA_802154_RAL_ERROR_NONE) {
		LOG_ERR("Failed to enter receive state");
		return -EIO;
	}

	LOG_DBG("802.15.4 radio started on channel: %u",
		stm32wba_802154_ral_channel_get());

	return 0;
}

static int stm32wba_802154_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (stm32wba_802154_ral_sleep() != STM32WBA_802154_RAL_ERROR_NONE) {
		LOG_ERR("Error while stopping radio");
		return -EIO;
	}

	LOG_DBG("802.15.4 radio stopped");

	return 0;
}

static int stm32wba_802154_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_fifo_init(&stm32wba_802154_data.rx_fifo);
	k_sem_init(&stm32wba_802154_data.tx_wait, 0, 1);
	k_sem_init(&stm32wba_802154_data.cca_wait, 0, 1);
#if defined(CONFIG_NET_L2_OPENTHREAD)
	stm32wba_802154_ral_set_config_lib_params(1, 0);
#else
	stm32wba_802154_ral_set_config_lib_params(0, 1);
#endif /* CONFIG_NET_L2_OPENTHREAD */
	stm32wba_802154_ral_init();
	stm32wba_802154_ral_promiscuous_set(false);
#if !defined(CONFIG_NET_L2_CUSTOM_IEEE802154_STM32WBA) && !defined(CONFIG_NET_L2_OPENTHREAD)
	stm32wba_802154_data.rx_on_when_idle = true;
#else
	stm32wba_802154_data.rx_on_when_idle = false;
#endif /* CONFIG_NET_L2_CUSTOM_IEEE802154_STM32WBA && CONFIG_NET_L2_OPENTHREAD */
	stm32wba_802154_ral_set_continuous_reception(stm32wba_802154_data.rx_on_when_idle);

	k_thread_create(&stm32wba_802154_data.rx_thread, stm32wba_802154_data.rx_stack,
			CONFIG_IEEE802154_STM32WBA_RX_STACK_SIZE,
			stm32wba_802154_rx_thread, &stm32wba_802154_data, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_thread_name_set(&stm32wba_802154_data.rx_thread, "stm32wba_rx");

	LOG_DBG("STM32WBA 802.15.4 radio initialized");

	return 0;
}

static void stm32wba_802154_iface_init(struct net_if *iface)
{
	static struct stm32wba_802154_ral_cbk_dispatch_tbl ll_cbk_dispatch_tbl = {
		.stm32wba_802154_ral_cbk_ed_scan_done = stm32wba_802154_energy_scan_done,
		.stm32wba_802154_ral_cbk_tx_done = stm32wba_802154_transmit_done,
		.stm32wba_802154_ral_cbk_rx_done = stm32wba_802154_receive_done,
		.stm32wba_802154_ral_cbk_cca_done = stm32wba_802154_cca_done,
		.stm32wba_802154_ral_cbk_tx_ack_started = stm32wba_802154_tx_ack_started,
	};

	link_layer_register_isr();

#if !defined(CONFIG_NET_L2_CUSTOM_IEEE802154_STM32WBA)
	ll_sys_thread_init();
	/* Intentionally discard the return values */
	(void)stm32wba_802154_ral_set_max_csma_backoff(MAX_CSMA_BACKOFF);
	(void)stm32wba_802154_ral_set_max_frame_retries(MAX_FRAME_RETRY);
	(void)stm32wba_802154_ral_set_cca_energy_detect_threshold(CCA_THRESHOLD);
#endif
	stm32wba_802154_ral_call_back_funcs_init(&ll_cbk_dispatch_tbl);

	stm32wba_802154_get_eui64(stm32wba_802154_data.mac);
	net_if_set_link_addr(iface, stm32wba_802154_data.mac,
				sizeof(stm32wba_802154_data.mac), NET_LINK_IEEE802154);

	stm32wba_802154_data.iface = iface;

	ieee802154_init(iface);

#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154_STM32WBA)
	ll_sys_mac_cntrl_init();
#endif
}

static int stm32wba_802154_set_ack_fpb(const struct ieee802154_config *config)
{
	int err = 0;
	stm32wba_802154_ral_error_t ret;

	if (config->ack_fpb.extended) {
		ret = stm32wba_802154_ral_pending_bit_for_ext_addr_set(
							(const uint8_t *)config->ack_fpb.addr);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			LOG_ERR("Failed to set ACK_FPB for extended address: "
				"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
			err = -ENOMEM;
		} else {
			LOG_DBG("Set ACK_FPB for extended address: "
				"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
		}
	} else {
		uint16_t short_addr = (uint16_t)config->ack_fpb.addr[0] |
				      (uint16_t)(config->ack_fpb.addr[1] << 8);

		ret = stm32wba_802154_ral_pending_bit_for_short_addr_set(short_addr);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			LOG_ERR("Failed to set ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
			err = -ENOMEM;
		} else {
			LOG_DBG("Set ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
		}
	}
	return err;
}

static int stm32wba_802154_clear_ack_fpb(const struct ieee802154_config *config)
{
	int err = 0;
	stm32wba_802154_ral_error_t ret;

	if (config->ack_fpb.extended) {
		ret = stm32wba_802154_ral_pending_bit_for_ext_addr_clear(
						(const uint8_t *)config->ack_fpb.addr);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			LOG_ERR("Failed to clear ACK_FPB for extended address: "
				"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
			err = -ENOENT;
		} else {
			LOG_DBG("Clear ACK_FPB for extended address: "
				"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
		}
	} else {
		uint16_t short_addr = (uint16_t)config->ack_fpb.addr[0] |
				     (uint16_t)(config->ack_fpb.addr[1] << 8);

		ret = stm32wba_802154_ral_pending_bit_for_short_addr_clear(short_addr);
		if (ret != STM32WBA_802154_RAL_ERROR_NONE) {
			LOG_ERR("Failed to clear ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
			err = -ENOENT;
		} else {
			LOG_DBG("Clear ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
		}
	}
	return err;
}

static int stm32wba_802154_clear_all_ack_fpb(const struct ieee802154_config *config)
{
	if (config->ack_fpb.extended) {
		stm32wba_802154_ral_pending_bit_for_ext_addr_reset();
		LOG_DBG("Clear ACK_FPB for all extended addresses");
	} else {
		stm32wba_802154_ral_pending_bit_for_short_addr_reset();
		LOG_DBG("Clear ACK_FPB for all short addresses");
	}
	return 0;
}

static int stm32wba_802154_configure_ack_fpb(const struct ieee802154_config *config)
{
	int ret;

	if (config->ack_fpb.enabled) {
		/* Set ACK pending bit for an addr (short or extended) */
		ret = stm32wba_802154_set_ack_fpb(config);
	} else if (config->ack_fpb.addr != NULL) {
		/* Clear ACK pending bit for an addr (short or extended) */
		ret = stm32wba_802154_clear_ack_fpb(config);
	} else {
		/* Clear all ACK pending bit for addr (short or extended) */
		ret = stm32wba_802154_clear_all_ack_fpb(config);
	}

	return ret;
}

static int stm32wba_802154_configure(const struct device *dev,
				     enum ieee802154_config_type type,
				     const struct ieee802154_config *config)
{
	int ret = 0;

	ARG_UNUSED(dev);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		LOG_DBG("Setting AUTO_ACK_FPB: enabled = %d", config->auto_ack_fpb.enabled);
		stm32wba_802154_ral_auto_pending_bit_set(config->auto_ack_fpb.enabled);
		break;

	case IEEE802154_CONFIG_ACK_FPB:
		ret = stm32wba_802154_configure_ack_fpb(config);
		break;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		LOG_DBG("Setting PAN_COORDINATOR: %d", config->pan_coordinator);
		stm32wba_802154_ral_pan_coord_set(config->pan_coordinator);
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		LOG_DBG("Setting PROMISCUOUS mode: %d", config->promiscuous);
		stm32wba_802154_ral_promiscuous_set(config->promiscuous);
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		LOG_DBG("Setting EVENT_HANDLER");
		stm32wba_802154_data.event_handler = config->event_handler;
		break;

	case IEEE802154_CONFIG_RX_ON_WHEN_IDLE:
		stm32wba_802154_data.rx_on_when_idle = config->rx_on_when_idle;
		stm32wba_802154_ral_set_continuous_reception(config->rx_on_when_idle);
		break;

	default:
#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
		ret = stm32wba_802154_configure_extended(
					(enum ieee802154_stm32wba_config_type)type,
					(const struct ieee802154_stm32wba_config *)config);
#else
		LOG_ERR("Unsupported configuration type: %d", type);
		ret = -EINVAL;
#endif
	}

	return ret;
}

static int stm32wba_802154_attr_get(const struct device *dev,
				    enum ieee802154_attr attr,
				    struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	if (ieee802154_attr_get_channel_page_and_range(
			attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
			&drv_attr.phy_supported_channels, value) == 0) {
		LOG_DBG("Attribute successfully retrieved for channel page and range");
		return 0;
	}

#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
	return stm32wba_802154_attr_get_extended((enum ieee802154_stm32wba_attr)attr,
					  (struct ieee802154_stm32wba_attr_value *)value);
#else
	LOG_ERR("Unsupported attribute: %u", attr);
	return -ENOENT;
#endif
}

/* WBA radio driver callbacks */

static void stm32wba_802154_receive_done(uint8_t *p_buffer,
					 stm32wba_802154_ral_receive_done_metadata_t *p_metadata)
{
	if (p_buffer == NULL) {
		stm32wba_802154_receive_failed(p_metadata->error);
		return;
	}

	for (uint32_t i = 0; i < ARRAY_SIZE(stm32wba_802154_data.rx_frames); i++) {
		if (stm32wba_802154_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		stm32wba_802154_data.rx_frames[i].psdu = p_buffer;
		stm32wba_802154_data.rx_frames[i].length = p_metadata->length;
		stm32wba_802154_data.rx_frames[i].rssi = p_metadata->power;
		stm32wba_802154_data.rx_frames[i].lqi = p_metadata->lqi;
		stm32wba_802154_data.rx_frames[i].time = p_metadata->time;
		stm32wba_802154_data.rx_frames[i].ack_fpb =
							stm32wba_802154_data.last_frame_ack_fpb;
		stm32wba_802154_data.rx_frames[i].ack_seb =
							stm32wba_802154_data.last_frame_ack_seb;
		stm32wba_802154_data.last_frame_ack_fpb = false;
		stm32wba_802154_data.last_frame_ack_seb = false;

		k_fifo_put(&stm32wba_802154_data.rx_fifo, &stm32wba_802154_data.rx_frames[i]);

		return;
	}

	LOG_ERR("Not enough RX frames allocated for 802.15.4 driver");
}

void stm32wba_802154_tx_ack_started(bool ack_fpb, bool ack_seb)
{
	stm32wba_802154_data.last_frame_ack_fpb = ack_fpb;
	stm32wba_802154_data.last_frame_ack_seb = ack_seb;
}

static void stm32wba_802154_transmit_done(
				uint8_t *p_frame,
				stm32wba_802154_ral_tx_error_t error,
				const stm32wba_802154_ral_transmit_done_metadata_t *p_metadata)
{
	ARG_UNUSED(p_frame);

	stm32wba_802154_data.tx_result = error;
	stm32wba_802154_data.tx_frame_is_secured = p_metadata->is_secured;
	stm32wba_802154_data.tx_frame_mac_hdr_rdy = p_metadata->dynamic_data_is_set;
	stm32wba_802154_data.ack_frame.length = p_metadata->length;

	if (stm32wba_802154_data.ack_frame.length != 0) {
		stm32wba_802154_data.ack_frame.psdu = p_metadata->p_ack;
		stm32wba_802154_data.ack_frame.rssi = p_metadata->power;
		stm32wba_802154_data.ack_frame.lqi = p_metadata->lqi;
	} else {
		stm32wba_802154_data.ack_frame.psdu = NULL;
		stm32wba_802154_data.ack_frame.rssi = 0;
		stm32wba_802154_data.ack_frame.lqi = 0;
	}

	k_sem_give(&stm32wba_802154_data.tx_wait);
}

static void stm32wba_802154_cca_done(uint8_t error)
{
	if (error == STM32WBA_802154_RAL_RX_ERROR_NONE) {
		stm32wba_802154_data.channel_free = true;
	}

	k_sem_give(&stm32wba_802154_data.cca_wait);
}

static void stm32wba_802154_energy_scan_done(int8_t rssi_result)
{
	if (stm32wba_802154_data.energy_scan_done_cb != NULL) {
		energy_scan_done_cb_t callback = stm32wba_802154_data.energy_scan_done_cb;

		stm32wba_802154_data.energy_scan_done_cb = NULL;
		callback(stm32wba_802154_get_device(), rssi_result);
	}
}

#ifdef CONFIG_PM_DEVICE
static int radio_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_RADIO);
#if defined(CONFIG_PM_S2RAM)
		if (LL_PWR_IsActiveFlag_SB() == 1U) {
			/* Put the radio in active state */
			LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_RADIO);
			link_layer_register_isr();
		}
		LINKLAYER_PLAT_NotifyWFIExit();
		ll_sys_dp_slp_exit();
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
#if defined(CONFIG_PM_S2RAM)
		uint64_t next_radio_evt;
		enum pm_state state = pm_state_next_get(_current_cpu->id)->state;

		if (state == PM_STATE_SUSPEND_TO_RAM) {
			next_radio_evt = os_timer_get_earliest_time();
			if (llhwc_cmn_is_dp_slp_enabled() == 0) {
				if (next_radio_evt > CFG_LPM_STDBY_WAKEUP_TIME) {
					/* No event in a "near" futur */
					next_radio_evt -= CFG_LPM_STDBY_WAKEUP_TIME;
					ll_sys_dp_slp_enter(next_radio_evt);
				}
			}
		}
#endif
		LINKLAYER_PLAT_NotifyWFIEnter();
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

PM_DEVICE_DT_INST_DEFINE(0, radio_pm_action);
#endif /* CONFIG_PM_DEVICE */

static const struct ieee802154_radio_api stm32wba_802154_radio_api = {
	.iface_api.init = stm32wba_802154_iface_init,
	.get_capabilities = stm32wba_802154_get_capabilities,
	.cca = stm32wba_802154_cca,
	.set_channel = stm32wba_802154_set_channel,
	.filter = stm32wba_802154_filter,
	.set_txpower = stm32wba_802154_set_txpower,
	.start = stm32wba_802154_start,
	.stop = stm32wba_802154_stop,
	.tx = stm32wba_802154_tx,
	.ed_scan = stm32wba_802154_energy_scan_start,
	.get_time = stm32wba_802154_get_time,
	.get_sch_acc = stm32wba_802154_get_acc,
	.configure = stm32wba_802154_configure,
	.attr_get = stm32wba_802154_attr_get,
};

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2		IEEE802154_L2
#define L2_CTX_TYPE	NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU		IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2		OPENTHREAD_L2
#define L2_CTX_TYPE	NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU		1280
#elif defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
#define L2		CUSTOM_IEEE802154_L2
#define L2_CTX_TYPE	NET_L2_GET_CTX_TYPE(CUSTOM_IEEE802154_L2)
#define MTU		CONFIG_NET_L2_CUSTOM_IEEE802154_MTU
#endif

#if defined(CONFIG_NET_L2_PHY_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0,
			  stm32wba_802154_driver_init,
			  PM_DEVICE_DT_INST_GET(0),
			  &stm32wba_802154_data,
			  NULL,
			  CONFIG_IEEE802154_STM32WBA_INIT_PRIO,
			  &stm32wba_802154_radio_api,
			  L2,
			  L2_CTX_TYPE,
			  MTU);
#else
DEVICE_DT_INST_DEFINE(0,
		      stm32wba_802154_driver_init,
		      PM_DEVICE_DT_INST_GET(0),
		      &stm32wba_802154_data,
		      NULL,
		      POST_KERNEL,
		      CONFIG_IEEE802154_STM32WBA_INIT_PRIO,
		      &stm32wba_802154_radio_api);
#endif
