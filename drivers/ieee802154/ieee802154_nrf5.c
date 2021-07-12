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
#include "nrf_802154_const.h"

#if defined(CONFIG_NRF_802154_SER_HOST)
#include "nrf_802154_serialization_error.h"
#endif

struct nrf5_802154_config {
	void (*irq_config_func)(const struct device *dev);
};

static struct nrf5_802154_data nrf5_data;

#define ACK_REQUEST_BYTE 1
#define ACK_REQUEST_BIT (1 << 5)
#define FRAME_PENDING_BYTE 1
#define FRAME_PENDING_BIT (1 << 4)
#define TXTIME_OFFSET_US (1 * USEC_PER_MSEC)

#define DRX_SLOT_PH 0 /* Placeholder delayed reception window ID */
#define DRX_SLOT_RX 1 /* Actual delayed reception window ID */
#define PH_DURATION 10 /* Duration of the placeholder window, in microseconds  */

#if defined(CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE)
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#define EUI64_ADDR (NRF_UICR->OTP)
#else
#define EUI64_ADDR (NRF_UICR->CUSTOMER)
#endif /* CONFIG_SOC_NRF5340_CPUAPP */
#else
#if defined(CONFIG_SOC_NRF5340_CPUAPP) || defined(CONFIG_SOC_NRF5340_CPUNET)
#define EUI64_ADDR (NRF_FICR->INFO.DEVICEID)
#else
#define EUI64_ADDR (NRF_FICR->DEVICEID)
#endif /* CONFIG_SOC_NRF5340_CPUAPP || CONFIG_SOC_NRF5340_CPUNET */
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

static void nrf5_get_eui64(uint8_t *mac)
{
	uint64_t factoryAddress;
	uint32_t index = 0;

#if !defined(CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE)
	/* Set the MAC Address Block Larger (MA-L) formerly called OUI. */
	mac[index++] = (IEEE802154_NRF5_VENDOR_OUI >> 16) & 0xff;
	mac[index++] = (IEEE802154_NRF5_VENDOR_OUI >> 8) & 0xff;
	mac[index++] = IEEE802154_NRF5_VENDOR_OUI & 0xff;
#endif

#if defined(CONFIG_SOC_NRF5340_CPUAPP) && \
	defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#error Accessing EUI64 on the non-secure mode is not supported at the moment
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
		IEEE802154_HW_2_4_GHZ |
		IEEE802154_HW_TX_RX_ACK |
		IEEE802154_HW_ENERGY_SCAN |
		((caps & NRF_802154_CAPABILITY_DELAYED_TX) ? IEEE802154_HW_TXTIME : 0UL) |
		((caps & NRF_802154_CAPABILITY_DELAYED_RX) ? IEEE802154_HW_RXTIME : 0UL) |
		IEEE802154_HW_SLEEP_TO_TX |
		((caps & NRF_802154_CAPABILITY_SECURITY) ? IEEE802154_HW_TX_SEC : 0UL);
}

/* Radio device API */

static enum ieee802154_hw_caps nrf5_get_capabilities(const struct device *dev)
{
	return nrf5_data.capabilities;
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
	uint8_t ack_len;
	struct net_pkt *ack_pkt;
	int err = 0;

	if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) || IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
		ack_len = nrf5_radio->ack_frame.psdu[0];
	} else {
		ack_len = nrf5_radio->ack_frame.psdu[0] - NRF5_FCS_LENGTH;
	}

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

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	struct net_ptp_time timestamp = {
		.second = nrf5_radio->ack_frame.time / USEC_PER_SEC,
		.nanosecond = (nrf5_radio->ack_frame.time % USEC_PER_SEC) * NSEC_PER_USEC
	};

	net_pkt_set_timestamp(ack_pkt, &timestamp);
#endif

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

static bool nrf5_tx_immediate(struct net_pkt *pkt, uint8_t *payload, bool cca)
{
	nrf_802154_transmit_metadata_t metadata = {
		.frame_props = {
			.is_secured = pkt->ieee802154_frame_secured,
			.dynamic_data_is_set = pkt->ieee802154_mac_hdr_rdy,
		},
		.cca = cca,
	};

	return nrf_802154_transmit_raw(payload, &metadata);
}

static bool nrf5_tx_csma_ca(struct net_pkt *pkt, uint8_t *payload)
{
	nrf_802154_transmit_csma_ca_metadata_t metadata = {
		.frame_props = {
			.is_secured = pkt->ieee802154_frame_secured,
			.dynamic_data_is_set = pkt->ieee802154_mac_hdr_rdy,
		},
	};

	return nrf_802154_transmit_csma_ca_raw(payload, &metadata);
}

