/* ieee802154_nrf5.c - nRF5 802.15.4 driver */

/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_ieee802154

#define LOG_MODULE_NAME ieee802154_nrf5
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
#include <soc_secure.h>
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

#include "ieee802154_nrf5.h"
#include "nrf_802154.h"
#include "nrf_802154_const.h"

#if defined(CONFIG_NRF_802154_SER_HOST)
#include "nrf_802154_serialization_error.h"
#endif

struct nrf5_802154_config {
	void (*irq_config_func)(const struct device *dev);
};

static struct nrf5_802154_data nrf5_data;
#if defined(CONFIG_IEEE802154_RAW_MODE)
static const struct device *nrf5_dev;
#endif

#define DRX_SLOT_RX 0 /* Delayed reception window ID */

#define NSEC_PER_TEN_SYMBOLS (10 * IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS)

#if defined(CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE)
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#error "NRF_UICR->OTP is not supported to read from non-secure"
#else
#define EUI64_ADDR (NRF_UICR->OTP)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
#else
#define EUI64_ADDR (NRF_UICR->CUSTOMER)
#endif /* CONFIG_SOC_NRF5340_CPUAPP */
#endif /* CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE */

#if defined(CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE)
#define EUI64_ADDR_HIGH CONFIG_IEEE802154_NRF5_UICR_EUI64_REG
#define EUI64_ADDR_LOW (CONFIG_IEEE802154_NRF5_UICR_EUI64_REG + 1)
#else
#define EUI64_ADDR_HIGH 0
#define EUI64_ADDR_LOW 1
#endif /* CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE */

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

static inline const struct device *nrf5_get_device(void)
{
#if defined(CONFIG_IEEE802154_RAW_MODE)
	return nrf5_dev;
#else
	return net_if_get_device(nrf5_data.iface);
#endif
}

static void nrf5_get_eui64(uint8_t *mac)
{
	uint64_t factoryAddress;
	uint32_t index = 0;

#if !defined(CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE)
	uint32_t deviceid[2];

	/* Set the MAC Address Block Larger (MA-L) formerly called OUI. */
	mac[index++] = (IEEE802154_NRF5_VENDOR_OUI >> 16) & 0xff;
	mac[index++] = (IEEE802154_NRF5_VENDOR_OUI >> 8) & 0xff;
	mac[index++] = IEEE802154_NRF5_VENDOR_OUI & 0xff;

	soc_secure_read_deviceid(deviceid);

	factoryAddress = (uint64_t)deviceid[EUI64_ADDR_HIGH] << 32;
	factoryAddress |= deviceid[EUI64_ADDR_LOW];
#else
	/* Use device identifier assigned during the production. */
	factoryAddress = (uint64_t)EUI64_ADDR[EUI64_ADDR_HIGH] << 32;
	factoryAddress |= EUI64_ADDR[EUI64_ADDR_LOW];
#endif
	memcpy(mac + index, &factoryAddress, sizeof(factoryAddress) - index);
}

static void nrf5_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct nrf5_802154_data *nrf5_radio = (struct nrf5_802154_data *)arg1;
	struct net_pkt *pkt;
	struct nrf5_802154_rx_frame *rx_frame;
	uint8_t pkt_len;
	uint8_t *psdu;

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
		if (IS_ENABLED(CONFIG_IEEE802154_NRF5_FCS_IN_LENGTH)) {
			pkt_len = rx_frame->psdu[0];
		} else {
			pkt_len = rx_frame->psdu[0] -  IEEE802154_FCS_LENGTH;
		}

#if defined(CONFIG_NET_BUF_DATA_SIZE)
		__ASSERT_NO_MSG(pkt_len <= CONFIG_NET_BUF_DATA_SIZE);
