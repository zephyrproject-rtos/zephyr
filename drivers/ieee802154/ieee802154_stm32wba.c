/* ieee802154_stm32wba.c - STM32WBxx 802.15.4 driver */

/*
 * Copyright (c) 2025 STMicroelectronics
 *
 */

#define DT_DRV_COMPAT st_stm32wba_ieee802154

#define LOG_MODULE_NAME ieee802154_stm32wba
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/debug/stack.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>
#include <zephyr/irq.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio_openthread.h>
#endif

#include <soc.h>
#include <errno.h>
#include <string.h>

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
#include <ll_sys.h>
#include <ll_sys_startup.h>
#include "ieee802154_stm32wba.h"
#include "ral.h"
#include <stm32wba_802154.h>
#include <stm32wba_802154_callbacks.h>

extern uint32_t llhwc_cmn_is_dp_slp_enabled(void);

struct stm32wba_802154_config {
	void (*irq_config_func)(const struct device *dev);
};

static struct stm32wba_802154_data stm32wba_data;

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

#define NSEC_PER_TEN_SYMBOLS (10 * IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS)

/* Convenience defines for RADIO */
#define STM32WBA_802154_DATA(dev) ((struct stm32wba_802154_data * const)(dev)->data)

#if CONFIG_IEEE802154_VENDOR_OUI_ENABLE
#define IEEE802154_STM32WBA_VENDOR_OUI	CONFIG_IEEE802154_VENDOR_OUI
#else
#define IEEE802154_STM32WBA_VENDOR_OUI	(uint32_t)0x80E105
#endif

#define MAX_CSMA_BACKOFF 4
#define MAX_FRAME_RETRY  3
#define CCA_THRESHOLD    (-70)

static inline const struct device *stm32wba_get_device(void)
{
	LOG_DBG("Getting device instance");

	return net_if_get_device(stm32wba_data.iface);
}

static void stm32wba_get_eui64(uint8_t *mac)
{
	st_ral_eui64_get(mac);

	LOG_DBG("Device EUI64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
}

static void stm32wba_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct stm32wba_802154_data *stm32wba_radio = (struct stm32wba_802154_data *)arg1;
	struct net_pkt *pkt;
	struct stm32wba_802154_rx_frame *rx_frame;
	uint8_t pkt_len;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("RX thread started");

	while (1) {
		pkt = NULL;
		rx_frame = NULL;

		rx_frame = k_fifo_get(&stm32wba_radio->rx_fifo, K_FOREVER);

		__ASSERT_NO_MSG(rx_frame->psdu);

		/* Depending on the net L2 layer, the FCS may be included in length or not */
		if (IS_ENABLED(CONFIG_IEEE802154_STM32WBA_FCS_IN_LENGTH)) {
			pkt_len = rx_frame->length;
		} else {
			pkt_len = rx_frame->length -  IEEE802154_FCS_LENGTH;
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

		if (net_pkt_write(pkt, rx_frame->psdu, pkt_len)) {
			LOG_ERR("Failed to write packet data");
			goto drop;
		}

		net_pkt_set_ieee802154_lqi(pkt, rx_frame->lqi);
		net_pkt_set_ieee802154_rssi_dbm(pkt, rx_frame->rssi);
		net_pkt_set_ieee802154_ack_fpb(pkt, rx_frame->ack_fpb);

#if defined(CONFIG_NET_L2_OPENTHREAD)
		net_pkt_set_ieee802154_ack_seb(pkt, rx_frame->ack_seb);
#endif

		if (net_recv_data(stm32wba_radio->iface, pkt) < 0) {
			LOG_ERR("Packet dropped by NET stack");
			goto drop;
		}

		rx_frame->psdu = NULL;

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			log_stack_usage(&stm32wba_radio->rx_thread);
		}

		continue;

drop:
		rx_frame->psdu = NULL;

		net_pkt_unref(pkt);
	}
}