/* This function cannot be used in the serialized version yet. */
#if defined(CONFIG_NET_PKT_TXTIME) && !defined(CONFIG_NRF_802154_SER_HOST)
static bool nrf5_tx_at(struct net_pkt *pkt, uint8_t *payload, bool cca)
{
	nrf_802154_transmit_at_metadata_t metadata = {
		.frame_props = {
			.is_secured = pkt->ieee802154_frame_secured,
			.dynamic_data_is_set = pkt->ieee802154_mac_hdr_rdy,
		},
		.cca = cca,
		.channel = nrf_802154_channel_get(),
	};
	uint32_t tx_at = net_pkt_txtime(pkt) / NSEC_PER_USEC;
	bool ret;

	ret = nrf_802154_transmit_raw_at(payload,
					 tx_at - TXTIME_OFFSET_US,
					 TXTIME_OFFSET_US,
					 &metadata);
	if (nrf5_data.event_handler) {
		LOG_WRN("TX_STARTED event will be triggered without delay");
	}
	return ret;
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

	LOG_DBG("%p (%u)", payload, payload_len);

	nrf5_radio->tx_psdu[0] = payload_len + NRF5_FCS_LENGTH;
	memcpy(nrf5_radio->tx_psdu + 1, payload, payload_len);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&nrf5_radio->tx_wait);

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
	case IEEE802154_TX_MODE_CCA:
		ret = nrf5_tx_immediate(pkt, nrf5_radio->tx_psdu,
					mode == IEEE802154_TX_MODE_CCA);
		break;
	case IEEE802154_TX_MODE_CSMA_CA:
		ret = nrf5_tx_csma_ca(pkt, nrf5_radio->tx_psdu);
		break;
/* This function cannot be used in the serialized version yet. */
#if defined(CONFIG_NET_PKT_TXTIME) && !defined(CONFIG_NRF_802154_SER_HOST)
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
		__ASSERT_NO_MSG(pkt);
		ret = nrf5_tx_at(pkt, nrf5_radio->tx_psdu,
				 mode == IEEE802154_TX_MODE_TXTIME_CCA);
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

#if NRF_802154_ENCRYPTION_ENABLED
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

static uint64_t nrf5_get_time(const struct device *dev)
{
	ARG_UNUSED(dev);

	return nrf_802154_time_get();
}

static uint8_t nrf5_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_DELAY_TRX_ACC;
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
#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	if (nrf_802154_sleep_if_idle() != NRF_802154_SLEEP_ERROR_NONE) {
		if (nrf5_data.event_handler) {
			nrf5_data.event_handler(dev, IEEE802154_EVENT_SLEEP, NULL);
		} else {
			LOG_WRN("Transition to radio sleep cannot be handled.");
		}
		return 0;
	}
#else
	ARG_UNUSED(dev);
#endif

	if (!nrf_802154_sleep()) {
		LOG_ERR("Error while stopping radio");
		return -EIO;
	}

	LOG_DBG("nRF5 802154 radio stopped");

	return 0;
}

#if !IS_ENABLED(CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT)
static void nrf5_radio_irq(void *arg)
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

	k_fifo_init(&nrf5_radio->rx_fifo);
	k_sem_init(&nrf5_radio->tx_wait, 0, 1);
	k_sem_init(&nrf5_radio->cca_wait, 0, 1);

	nrf_802154_init();

	nrf5_get_capabilities_at_boot();

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
	nrf_802154_security_error_t err;
	nrf_802154_key_t key;
	uint8_t key_id_to_remove;

	__ASSERT(mac_keys, "Invalid argument.");

	/* Remove old invalid key assuming that its index is first_valid_key_id - 1.
	 * TODO: This is Thread specific assumption, need to be changed when RD will provided
	 * API for removing all keys or handling this internally.
	 */
	key_id_to_remove = mac_keys->key_index == 1 ? 0x80 : mac_keys->key_index - 1;

	key.id.mode = mac_keys->key_id_mode;
	key.id.p_key_id = &key_id_to_remove;

	nrf_802154_security_key_remove(&key.id);

	for (struct ieee802154_key *keys = mac_keys; keys->key_value; keys++) {
		key.value.p_cleartext_key = keys->key_value;
		key.id.mode = keys->key_id_mode;
		key.id.p_key_id = &(keys->key_index);
		key.type = NRF_802154_KEY_CLEARTEXT;
		key.frame_counter = 0;
		key.use_global_frame_counter = !(keys->frame_counter_per_key);

		nrf_802154_security_key_remove(&key.id);
		err = nrf_802154_security_key_store(&key);
		__ASSERT(err == NRF_802154_SECURITY_ERROR_NONE ||
				 err == NRF_802154_SECURITY_ERROR_ALREADY_PRESENT,
			 "Storing key failed, err: %d", err);
	};
}
#endif /* CONFIG_NRF_802154_ENCRYPTION */

#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
static void nrf5_receive_at(uint32_t start, uint32_t duration, uint8_t channel, uint32_t id)
{
	nrf_802154_receive_at(start - TXTIME_OFFSET_US, TXTIME_OFFSET_US, duration, channel, id);
}