#endif

		LOG_DBG("Frame received");

		/* Block the RX thread until net_pkt is available, so that we
		 * don't drop already ACKed frame in case of temporary net_pkt
		 * scarcity. The nRF 802154 radio driver will accumulate any
		 * incoming frames until it runs out of internal buffers (and
		 * thus stops acknowledging consecutive frames).
		 */
		pkt = net_pkt_rx_alloc_with_buffer(nrf5_radio->iface, pkt_len,
						   AF_UNSPEC, 0, K_FOREVER);

		if (net_pkt_write(pkt, rx_frame->psdu + 1, pkt_len)) {
			goto drop;
		}

		net_pkt_set_ieee802154_lqi(pkt, rx_frame->lqi);
		net_pkt_set_ieee802154_rssi_dbm(pkt, rx_frame->rssi);
		net_pkt_set_ieee802154_ack_fpb(pkt, rx_frame->ack_fpb);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
		net_pkt_set_timestamp_ns(pkt, rx_frame->time * NSEC_PER_USEC);
#endif

#if defined(CONFIG_NET_L2_OPENTHREAD)
		net_pkt_set_ieee802154_ack_seb(pkt, rx_frame->ack_seb);
#endif

		LOG_DBG("Caught a packet (%u) (LQI: %u)",
			 pkt_len, rx_frame->lqi);

		if (net_recv_data(nrf5_radio->iface, pkt) < 0) {
			LOG_ERR("Packet dropped by NET stack");
			goto drop;
		}

		psdu = rx_frame->psdu;
		rx_frame->psdu = NULL;
		nrf_802154_buffer_free_raw(psdu);

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			log_stack_usage(&nrf5_radio->rx_thread);
		}

		continue;

drop:
		psdu = rx_frame->psdu;
		rx_frame->psdu = NULL;
		nrf_802154_buffer_free_raw(psdu);

		net_pkt_unref(pkt);
	}
}

static void nrf5_get_capabilities_at_boot(void)
{
	nrf_802154_capabilities_t caps = nrf_802154_capabilities_get();

	nrf5_data.capabilities =
		IEEE802154_HW_FCS |
		IEEE802154_HW_PROMISC |
		IEEE802154_HW_FILTER |
		((caps & NRF_802154_CAPABILITY_CSMA) ? IEEE802154_HW_CSMA : 0UL) |
		IEEE802154_HW_TX_RX_ACK |
		IEEE802154_HW_RX_TX_ACK |
		IEEE802154_HW_ENERGY_SCAN |
		((caps & NRF_802154_CAPABILITY_DELAYED_TX) ? IEEE802154_HW_TXTIME : 0UL) |
		((caps & NRF_802154_CAPABILITY_DELAYED_RX) ? IEEE802154_HW_RXTIME : 0UL) |
		IEEE802154_HW_SLEEP_TO_TX |
		IEEE802154_RX_ON_WHEN_IDLE |
		((caps & NRF_802154_CAPABILITY_SECURITY) ? IEEE802154_HW_TX_SEC : 0UL)
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
		| IEEE802154_OPENTHREAD_HW_MULTIPLE_CCA
#endif
		;
}

/* Radio device API */

static enum ieee802154_hw_caps nrf5_get_capabilities(const struct device *dev)
{
	return nrf5_data.capabilities;
}

static int nrf5_cca(const struct device *dev)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);

	nrf_802154_channel_set(nrf5_data.channel);

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
		return channel < 11 ? -ENOTSUP : -EINVAL;
	}

	nrf5_data.channel = channel;

	return 0;
}