static void stm32wba_get_capabilities_at_boot(void)
{
	LOG_DBG("Fetching capabilities at boot");

	stm32wba_data.capabilities =
		IEEE802154_HW_ENERGY_SCAN |
		IEEE802154_HW_FCS |
		IEEE802154_HW_FILTER |
		IEEE802154_HW_PROMISC |
#if CONFIG_IEEE802154_STM32WBA_CSMA_CA_ENABLED
		IEEE802154_HW_CSMA |
#endif
		IEEE802154_HW_TX_RX_ACK |
		IEEE802154_HW_RETRANSMISSION |
		IEEE802154_HW_RX_TX_ACK |
#if (CONFIG_MAC_CSL_TRANSMITTER_ENABLE == 1)
		IEEE802154_HW_TXTIME |
#endif
#if (CONFIG_MAC_CSL_RECEIVER_ENABLE == 1)
		IEEE802154_HW_RXTIME |
#endif
		IEEE802154_HW_SLEEP_TO_TX |
		IEEE802154_RX_ON_WHEN_IDLE
		;
}

static void stm32wba_receive_failed(stm32wba_802154_rx_error_t error)
{
	const struct device *dev = stm32wba_get_device();

	enum ieee802154_rx_fail_reason reason;

	if (error == STM32WBA_802154_RX_ERROR_NO_BUFFERS) {
		reason = IEEE802154_RX_FAIL_NOT_RECEIVED;
	} else {
		reason = IEEE802154_RX_FAIL_OTHER;
	}

	if (IS_ENABLED(CONFIG_IEEE802154_STM32WBA_LOG_RX_FAILURES)) {
		LOG_INF("Receive failed, error = %d", error);
	}

	stm32wba_data.last_frame_ack_fpb = false;
	stm32wba_data.last_frame_ack_seb = false;

	if (stm32wba_data.event_handler) {
		stm32wba_data.event_handler(dev, IEEE802154_EVENT_RX_FAILED, (void *)&reason);
	}
}

#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
static int stm32wba_configure_extended(enum ieee802154_stm32wba_config_type type,
				       const struct ieee802154_stm32wba_config *config)
{
	switch (type) {
	case IEEE802154_STM32WBA_CONFIG_CCA_THRESHOLD:
		LOG_DBG("Setting CCA_THRESHOLD: %d", config->cca_thr);
		if (st_ral_set_cca_energy_detect_threshold(config->cca_thr)) {
			return -EIO;
		}
		break;

	case IEEE802154_STM32WBA_CONFIG_CONTINUOUS_RECEPTION:
		LOG_DBG("Setting CONTINUOUS_RECEPTION: %u", config->en_cont_rec);
		st_ral_set_continous_reception(config->en_cont_rec);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_FRAME_RETRIES:
		LOG_DBG("Setting MAX_FRAME_RETRIES: %u", config->max_frm_retries);
		st_ral_set_max_frame_retries(config->max_frm_retries);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_CSMA_FRAME_RETRIES:
		LOG_DBG("Setting MAX_CSMA_FRAME_RETRIES: %u", config->max_csma_frm_retries);
		st_ral_set_max_csma_frame_retries(config->max_csma_frm_retries);
		break;

	case IEEE802154_STM32WBA_CONFIG_MIN_CSMA_BE:
		LOG_DBG("Setting MIN_CSMA_BE: %u", config->min_csma_be);
		st_ral_set_min_csma_be(config->min_csma_be);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BE:
		LOG_DBG("Setting MAX_CSMA_BE: %u", config->max_csma_be);
		st_ral_set_max_csma_be(config->max_csma_be);
		break;

	case IEEE802154_STM32WBA_CONFIG_MAX_CSMA_BACKOFF:
		LOG_DBG("Setting MAX_CSMA_BACKOFF: %u", config->max_csma_backoff);
		st_ral_set_max_csma_backoff(config->max_csma_backoff);
		break;

	case IEEE802154_STM32WBA_CONFIG_IMPLICIT_BROADCAST:
		LOG_DBG("Setting IMPLICIT_BROADCAST: %u", config->impl_brdcast);
		st_ral_set_implicitbroadcast(config->impl_brdcast);
		break;

	case IEEE802154_STM32WBA_CONFIG_ANTENNA_DIV:
		LOG_DBG("Setting ANTENNA_DIV: %u", config->ant_div);
		if (st_ral_set_ant_div_enable(config->ant_div)) {
			return -EIO;
		}
		break;

	case IEEE802154_STM32WBA_CONFIG_RADIO_RESET:
		LOG_DBG("Setting RADIO_RESET");
		if (st_ral_radio_reset()) {
			return -EIO;
		}
		break;

	default:
		LOG_ERR("Unsupported configuration type: %d", type);
		return -EINVAL;
	}

	return 0;
}