static void nrf5_config_csl_period(uint16_t period)
{
	nrf_802154_receive_at_cancel(DRX_SLOT_PH);
	nrf_802154_receive_at_cancel(DRX_SLOT_RX);

	nrf_802154_csl_writer_period_set(period);

	/* A placeholder reception window is scheduled so that the radio driver is able to inject
	 * the proper CSL Phase in the transmitted CSL Information Elements.
	 */
	nrf5_receive_at(nrf5_data.csl_rx_time, PH_DURATION, nrf_802154_channel_get(), DRX_SLOT_PH);
}

static void nrf5_schedule_rx(uint8_t channel, uint32_t start, uint32_t duration)
{
	nrf5_receive_at(start, duration, channel, DRX_SLOT_RX);

	/* The placeholder reception window is rescheduled for the next period */
	nrf_802154_receive_at_cancel(DRX_SLOT_PH);
	nrf5_receive_at(nrf5_data.csl_rx_time, PH_DURATION, channel, DRX_SLOT_PH);
}
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */

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
#endif /* CONFIG_NRF_802154_ENCRYPTION */

	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE: {
		uint8_t short_addr_le[SHORT_ADDRESS_SIZE];
		uint8_t ext_addr_le[EXTENDED_ADDRESS_SIZE];

		sys_put_le16(config->ack_ie.short_addr, short_addr_le);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		memcpy(ext_addr_le, config->ack_ie.ext_addr, EXTENDED_ADDRESS_SIZE);
#else
		sys_memcpy_swap(ext_addr_le, config->ack_ie.ext_addr, EXTENDED_ADDRESS_SIZE);
#endif

		if (config->ack_ie.data_len > 0) {
			nrf_802154_ack_data_set(short_addr_le, false, config->ack_ie.data,
						config->ack_ie.data_len, NRF_802154_ACK_DATA_IE);
			nrf_802154_ack_data_set(ext_addr_le, true, config->ack_ie.data,
						config->ack_ie.data_len, NRF_802154_ACK_DATA_IE);
		} else {
			nrf_802154_ack_data_clear(short_addr_le, false, NRF_802154_ACK_DATA_IE);
			nrf_802154_ack_data_clear(ext_addr_le, true, NRF_802154_ACK_DATA_IE);
		}
	} break;

#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	case IEEE802154_CONFIG_CSL_RX_TIME:
		nrf5_data.csl_rx_time = config->csl_rx_time;
		break;

	case IEEE802154_CONFIG_RX_SLOT:
		nrf5_schedule_rx(config->rx_slot.channel, config->rx_slot.start,
				 config->rx_slot.duration);
		break;

	case IEEE802154_CONFIG_CSL_PERIOD:
		nrf5_config_csl_period(config->csl_period);
		break;
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */

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

void nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	if ((id == DRX_SLOT_PH) || (id == DRX_SLOT_RX)) {
		nrf5_stop(net_if_get_device(nrf5_data.iface));
		return;
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

	nrf5_data.last_frame_ack_fpb = false;
	if (nrf5_data.event_handler) {
		nrf5_data.event_handler(net_if_get_device(nrf5_data.iface),
					IEEE802154_EVENT_RX_FAILED,
					(void *)&reason);
	}
}

void nrf_802154_tx_ack_started(const uint8_t *data)
{
	nrf5_data.last_frame_ack_fpb =
				data[FRAME_PENDING_BYTE] & FRAME_PENDING_BIT;
}

void nrf_802154_transmitted_raw(uint8_t *frame,
				const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);

	nrf5_data.tx_result = NRF_802154_TX_ERROR_NONE;
	nrf5_data.tx_frame_is_secured = metadata->frame_props.is_secured;
	nrf5_data.tx_frame_mac_hdr_rdy = metadata->frame_props.dynamic_data_is_set;
	nrf5_data.ack_frame.psdu = metadata->data.transmitted.p_ack;
	nrf5_data.ack_frame.rssi = metadata->data.transmitted.power;
	nrf5_data.ack_frame.lqi = metadata->data.transmitted.lqi;

#if !defined(CONFIG_NRF_802154_SER_HOST) && defined(CONFIG_NET_PKT_TIMESTAMP)
	nrf5_data.ack_frame.time =
		nrf_802154_first_symbol_timestamp_get(
			metadata->data.transmitted.time, nrf5_data.ack_frame.psdu[0]);
#endif

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

#if defined(CONFIG_NRF_802154_SER_HOST)
void nrf_802154_serialization_error(const nrf_802154_ser_err_data_t *p_err)
{
	__ASSERT(false, "802.15.4 serialization error");
}
#endif

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
	.get_time = nrf5_get_time,
	.get_sch_acc = nrf5_get_acc,
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
		nrf5_init, NULL, &nrf5_data, &nrf5_radio_cfg,
		CONFIG_IEEE802154_NRF5_INIT_PRIO,
		&nrf5_radio_api, L2,
		L2_CTX_TYPE, MTU);
#else
DEVICE_DEFINE(nrf5_154_radio, CONFIG_IEEE802154_NRF5_DRV_NAME,
		nrf5_init, NULL, &nrf5_data, &nrf5_radio_cfg,
		POST_KERNEL, CONFIG_IEEE802154_NRF5_INIT_PRIO,
		&nrf5_radio_api);
#endif