static int nrf5_energy_scan_start(const struct device *dev,
				  uint16_t duration,
				  energy_scan_done_cb_t done_cb)
{
	int err = 0;

	ARG_UNUSED(dev);

	nrf_802154_channel_set(nrf5_data.channel);

	if (nrf5_data.energy_scan_done == NULL) {
		nrf5_data.energy_scan_done = done_cb;

		if (nrf_802154_energy_detection(duration * 1000) == false) {
			nrf5_data.energy_scan_done = NULL;
			err = -EBUSY;
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

	nrf5_data.txpwr = dbm;

	return 0;
}

static int handle_ack(struct nrf5_802154_data *nrf5_radio)
{
	uint8_t ack_len;
	struct net_pkt *ack_pkt;
	int err = 0;

	if (IS_ENABLED(CONFIG_IEEE802154_NRF5_FCS_IN_LENGTH)) {
		ack_len = nrf5_radio->ack_frame.psdu[0];
	} else {
		ack_len = nrf5_radio->ack_frame.psdu[0] - IEEE802154_FCS_LENGTH;
	}

	ack_pkt = net_pkt_rx_alloc_with_buffer(nrf5_radio->iface, ack_len,
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
	net_pkt_set_ieee802154_rssi_dbm(ack_pkt, nrf5_radio->ack_frame.rssi);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	net_pkt_set_timestamp_ns(ack_pkt, nrf5_radio->ack_frame.time * NSEC_PER_USEC);
#endif

	net_pkt_cursor_init(ack_pkt);

	if (ieee802154_handle_ack(nrf5_radio->iface, ack_pkt) != NET_OK) {
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

static bool nrf5_tx_immediate(struct net_pkt *pkt, uint8_t *payload, bool cca)
{
	nrf_802154_transmit_metadata_t metadata = {
		.frame_props = {
			.is_secured = net_pkt_ieee802154_frame_secured(pkt),
			.dynamic_data_is_set = net_pkt_ieee802154_mac_hdr_rdy(pkt),
		},
		.cca = cca,
		.tx_power = {
			.use_metadata_value = true,
			.power = nrf5_data.txpwr,
		},
		.tx_channel = {
			.use_metadata_value = true,
			.channel = nrf5_data.channel,
		},
	};

	return nrf_802154_transmit_raw(payload, &metadata);
}

#if NRF_802154_CSMA_CA_ENABLED
static bool nrf5_tx_csma_ca(struct net_pkt *pkt, uint8_t *payload)
{
	nrf_802154_transmit_csma_ca_metadata_t metadata = {
		.frame_props = {
			.is_secured = net_pkt_ieee802154_frame_secured(pkt),
			.dynamic_data_is_set = net_pkt_ieee802154_mac_hdr_rdy(pkt),
		},
		.tx_power = {
			.use_metadata_value = true,
			.power = nrf5_data.txpwr,
		},
		.tx_channel = {
			.use_metadata_value = true,
			.channel = nrf5_data.channel,
		},
	};

	return nrf_802154_transmit_csma_ca_raw(payload, &metadata);
}
#endif

#if defined(CONFIG_NET_PKT_TXTIME)
static bool nrf5_tx_at(struct nrf5_802154_data *nrf5_radio, struct net_pkt *pkt,
		   uint8_t *payload, enum ieee802154_tx_mode mode)
{
	bool cca = false;
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	uint8_t max_extra_cca_attempts = 0;
#endif

	switch (mode) {
	case IEEE802154_TX_MODE_TXTIME:
		break;
	case IEEE802154_TX_MODE_TXTIME_CCA:
		cca = true;
		break;
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	case IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA:
		cca = true;
		max_extra_cca_attempts = nrf5_data.max_extra_cca_attempts;
		break;
#endif
		break;
	default:
		__ASSERT_NO_MSG(false);
		return false;
	}

	nrf_802154_transmit_at_metadata_t metadata = {
		.frame_props = {
			.is_secured = net_pkt_ieee802154_frame_secured(pkt),
			.dynamic_data_is_set = net_pkt_ieee802154_mac_hdr_rdy(pkt),
		},
		.cca = cca,
		.channel = nrf5_data.channel,
		.tx_power = {
			.use_metadata_value = true,
			.power = nrf5_data.txpwr,
		},
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
		.extra_cca_attempts = max_extra_cca_attempts,
#endif
	};

	/* The timestamp points to the start of PHR but `nrf_802154_transmit_raw_at`
	 * expects a timestamp pointing to start of SHR.
	 */
	uint64_t tx_at = nrf_802154_timestamp_phr_to_shr_convert(
		net_pkt_timestamp_ns(pkt) / NSEC_PER_USEC);

	return nrf_802154_transmit_raw_at(payload, tx_at, &metadata);
}
#endif /* CONFIG_NET_PKT_TXTIME */

static int nrf5_tx(const struct device *dev,
		   enum ieee802154_tx_mode mode,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
	uint8_t payload_len = frag->len;
	uint8_t *payload = frag->data;
	bool ret = true;

	if (payload_len > IEEE802154_MTU) {
		LOG_ERR("Payload too large: %d", payload_len);
		return -EMSGSIZE;
	}

	LOG_DBG("%p (%u)", payload, payload_len);

	nrf5_radio->tx_psdu[0] = payload_len + IEEE802154_FCS_LENGTH;
	memcpy(nrf5_radio->tx_psdu + 1, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&nrf5_radio->tx_wait);

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
	case IEEE802154_TX_MODE_CCA:
		ret = nrf5_tx_immediate(pkt, nrf5_radio->tx_psdu,
					mode == IEEE802154_TX_MODE_CCA);
		break;
#if NRF_802154_CSMA_CA_ENABLED
	case IEEE802154_TX_MODE_CSMA_CA:
		ret = nrf5_tx_csma_ca(pkt, nrf5_radio->tx_psdu);
		break;
#endif
#if defined(CONFIG_NET_PKT_TXTIME)
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	case IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA:
#endif
		__ASSERT_NO_MSG(pkt);
		ret = nrf5_tx_at(nrf5_radio, pkt, nrf5_radio->tx_psdu, mode);
		break;
#endif /* CONFIG_NET_PKT_TXTIME */
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

#if defined(CONFIG_NRF_802154_ENCRYPTION)
	/*
	 * When frame encryption by the radio driver is enabled, the frame stored in
	 * the tx_psdu buffer is:
	 * 1) authenticated and encrypted in place which causes that after an unsuccessful
	 *    TX attempt, this frame must be propagated back to the upper layer for retransmission.
	 *    The upper layer must ensure that the exact same secured frame is used for
	 *    retransmission
	 * 2) frame counters are updated in place and for keeping the link frame counter up to date,
	 *    this information must be propagated back to the upper layer
	 */
	memcpy(payload, nrf5_radio->tx_psdu + 1, payload_len);
#endif
	net_pkt_set_ieee802154_frame_secured(pkt, nrf5_radio->tx_frame_is_secured);
	net_pkt_set_ieee802154_mac_hdr_rdy(pkt, nrf5_radio->tx_frame_mac_hdr_rdy);

	switch (nrf5_radio->tx_result) {
	case NRF_802154_TX_ERROR_NONE:
		if (nrf5_radio->ack_frame.psdu == NULL) {
			/* No ACK was requested. */
			return 0;
		}
		/* Handle ACK packet. */
		return handle_ack(nrf5_radio);
	case NRF_802154_TX_ERROR_NO_MEM:
		return -ENOBUFS;
	case NRF_802154_TX_ERROR_BUSY_CHANNEL:
		return -EBUSY;
	case NRF_802154_TX_ERROR_INVALID_ACK:
	case NRF_802154_TX_ERROR_NO_ACK:
		return -ENOMSG;
	case NRF_802154_TX_ERROR_ABORTED:
	case NRF_802154_TX_ERROR_TIMESLOT_DENIED:
	case NRF_802154_TX_ERROR_TIMESLOT_ENDED:
	default:
		return -EIO;
	}
}

static net_time_t nrf5_get_time(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (net_time_t)nrf_802154_time_get() * NSEC_PER_USEC;
}

static uint8_t nrf5_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_NRF5_DELAY_TRX_ACC;
}

static int nrf5_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	nrf_802154_tx_power_set(nrf5_data.txpwr);
	nrf_802154_channel_set(nrf5_data.channel);

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
#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	if (nrf_802154_sleep_if_idle() != NRF_802154_SLEEP_ERROR_NONE) {
		if (nrf5_data.event_handler) {
			nrf5_data.event_handler(dev, IEEE802154_EVENT_RX_OFF, NULL);
		} else {
			LOG_WRN("Transition to radio sleep cannot be handled.");
		}
		Z_SPIN_DELAY(1);
		return 0;
	}
#else
	ARG_UNUSED(dev);

	if (!nrf_802154_sleep()) {
		LOG_ERR("Error while stopping radio");
		return -EIO;
	}
#endif

	LOG_DBG("nRF5 802154 radio stopped");

	return 0;
}

#if defined(CONFIG_NRF_802154_CARRIER_FUNCTIONS)
static int nrf5_continuous_carrier(const struct device *dev)
{
	ARG_UNUSED(dev);

	nrf_802154_tx_power_set(nrf5_data.txpwr);
	nrf_802154_channel_set(nrf5_data.channel);

	if (!nrf_802154_continuous_carrier()) {
		LOG_ERR("Failed to enter continuous carrier state");
		return -EIO;
	}

	LOG_DBG("Continuous carrier wave transmission started (channel: %d)",
		nrf_802154_channel_get());

	return 0;
}
#endif

#if !IS_ENABLED(CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT)
static void nrf5_radio_irq(const void *arg)
{
	ARG_UNUSED(arg);

	nrf_802154_radio_irq_handler();
}
#endif

static void nrf5_irq_config(const struct device *dev)
{
	ARG_UNUSED(dev);

#if !IS_ENABLED(CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT)
	IRQ_CONNECT(RADIO_IRQn, NRF_802154_IRQ_PRIORITY,
		    nrf5_radio_irq, NULL, 0);
	irq_enable(RADIO_IRQn);
#endif
}

static int nrf5_init(const struct device *dev)
{
	const struct nrf5_802154_config *nrf5_radio_cfg = NRF5_802154_CFG(dev);
	struct nrf5_802154_data *nrf5_radio = NRF5_802154_DATA(dev);
#if defined(CONFIG_IEEE802154_RAW_MODE)
	nrf5_dev = dev;
#endif

	k_fifo_init(&nrf5_radio->rx_fifo);
	k_sem_init(&nrf5_radio->tx_wait, 0, 1);
	k_sem_init(&nrf5_radio->cca_wait, 0, 1);

	nrf_802154_init();

	nrf5_get_capabilities_at_boot();

	nrf5_radio->rx_on_when_idle = true;
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

#if defined(CONFIG_NRF_802154_ENCRYPTION)
static void nrf5_config_mac_keys(struct ieee802154_key *mac_keys)
{
	nrf_802154_security_key_remove_all();

	for (uint8_t i = 0; mac_keys->key_value
			&& i < NRF_802154_SECURITY_KEY_STORAGE_SIZE; mac_keys++, i++) {
		nrf_802154_key_t key = {
			.value.p_cleartext_key = mac_keys->key_value,
			.id.mode = mac_keys->key_id_mode,
			.id.p_key_id = mac_keys->key_id,
			.type = NRF_802154_KEY_CLEARTEXT,
			.frame_counter = 0,
			.use_global_frame_counter = !(mac_keys->frame_counter_per_key),
		};

		__ASSERT_EVAL((void)nrf_802154_security_key_store(&key),
			nrf_802154_security_error_t err = nrf_802154_security_key_store(&key),
			err == NRF_802154_SECURITY_ERROR_NONE ||
			err == NRF_802154_SECURITY_ERROR_ALREADY_PRESENT,
			"Storing key failed, err: %d", err);
	};
}
#endif /* CONFIG_NRF_802154_ENCRYPTION */

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
		break;

#if defined(CONFIG_NRF_802154_ENCRYPTION)
	case IEEE802154_CONFIG_MAC_KEYS:
		nrf5_config_mac_keys(config->mac_keys);
		break;

	case IEEE802154_CONFIG_FRAME_COUNTER:
		nrf_802154_security_global_frame_counter_set(config->frame_counter);
		break;

	case IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER:
		nrf_802154_security_global_frame_counter_set_if_larger(config->frame_counter);
		break;
#endif /* CONFIG_NRF_802154_ENCRYPTION */

	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE: {
		uint8_t ext_addr_le[EXTENDED_ADDRESS_SIZE];
		uint8_t short_addr_le[SHORT_ADDRESS_SIZE];
		uint8_t element_id;
		bool valid_vendor_specific_ie = false;

		if (config->ack_ie.short_addr == IEEE802154_BROADCAST_ADDRESS ||
		    config->ack_ie.ext_addr == NULL) {
			return -ENOTSUP;
		}

		sys_put_le16(config->ack_ie.short_addr, short_addr_le);
		sys_memcpy_swap(ext_addr_le, config->ack_ie.ext_addr, EXTENDED_ADDRESS_SIZE);

		if (config->ack_ie.header_ie == NULL || config->ack_ie.header_ie->length == 0) {
			nrf_802154_ack_data_clear(short_addr_le, false, NRF_802154_ACK_DATA_IE);
			nrf_802154_ack_data_clear(ext_addr_le, true, NRF_802154_ACK_DATA_IE);
		} else {
			element_id = ieee802154_header_ie_get_element_id(config->ack_ie.header_ie);

#if defined(CONFIG_NET_L2_OPENTHREAD)
			uint8_t vendor_oui_le[IEEE802154_OPENTHREAD_VENDOR_OUI_LEN] =
				IEEE802154_OPENTHREAD_THREAD_IE_VENDOR_OUI;

			if (element_id == IEEE802154_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE &&
			    memcmp(config->ack_ie.header_ie->content.vendor_specific.vendor_oui,
				   vendor_oui_le, sizeof(vendor_oui_le)) == 0) {
				valid_vendor_specific_ie = true;
			}
#endif

			if (element_id != IEEE802154_HEADER_IE_ELEMENT_ID_CSL_IE &&
			    !valid_vendor_specific_ie) {
				return -ENOTSUP;
			}

			nrf_802154_ack_data_set(short_addr_le, false, config->ack_ie.header_ie,
						config->ack_ie.header_ie->length +
							IEEE802154_HEADER_IE_HEADER_LENGTH,
						NRF_802154_ACK_DATA_IE);
			nrf_802154_ack_data_set(ext_addr_le, true, config->ack_ie.header_ie,
						config->ack_ie.header_ie->length +
							IEEE802154_HEADER_IE_HEADER_LENGTH,
						NRF_802154_ACK_DATA_IE);
		}
	} break;

#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	case IEEE802154_CONFIG_EXPECTED_RX_TIME: {

#if defined(CONFIG_NRF_802154_SER_HOST)
		net_time_t period_ns = nrf5_data.csl_period * NSEC_PER_TEN_SYMBOLS;
		bool changed = (config->expected_rx_time - nrf5_data.csl_rx_time) % period_ns;

		nrf5_data.csl_rx_time = config->expected_rx_time;

		if (changed)
#endif /* CONFIG_NRF_802154_SER_HOST */
		{
			nrf_802154_csl_writer_anchor_time_set(
				nrf_802154_timestamp_phr_to_mhr_convert(config->expected_rx_time /
									NSEC_PER_USEC));
		}
	} break;

	case IEEE802154_CONFIG_RX_SLOT: {
		/* Note that even if the nrf_802154_receive_at function is not called in time
		 * (for example due to the call being blocked by higher priority threads) and
		 * the delayed reception window is not scheduled, the CSL phase will still be
		 * calculated as if the following reception windows were at times
		 * anchor_time + n * csl_period. The previously set
		 * anchor_time will be used for calculations.
		 */
		nrf_802154_receive_at(config->rx_slot.start / NSEC_PER_USEC,
				      config->rx_slot.duration / NSEC_PER_USEC,
				      config->rx_slot.channel, DRX_SLOT_RX);
	} break;

	case IEEE802154_CONFIG_CSL_PERIOD: {
		nrf_802154_csl_writer_period_set(config->csl_period);
#if defined(CONFIG_NRF_802154_SER_HOST)
		nrf5_data.csl_period = config->csl_period;
#endif
	} break;
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */

#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	case IEEE802154_OPENTHREAD_CONFIG_MAX_EXTRA_CCA_ATTEMPTS:
		nrf5_data.max_extra_cca_attempts =
			((const struct ieee802154_openthread_config *)config)
				->max_extra_cca_attempts;
		break;
#endif /* CONFIG_IEEE802154_NRF5_MULTIPLE_CCA */

	case IEEE802154_CONFIG_RX_ON_WHEN_IDLE:
		nrf_802154_rx_on_when_idle_set(config->rx_on_when_idle);
		nrf5_data.rx_on_when_idle = config->rx_on_when_idle;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

static int nrf5_attr_get(const struct device *dev,
			 enum ieee802154_attr attr,
			 struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	if (ieee802154_attr_get_channel_page_and_range(
		    attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		    &drv_attr.phy_supported_channels, value) == 0) {
		return 0;
	}

	switch ((uint32_t)attr) {
#if defined(CONFIG_IEEE802154_NRF5_MULTIPLE_CCA)
	/* TODO: t_recca and t_ccatx should be provided by the public API of the
	 * nRF 802.15.4 Radio Driver.
	 */
	case IEEE802154_OPENTHREAD_ATTR_T_RECCA:
		((struct ieee802154_openthread_attr_value *)value)->t_recca = 0;
		break;
	case IEEE802154_OPENTHREAD_ATTR_T_CCATX:
		((struct ieee802154_openthread_attr_value *)value)->t_ccatx = 20;
		break;
#endif
	default:
		return -ENOENT;
	}

	return 0;
}

/* nRF5 radio driver callbacks */

void nrf_802154_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(nrf5_data.rx_frames); i++) {
		if (nrf5_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		nrf5_data.rx_frames[i].psdu = data;
		nrf5_data.rx_frames[i].rssi = power;
		nrf5_data.rx_frames[i].lqi = lqi;

#if defined(CONFIG_NET_PKT_TIMESTAMP)
		nrf5_data.rx_frames[i].time =
			nrf_802154_timestamp_end_to_phr_convert(time, data[0]);
#endif

		nrf5_data.rx_frames[i].ack_fpb = nrf5_data.last_frame_ack_fpb;
		nrf5_data.rx_frames[i].ack_seb = nrf5_data.last_frame_ack_seb;
		nrf5_data.last_frame_ack_fpb = false;
		nrf5_data.last_frame_ack_seb = false;

		k_fifo_put(&nrf5_data.rx_fifo, &nrf5_data.rx_frames[i]);

		return;
	}

	__ASSERT(false, "Not enough rx frames allocated for 15.4 driver");
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
	const struct device *dev = nrf5_get_device();

#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	if (id == DRX_SLOT_RX && error == NRF_802154_RX_ERROR_DELAYED_TIMEOUT) {
		if (!nrf5_data.rx_on_when_idle) {
			/* Transition to RxOff done automatically by the driver */
			return;
		} else if (nrf5_data.event_handler) {
			/* Notify the higher layer to allow it to transition if needed */
			nrf5_data.event_handler(dev, IEEE802154_EVENT_RX_OFF, NULL);
		}
	}
#else
	ARG_UNUSED(id);
#endif

	enum ieee802154_rx_fail_reason reason;

	switch (error) {
	case NRF_802154_RX_ERROR_INVALID_FRAME:
	case NRF_802154_RX_ERROR_DELAYED_TIMEOUT:
		reason = IEEE802154_RX_FAIL_NOT_RECEIVED;
		break;

	case NRF_802154_RX_ERROR_INVALID_FCS:
		reason = IEEE802154_RX_FAIL_INVALID_FCS;
		break;

	case NRF_802154_RX_ERROR_INVALID_DEST_ADDR:
		reason = IEEE802154_RX_FAIL_ADDR_FILTERED;
		break;

	default:
		reason = IEEE802154_RX_FAIL_OTHER;
		break;
	}

	if (IS_ENABLED(CONFIG_IEEE802154_NRF5_LOG_RX_FAILURES)) {
		LOG_INF("Rx failed, error = %d", error);
	}

	nrf5_data.last_frame_ack_fpb = false;
	nrf5_data.last_frame_ack_seb = false;

	if (nrf5_data.event_handler) {
		nrf5_data.event_handler(dev, IEEE802154_EVENT_RX_FAILED, (void *)&reason);
	}
}