static int stm32wba_attr_get_extended(enum ieee802154_stm32wba_attr attr,
				      const struct ieee802154_stm32wba_attr_value *value)
{
	switch ((uint32_t)attr) {
	case IEEE802154_STM32WBA_ATTR_CCA_THRESHOLD:
		LOG_DBG("Getting CCA_THRESHOLD attribute");
		static int8_t l_cca_thr;

		if (st_ral_get_cca_energy_detect_threshold(&l_cca_thr)) {
			return -ENOENT;
		}
		((struct ieee802154_stm32wba_attr_value *)value)->cca_thr = &l_cca_thr;
		break;

	case IEEE802154_STM32WBA_ATTR_IEEE_EUI64:
		LOG_DBG("Getting IEEE_EUI64 attribute");
		uint8_t l_eui64[8];

		stm32wba_get_eui64(l_eui64);
		memcpy(((struct ieee802154_stm32wba_attr_value *)value)->eui64, l_eui64,
				sizeof(l_eui64));
		break;

	case IEEE802154_STM32WBA_ATTR_TX_POWER:
		LOG_DBG("Getting TX_POWER attribute");
		static uint8_t l_tx_power;

		if (st_ral_tx_power_get(&l_tx_power)) {
			return -ENOENT;
		}
		((struct ieee802154_stm32wba_attr_value *)value)->tx_power = &l_tx_power;
		break;

	case IEEE802154_STM32WBA_ATTR_RAND_NUM:
		LOG_DBG("Getting RAND_NUM attribute");
		static uint8_t l_rand_num;

		if (st_ral_mac_gen_rnd_num(&l_rand_num, 1, true)) {
			return -ENOENT;
		}
		((struct ieee802154_stm32wba_attr_value *)value)->rand_num = &l_rand_num;
		break;

	default:
		LOG_ERR("Unsupported attribute: %u", (uint32_t)attr);
		return -ENOENT;
	}

	return 0;
}
#endif

/* Radio device API */

static enum ieee802154_hw_caps stm32wba_get_capabilities(const struct device *dev)
{
	return stm32wba_data.capabilities;
}