void nrf_802154_tx_ack_started(const uint8_t *data)
{
	nrf5_data.last_frame_ack_fpb = data[FRAME_PENDING_OFFSET] & FRAME_PENDING_BIT;
	nrf5_data.last_frame_ack_seb = data[SECURITY_ENABLED_OFFSET] & SECURITY_ENABLED_BIT;
}

void nrf_802154_transmitted_raw(uint8_t *frame,
				const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);

	nrf5_data.tx_result = NRF_802154_TX_ERROR_NONE;
	nrf5_data.tx_frame_is_secured = metadata->frame_props.is_secured;
	nrf5_data.tx_frame_mac_hdr_rdy = metadata->frame_props.dynamic_data_is_set;
	nrf5_data.ack_frame.psdu = metadata->data.transmitted.p_ack;

	if (nrf5_data.ack_frame.psdu) {
		nrf5_data.ack_frame.rssi = metadata->data.transmitted.power;
		nrf5_data.ack_frame.lqi = metadata->data.transmitted.lqi;

#if defined(CONFIG_NET_PKT_TIMESTAMP)
		nrf5_data.ack_frame.time = nrf_802154_timestamp_end_to_phr_convert(
			metadata->data.transmitted.time, nrf5_data.ack_frame.psdu[0]);
#endif
	}

	k_sem_give(&nrf5_data.tx_wait);
}