static int stm32wba_cca(const struct device *dev)
{
	struct stm32wba_802154_data *stm32wba_radio = STM32WBA_802154_DATA(dev);

	if (st_ral_cca() != 0) {
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

static int stm32wba_set_channel(const struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);

	if (channel < drv_attr.phy_channel_range.from_channel
	 || channel > drv_attr.phy_channel_range.to_channel) {
		LOG_ERR("Invalid channel: %u (valid range: %u to %u)", channel,
			drv_attr.phy_channel_range.from_channel,
			drv_attr.phy_channel_range.to_channel);
		return channel < drv_attr.phy_channel_range.from_channel ? -ENOTSUP : -EINVAL;
	}

	LOG_DBG("Setting channel %u", channel);
	st_ral_set_channel(channel);

	return 0;
}

static int stm32wba_energy_scan_start(const struct device *dev,
				      uint16_t duration,
				      energy_scan_done_cb_t done_cb)
{
	int err = 0;

	ARG_UNUSED(dev);

	LOG_DBG("Starting energy scan with duration: %u ms", duration);

	if (stm32wba_data.energy_scan_done == NULL) {
		stm32wba_data.energy_scan_done = done_cb;

		if (st_ral_energy_detection(duration) != false) {
			LOG_ERR("Energy detection failed, device is busy");
			stm32wba_data.energy_scan_done = NULL;
			err = -EBUSY;
		}
	} else {
		LOG_ERR("Energy scan already in progress");
		err = -EALREADY;
	}

	return err;
}

static int stm32wba_filter(const struct device *dev, bool set,
			   enum ieee802154_filter_type type,
			   const struct ieee802154_filter *filter)
{
	if (!set) {
		LOG_ERR("Filter unset, operation is not supported");
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		LOG_DBG("Setting extended address filter to \
			%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
			filter->ieee_addr[0], filter->ieee_addr[1], filter->ieee_addr[2],
			filter->ieee_addr[3], filter->ieee_addr[4], filter->ieee_addr[5],
			filter->ieee_addr[6], filter->ieee_addr[7]);
		st_ral_extended_address_set(filter->ieee_addr);
		return 0;
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		LOG_DBG("Setting short address filter to 0x%04x", filter->short_addr);
		st_ral_short_address_set(filter->short_addr);
		return 0;
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		LOG_DBG("Setting PAN ID filter to 0x%04x", filter->pan_id);
		st_ral_pan_id_set(filter->pan_id);
		return 0;
	} else {
		LOG_ERR("Unsupported filter type: %u", type);
		return -ENOTSUP;
	}
}

static int stm32wba_set_txpower(const struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);

	if (dbm < STM32WBA_PWR_MIN || dbm > STM32WBA_PWR_MAX) {
		LOG_ERR("Invalid TX power: %d dBm (valid range: %d to %d dBm)",
			dbm, STM32WBA_PWR_MIN, STM32WBA_PWR_MAX);
		return -EINVAL;
	}

	st_ral_tx_power_set(dbm);
	stm32wba_data.txpwr = dbm;

	LOG_DBG("Setting TX power to %d dBm", dbm);

	return 0;
}