void nrf_802154_transmit_failed(uint8_t *frame,
				nrf_802154_tx_error_t error,
				const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);

	nrf5_data.tx_result = error;
	nrf5_data.tx_frame_is_secured = metadata->frame_props.is_secured;
	nrf5_data.tx_frame_mac_hdr_rdy = metadata->frame_props.dynamic_data_is_set;

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

void nrf_802154_energy_detected(const nrf_802154_energy_detected_t *result)
{
	if (nrf5_data.energy_scan_done != NULL) {
		energy_scan_done_cb_t callback = nrf5_data.energy_scan_done;

		nrf5_data.energy_scan_done = NULL;
		callback(nrf5_get_device(), result->ed_dbm);
	}
}

void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	if (nrf5_data.energy_scan_done != NULL) {
		energy_scan_done_cb_t callback = nrf5_data.energy_scan_done;

		nrf5_data.energy_scan_done = NULL;
		callback(nrf5_get_device(), SHRT_MAX);
	}
}

#if defined(CONFIG_NRF_802154_SER_HOST)
void nrf_802154_serialization_error(const nrf_802154_ser_err_data_t *err)
{
	__ASSERT(false, "802.15.4 serialization error: %d", err->reason);
	k_oops();
}
#endif

static const struct nrf5_802154_config nrf5_radio_cfg = {
	.irq_config_func = nrf5_irq_config,
};