static int handle_ack(struct stm32wba_802154_data *stm32wba_radio)
{
	uint8_t ack_len;
	struct net_pkt *ack_pkt;
	int err = 0;

	if (IS_ENABLED(CONFIG_IEEE802154_STM32WBA_FCS_IN_LENGTH)) {
		ack_len = stm32wba_radio->ack_frame.length;
	} else {
		ack_len = stm32wba_radio->ack_frame.length - IEEE802154_FCS_LENGTH;
	}

	ack_pkt = net_pkt_rx_alloc_with_buffer(stm32wba_radio->iface, ack_len,
					       AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		err = -ENOMEM;
		return err;
	}

	/* Upper layers expect the frame to start at the MAC header, skip the
	 * PHY header (1 byte).
	 */
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

static void stm32wba_tx_started(const struct device *dev,
				struct net_pkt *pkt,
				struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	if (stm32wba_data.event_handler) {
		stm32wba_data.event_handler(dev, IEEE802154_EVENT_TX_STARTED, (void *)frag);
	}
}

static bool stm32wba_transmit(struct net_pkt *pkt, uint8_t *payload, uint8_t payload_len, bool cca)
{
	LOG_DBG("TX frame - sequence nb: %u, length: %u", payload[2], payload_len);

	stm32wba_802154_transmit_metadata_t metadata = {
		.is_secured = net_pkt_ieee802154_frame_secured(pkt),
		.dynamic_data_is_set = net_pkt_ieee802154_mac_hdr_rdy(pkt),
		.cca = cca,
		.tx_power = stm32wba_data.txpwr,
		.tx_channel = st_ral_channel_get()
	};

	return st_ral_transmit(payload, payload_len, &metadata);
}

static int stm32wba_tx(const struct device *dev,
		       enum ieee802154_tx_mode mode,
		       struct net_pkt *pkt,
		       struct net_buf *frag)
{
	struct stm32wba_802154_data *stm32wba_radio = STM32WBA_802154_DATA(dev);
	uint8_t payload_len = frag->len + IEEE802154_FCS_LENGTH;
	uint8_t *payload = frag->data;
	uint8_t err = STM32WBA_802154_TX_ERROR_NONE;

	if (payload_len > IEEE802154_MTU + IEEE802154_FCS_LENGTH) {
		LOG_ERR("Payload too large: %d", payload_len);
		return -EMSGSIZE;
	}

	memcpy(stm32wba_radio->tx_psdu, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&stm32wba_radio->tx_wait);

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
	case IEEE802154_TX_MODE_CCA:
		err = stm32wba_transmit(pkt, stm32wba_radio->tx_psdu, payload_len, false);
		break;
#if CONFIG_IEEE802154_STM32WBA_CSMA_CA_ENABLED
	case IEEE802154_TX_MODE_CSMA_CA:
		err = stm32wba_transmit(pkt, stm32wba_radio->tx_psdu, payload_len, true);
		break;
#endif
	default:
		LOG_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	if (err != STM32WBA_802154_TX_ERROR_NONE) {
		LOG_ERR("Cannot send frame, error = %d", err);
		return -EIO;
	}

	stm32wba_tx_started(dev, pkt, frag);

	/* Wait for the callback from the radio driver. */
	k_sem_take(&stm32wba_radio->tx_wait, K_FOREVER);

	LOG_DBG("Transmit done, result: %d", stm32wba_data.tx_result);

	net_pkt_set_ieee802154_frame_secured(pkt, stm32wba_radio->tx_frame_is_secured);
	net_pkt_set_ieee802154_mac_hdr_rdy(pkt, stm32wba_radio->tx_frame_mac_hdr_rdy);

	switch (stm32wba_radio->tx_result) {
	case STM32WBA_802154_TX_ERROR_NONE:
		if (stm32wba_radio->ack_frame.psdu == NULL) {
			/* No ACK was requested. */
			return 0;
		}
		/* Handle ACK packet. */
		return handle_ack(stm32wba_radio);
	case STM32WBA_802154_TX_ERROR_NO_MEM:
		return -ENOBUFS;
	case STM32WBA_802154_TX_ERROR_BUSY_CHANNEL:
		return -EBUSY;
	case STM32WBA_802154_TX_ERROR_NO_ACK:
		return -ENOMSG;
	case STM32WBA_802154_TX_ERROR_ABORTED:
	default:
		return -EIO;
	}
}

static net_time_t stm32wba_get_time(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (net_time_t)st_ral_time_get() * NSEC_PER_USEC;
}

static uint8_t stm32wba_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_STM32WBA_DELAY_TRX_ACC;
}

static int stm32wba_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	st_ral_tx_power_set(stm32wba_data.txpwr);

	if (!st_ral_receive()) {
		LOG_ERR("Failed to enter receive state");
		return -EIO;
	}

	LOG_DBG("802.15.4 radio started on channel: %u",
		st_ral_channel_get());

	return 0;
}

static int stm32wba_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (st_ral_sleep()) {
		LOG_ERR("Error while stopping radio");
		return -EIO;
	}

	LOG_DBG("802.15.4 radio stopped");

	return 0;
}