static const struct ieee802154_radio_api nrf5_radio_api = {
	.iface_api.init = nrf5_iface_init,

	.get_capabilities = nrf5_get_capabilities,
	.cca = nrf5_cca,
	.set_channel = nrf5_set_channel,
	.filter = nrf5_filter,
	.set_txpower = nrf5_set_txpower,
	.start = nrf5_start,
	.stop = nrf5_stop,
#if defined(CONFIG_NRF_802154_CARRIER_FUNCTIONS)
	.continuous_carrier = nrf5_continuous_carrier,
#endif
	.tx = nrf5_tx,
	.ed_scan = nrf5_energy_scan_start,
	.get_time = nrf5_get_time,
	.get_sch_acc = nrf5_get_acc,
	.configure = nrf5_configure,
	.attr_get = nrf5_attr_get
};

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#elif defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
#define L2 CUSTOM_IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(CUSTOM_IEEE802154_L2)
#define MTU CONFIG_NET_L2_CUSTOM_IEEE802154_MTU
#endif

#if defined(CONFIG_NET_L2_PHY_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, nrf5_init, NULL, &nrf5_data, &nrf5_radio_cfg,
			  CONFIG_IEEE802154_NRF5_INIT_PRIO, &nrf5_radio_api, L2,
			  L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, nrf5_init, NULL, &nrf5_data, &nrf5_radio_cfg,
		      POST_KERNEL, CONFIG_IEEE802154_NRF5_INIT_PRIO,
		      &nrf5_radio_api);
#endif