static void stm32wba_irq_config(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int stm32wba_driver_init(const struct device *dev)
{
	struct stm32wba_802154_data *stm32wba_radio = STM32WBA_802154_DATA(dev);

	k_fifo_init(&stm32wba_radio->rx_fifo);
	k_sem_init(&stm32wba_radio->tx_wait, 0, 1);
	k_sem_init(&stm32wba_radio->cca_wait, 0, 1);

	st_ral_init();
	st_ral_promiscuous_set(false);

	stm32wba_get_capabilities_at_boot();

	stm32wba_radio->rx_on_when_idle = true;

	k_thread_create(&stm32wba_radio->rx_thread, stm32wba_radio->rx_stack,
			CONFIG_IEEE802154_STM32WBA_RX_STACK_SIZE,
			stm32wba_rx_thread, stm32wba_radio, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_thread_name_set(&stm32wba_radio->rx_thread, "stm32wba_rx");

	LOG_DBG("STM32WBA 802.15.4 radio initialized");

	return 0;
}

static void stm32wba_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct stm32wba_802154_data *stm32wba_radio = STM32WBA_802154_DATA(dev);
	static struct ll_mac_cbk_dispatch_tbl ll_cbk_dispatch_tbl = {
		.mac_ed_scan_done = stm32wba_802154_energy_scan_done,
		.mac_tx_done = stm32wba_802154_transmit_done,
		.mac_rx_done = stm32wba_802154_receive_done,
	};

	link_layer_register_isr();
#if defined(NET_L2_CUSTOM_IEEE802154_STM32WBA)
	ll_sys_mac_cntrl_init();
#else
	ll_sys_thread_init();

	st_ral_set_max_csma_backoff(MAX_CSMA_BACKOFF);
	st_ral_set_max_frame_retries(MAX_FRAME_RETRY);
	st_ral_set_cca_energy_detect_threshold(CCA_THRESHOLD);
#endif
	st_ral_call_back_funcs_init(&ll_cbk_dispatch_tbl);

	stm32wba_get_eui64(stm32wba_radio->mac);
	net_if_set_link_addr(iface, stm32wba_radio->mac, sizeof(stm32wba_radio->mac),
			     NET_LINK_IEEE802154);

	stm32wba_radio->iface = iface;

	ieee802154_init(iface);
}

static int stm32wba_set_ack_fpb(const struct ieee802154_config *config)
{
	if (config->ack_fpb.extended) {
		if (st_ral_pending_bit_for_ext_addr_set(
			(const uint8_t *)config->ack_fpb.addr)) {
			LOG_ERR("Failed to set ACK_FPB for extended address: \
				%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
			return -ENOMEM;
		} else {
			LOG_DBG("Set ACK_FPB for extended address: \
				%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
		}
	} else {
		uint16_t shortAddr = (uint16_t)config->ack_fpb.addr[0] |
						(uint16_t)(config->ack_fpb.addr[1] << 8);
		if (st_ral_pending_bit_for_short_addr_set(
			(const uint16_t)shortAddr)) {
			LOG_ERR("Failed to set ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
			return -ENOMEM;
		} else {
			LOG_DBG("Set ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
		}
	}
	return 0;
}

static int stm32wba_clear_ack_fpb(const struct ieee802154_config *config)
{
	if (config->ack_fpb.extended) {
		if (st_ral_pending_bit_for_ext_addr_clear(
			(const uint8_t *)config->ack_fpb.addr)) {
			LOG_ERR("Failed to clear ACK_FPB for extended address: \
				%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
			return -ENOENT;
		} else {
			LOG_DBG("Clear ACK_FPB for extended address: \
				%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				config->ack_fpb.addr[0], config->ack_fpb.addr[1],
				config->ack_fpb.addr[2], config->ack_fpb.addr[3],
				config->ack_fpb.addr[4], config->ack_fpb.addr[5],
				config->ack_fpb.addr[6], config->ack_fpb.addr[7]);
		}
	} else {
		uint16_t shortAddr = (uint16_t)config->ack_fpb.addr[0] |
						(uint16_t)(config->ack_fpb.addr[1] << 8);
		if (st_ral_pending_bit_for_short_addr_clear(
			(const uint16_t)shortAddr)) {
			LOG_ERR("Failed to clear ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
			return -ENOENT;
		} else {
			LOG_DBG("Clear ACK_FPB for short address: 0x%02X%02X",
				config->ack_fpb.addr[1],
				config->ack_fpb.addr[0]);
		}
	}
	return 0;
}

static int stm32wba_clear_all_ack_fpb(const struct ieee802154_config *config)
{
	if (config->ack_fpb.extended) {
		st_ral_pending_bit_for_ext_addr_reset();
		LOG_DBG("Clear ACK_FPB for all extended addresses");
	} else {
		st_ral_pending_bit_for_short_addr_reset();
		LOG_DBG("Clear ACK_FPB for all short addresses");
	}
	return 0;
}

static int stm32wba_configure_ack_fpb(const struct ieee802154_config *config)
{
	/* Set ACK pending bit for an addr (short or extended) */
	if (config->ack_fpb.enabled) {
		return stm32wba_set_ack_fpb(config);
	}
	/* Clear ACK pending bit for an addr (short or extended) */
	else if (config->ack_fpb.addr != NULL) {
		return stm32wba_clear_ack_fpb(config);
	}
	/* Clear all ACK pending bit for addr (short or extended) */
	else {
		return stm32wba_clear_all_ack_fpb(config);
	}
}
static int stm32wba_configure(const struct device *dev,
			      enum ieee802154_config_type type,
			      const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);
	int ret = 0;

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		LOG_DBG("Setting AUTO_ACK_FPB: enabled = %d", config->auto_ack_fpb.enabled);
		st_ral_auto_pending_bit_set(config->auto_ack_fpb.enabled);
		break;

	case IEEE802154_CONFIG_ACK_FPB:
		ret = stm32wba_configure_ack_fpb(config);
		break;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		LOG_DBG("Setting PAN_COORDINATOR: %d", config->pan_coordinator);
		st_ral_pan_coord_set(config->pan_coordinator);
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		LOG_DBG("Setting PROMISCUOUS mode: %d", config->promiscuous);
		st_ral_promiscuous_set(config->promiscuous);
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		LOG_DBG("Setting EVENT_HANDLER");
		stm32wba_data.event_handler = config->event_handler;
		break;

	default:
#if defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
		ret = stm32wba_configure_extended((enum ieee802154_stm32wba_config_type)type,
						(const struct ieee802154_stm32wba_config *)config);
#else
		LOG_ERR("Unsupported configuration type: %d", type);
		ret = -EINVAL;
#endif
	}

	return ret;
}

static int stm32wba_attr_get(const struct device *dev,
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
	return stm32wba_attr_get_extended((enum ieee802154_stm32wba_attr)attr,
					  (const struct ieee802154_stm32wba_attr_value *)value);
#else
	LOG_ERR("Unsupported attribute: %u", (uint32_t)attr);
	return -ENOENT;
#endif
}

/* WBA radio driver callbacks */

void stm32wba_802154_receive_done(uint8_t *p_buffer,
				  stm32wba_802154_receive_done_metadata_t *p_metadata)
{
	if (p_buffer == NULL) {
		stm32wba_receive_failed(p_metadata->error);
		return;
	}

	for (uint32_t i = 0; i < ARRAY_SIZE(stm32wba_data.rx_frames); i++) {
		if (stm32wba_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		stm32wba_data.rx_frames[i].psdu = p_buffer;
		stm32wba_data.rx_frames[i].length = p_metadata->length;
		stm32wba_data.rx_frames[i].rssi = p_metadata->power;
		stm32wba_data.rx_frames[i].lqi = p_metadata->lqi;
		stm32wba_data.rx_frames[i].time = p_metadata->time;
		stm32wba_data.rx_frames[i].ack_fpb = stm32wba_data.last_frame_ack_fpb;
		stm32wba_data.rx_frames[i].ack_seb = stm32wba_data.last_frame_ack_seb;
		stm32wba_data.last_frame_ack_fpb = false;
		stm32wba_data.last_frame_ack_seb = false;

		k_fifo_put(&stm32wba_data.rx_fifo, &stm32wba_data.rx_frames[i]);

		return;
	}

	LOG_ERR("Not enough RX frames allocated for 802.15.4 driver");
}

void stm32wba_802154_tx_ack_started(bool ack_fpb, bool ack_seb)
{
	stm32wba_data.last_frame_ack_fpb = ack_fpb;
	stm32wba_data.last_frame_ack_seb = ack_seb;
}

void stm32wba_802154_transmit_done(uint8_t *p_frame,
				   stm32wba_802154_tx_error_t error,
				   const stm32wba_802154_transmit_done_metadata_t *p_metadata)
{
	ARG_UNUSED(p_frame);

	stm32wba_data.tx_result = error;
	stm32wba_data.tx_frame_is_secured = p_metadata->is_secured;
	stm32wba_data.tx_frame_mac_hdr_rdy = p_metadata->dynamic_data_is_set;
	stm32wba_data.ack_frame.length = p_metadata->length;

	if (stm32wba_data.ack_frame.length != 0) {
		stm32wba_data.ack_frame.psdu = p_metadata->p_ack;
		stm32wba_data.ack_frame.rssi = p_metadata->power;
		stm32wba_data.ack_frame.lqi = p_metadata->lqi;
	} else {
		stm32wba_data.ack_frame.psdu = NULL;
		stm32wba_data.ack_frame.rssi = 0;
		stm32wba_data.ack_frame.lqi = 0;
	}

	k_sem_give(&stm32wba_data.tx_wait);
}

void stm32wba_802154_cca_done(uint8_t error)
{
	if (error == STM32WBA_802154_RX_ERROR_NONE) {
		stm32wba_data.channel_free = true;
	}

	k_sem_give(&stm32wba_data.cca_wait);
}

void stm32wba_802154_energy_scan_done(int8_t rssiResult)
{
	if (stm32wba_data.energy_scan_done != NULL) {
		energy_scan_done_cb_t callback = stm32wba_data.energy_scan_done;

		stm32wba_data.energy_scan_done = NULL;
		callback(stm32wba_get_device(), rssiResult);
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
			if ( llhwc_cmn_is_dp_slp_enabled() == 0 ) {
				if ( next_radio_evt > CFG_LPM_STDBY_WAKEUP_TIME ) {
					/* No event in a "near" futur */
					ll_sys_dp_slp_enter( next_radio_evt - CFG_LPM_STDBY_WAKEUP_TIME );
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

static const struct stm32wba_802154_config stm32wba_radio_cfg = {
	.irq_config_func = stm32wba_irq_config,
};

static const struct ieee802154_radio_api stm32wba_radio_api = {
	.iface_api.init = stm32wba_iface_init,
	.get_capabilities = stm32wba_get_capabilities,
	.cca = stm32wba_cca,
	.set_channel = stm32wba_set_channel,
	.filter = stm32wba_filter,
	.set_txpower = stm32wba_set_txpower,
	.start = stm32wba_start,
	.stop = stm32wba_stop,
	.tx = stm32wba_tx,
	.ed_scan = stm32wba_energy_scan_start,
	.get_time = stm32wba_get_time,
	.get_sch_acc = stm32wba_get_acc,
	.configure = stm32wba_configure,
	.attr_get = stm32wba_attr_get
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
			  stm32wba_driver_init,
#if defined (CONFIG_PM_DEVICE)
			  PM_DEVICE_DT_INST_GET(0),
#else
			  NULL,
#endif
			  &stm32wba_data,
			  &stm32wba_radio_cfg,
			  CONFIG_IEEE802154_STM32WBA_INIT_PRIO,
			  &stm32wba_radio_api,
			  L2,
			  L2_CTX_TYPE,
			  MTU);
#else
DEVICE_DT_INST_DEFINE(0,
		      stm32wba_driver_init,
#if defined (CONFIG_PM_DEVICE)
		      PM_DEVICE_DT_INST_GET(0),
#else
		      NULL,
#endif
		      &stm32wba_data,
		      &stm32wba_radio_cfg,
		      POST_KERNEL,
		      CONFIG_IEEE802154_STM32WBA_INIT_PRIO,
		      &stm32wba_radio_api);
#endif
