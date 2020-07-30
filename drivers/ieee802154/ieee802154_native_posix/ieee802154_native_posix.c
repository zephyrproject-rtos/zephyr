/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_native_posix
#if defined(CONFIG_IEEE802154_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#include <errno.h>
#include <stdlib.h>

#include <kernel.h>
#include <arch/cpu.h>
#include <debug/stack.h>

#include <soc.h>
#include <device.h>
#include <init.h>
#include <debug/stack.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <sys/byteorder.h>
#include <string.h>
#include <random/rand32.h>

#include <net/ieee802154_radio.h>
#include <bs_radio/bs_radio.h>
#include "ieee802154_native_posix.h"
#include "ieee802154_native_posix_headers.h"

#define NATIVE_POSIX_802154_DATA(dev)                                          \
	((struct native_posix_802154_data *const)(dev)->data)
#define RX_THREAD_NAME "802154_rx_loop"

static struct native_posix_802154_data radio_data;

static void bs_radio_event_cb(struct bs_radio_event_data *data);
static int rx_frame_alloc(struct native_posix_802154_rx_frame *rx_frame,
			  uint16_t psdu_len);
static void rx_frame_free(struct native_posix_802154_rx_frame *rx_frame);
static void send_ack(const uint8_t *data);
static void fcs_fill(uint8_t *psdu);
static bool fcs_check(const uint8_t *psdu);

static void rx_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct device *dev = (struct device *)arg1;
	struct native_posix_802154_data *native_posix_radio =
		NATIVE_POSIX_802154_DATA(dev);
	struct net_pkt *pkt;
	struct native_posix_802154_rx_frame *rx_frame;
	uint8_t pkt_len;

	while (1) {
		pkt = NULL;
		rx_frame = NULL;

		LOG_DBG("Waiting for frame");

		rx_frame = k_fifo_get(&native_posix_radio->rx_fifo, K_FOREVER);

		__ASSERT_NO_MSG(rx_frame->psdu);

		/* rx_mpdu contains length, psdu, fcs|lqi
		 * The last 2 bytes contain LQI or FCS, depending if
		 * automatic CRC handling is enabled or not, respectively.
		 */
		if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) ||
		    IS_ENABLED(CONFIG_NET_L2_OPENTHREAD)) {
			pkt_len = rx_frame->psdu[0];
		} else {
			pkt_len = rx_frame->psdu[0] -
				  NATIVE_POSIX_802154_FCS_LENGTH;
		}

		__ASSERT_NO_MSG(pkt_len <= CONFIG_NET_BUF_DATA_SIZE);

		LOG_INF("Frame received: packet len (%d)", pkt_len);

		pkt = net_pkt_alloc_with_buffer(native_posix_radio->iface,
						pkt_len, AF_UNSPEC, 0,
						K_NO_WAIT);
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

		LOG_DBG("Caught a packet (%u) (LQI: %u)", pkt_len,
			rx_frame->lqi);

		if (net_recv_data(native_posix_radio->iface, pkt) < 0) {
			LOG_ERR("Packet dropped by NET stack");
			goto drop;
		}

		rx_frame_free(rx_frame);
		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
			log_stack_usage(&native_posix_radio->rx_thread);
		}

		continue;

	drop:
		rx_frame_free(rx_frame);
		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

/* Radio device API */

static enum ieee802154_hw_caps get_caps(struct device *dev)
{
	ARG_UNUSED(dev);
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER | IEEE802154_HW_CSMA |
	       IEEE802154_HW_2_4_GHZ | IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_ENERGY_SCAN;
}

static int cca(struct device *dev)
{
	ARG_UNUSED(dev);
	return bs_radio_cca();
}

static int set_channel(struct device *dev, uint16_t channel)
{
	ARG_UNUSED(dev);

	if ((channel < 11) || (channel > 26)) {
		return -EINVAL;
	}

	return bs_radio_channel_set(channel);
}

static int energy_scan_start(struct device *dev, uint16_t duration,
			     energy_scan_done_cb_t done_cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(done_cb);
	return bs_radio_rssi(duration);
}

static int filter(struct device *dev, bool set,
		  enum ieee802154_filter_type type,
		  const struct ieee802154_filter *filter)
{
	int ret = 0;
	uint8_t addr[2];

	if (!set) {
		return ret;
	}

	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		nrf_802154_pib_extended_address_set(filter->ieee_addr);
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		sys_put_le16(filter->short_addr, addr);
		nrf_802154_pib_short_address_set(addr);
		break;
	case IEEE802154_FILTER_TYPE_PAN_ID:
		sys_put_le16(filter->pan_id, addr);
		nrf_802154_pib_pan_id_set(addr);
		break;
	default:
		ret = -ENOTSUP;
		LOG_INF("Filter type %u is not supported", type);
		break;
	}

	return ret;
}

static int set_txpower(struct device *dev, int16_t dbm)
{
	ARG_UNUSED(dev);
	return bs_radio_tx_power_set(dbm);
}

static int handle_ack(void)
{
	uint8_t ack_len = radio_data.ack_frame.psdu[0];
	struct net_pkt *ack_pkt;
	int err = 0;

	ack_pkt = net_pkt_alloc_with_buffer(radio_data.iface, ack_len,
					    AF_UNSPEC, 0, K_NO_WAIT);
	if (!ack_pkt) {
		LOG_ERR("No free packet available.");
		err = -ENOMEM;
		goto free_ack;
	}

	/* Upper layers expect the frame to start at the MAC header, skip the
	 * PHY header (1 byte).
	 */
	if (net_pkt_write(ack_pkt, radio_data.ack_frame.psdu + 1, ack_len) <
	    0) {
		LOG_ERR("Failed to write to a packet.");
		err = -ENOMEM;
		goto free_net_ack;
	}

	net_pkt_set_ieee802154_lqi(ack_pkt, radio_data.ack_frame.lqi);
	net_pkt_set_ieee802154_rssi(ack_pkt, radio_data.ack_frame.rssi);

	err = net_recv_data(radio_data.iface, ack_pkt);
	if (err < 0) {
		LOG_ERR("ACK packet dropped by NET stack");
		goto free_net_ack;
	}

free_net_ack:
	net_pkt_unref(ack_pkt);

free_ack:
	rx_frame_free(&radio_data.ack_frame);
	radio_data.ack_frame.psdu = NULL;

	return err;
}

static void tx_started(struct device *dev, struct net_pkt *pkt,
		       struct net_buf *frag)
{
	ARG_UNUSED(pkt);

	if (radio_data.event_handler) {
		radio_data.event_handler(dev, IEEE802154_EVENT_TX_STARTED,
					 (void *)frag);
	}
}

static int tx(struct device *dev, enum ieee802154_tx_mode mode,
	      struct net_pkt *pkt, struct net_buf *frag)
{
	struct native_posix_802154_data *native_posix_radio =
		NATIVE_POSIX_802154_DATA(dev);
	uint8_t payload_len = frag->len;
	uint8_t *payload = frag->data;
	int tx_result = -EIO;
	bool ar_set;

	native_posix_radio->tx_psdu[0] =
		payload_len + NATIVE_POSIX_802154_FCS_LENGTH;
	memcpy(native_posix_radio->tx_psdu + 1, payload, payload_len);
	fcs_fill(native_posix_radio->tx_psdu);

	ar_set = nrf_802154_frame_parser_ar_bit_is_set(
		native_posix_radio->tx_psdu);

	/* Reset semaphore in case ACK was received after timeout */
	k_sem_reset(&native_posix_radio->tx_wait);
	k_sem_reset(&native_posix_radio->tx_ack_wait);

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
		tx_result = bs_radio_tx(native_posix_radio->tx_psdu, false);
		break;
	case IEEE802154_TX_MODE_CCA:
		tx_result = bs_radio_tx(native_posix_radio->tx_psdu, true);
		break;
	case IEEE802154_TX_MODE_CSMA_CA:
		tx_result = bs_radio_tx(native_posix_radio->tx_psdu, true);
		break;
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
	default:
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	if (tx_result) {
		LOG_ERR("Cannot send frame");
		return tx_result;
	}

	tx_started(dev, pkt, frag);

	LOG_INF("Sending frame (chan:%d, txpower:%d, frame_len:%d)",
		bs_radio_channel_get(), bs_radio_tx_power_get(), payload_len);

	k_sem_take(&native_posix_radio->tx_wait, K_FOREVER);

	if (!ar_set) {
		/* No ack requested */
		LOG_DBG("Frame has been sent");
		return 0;
	} else {
		/* Waiting for ack */
		LOG_DBG("Start waiting for ack!");
		tx_result = k_sem_take(
			&native_posix_radio->tx_ack_wait,
			K_USEC(NRF_802154_ACK_TIMEOUT_DEFAULT_TIMEOUT));
	}

	if (tx_result) {
		/* Ack was not received. */
		LOG_DBG("Ack not received!");
		return -EFAULT;
	}

	LOG_DBG("Ack has been received");
	return 0;
}

static int start(struct device *dev)
{
	ARG_UNUSED(dev);

	bs_radio_start(bs_radio_event_cb);

	LOG_INF("Native Posix radio started (channel: %d)",
		bs_radio_channel_get());

	return 0;
}

static int stop(struct device *dev)
{
	ARG_UNUSED(dev);
	LOG_INF("Native Posix radio stopped");

	bs_radio_stop();

	return 0;
}

static int driver802154_init(struct device *dev)
{
	struct native_posix_802154_data *native_posix_radio =
		NATIVE_POSIX_802154_DATA(dev);

	k_fifo_init(&native_posix_radio->rx_fifo);
	k_sem_init(&native_posix_radio->tx_wait, 0, 1);
	k_sem_init(&native_posix_radio->tx_ack_wait, 0, 1);

	nrf_802154_ack_data_init();
	nrf_802154_ack_generator_init();
	nrf_802154_pib_init();

	k_thread_create(&native_posix_radio->rx_thread,
			native_posix_radio->rx_stack,
			NATIVE_POSIX_802154_RX_STACK_SIZE, rx_thread, dev, NULL,
			NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_thread_name_set(&native_posix_radio->rx_thread, RX_THREAD_NAME);
	native_posix_radio->ack_frame.psdu = NULL;

	LOG_INF("Native Posix 802154 radio initialized");

	return 0;
}

/** TODO: Change that */
static uint8_t mac[8];
static void iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct native_posix_802154_data *native_posix_radio =
		NATIVE_POSIX_802154_DATA(dev);

	bs_radio_get_mac(mac);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	native_posix_radio->iface = iface;

	ieee802154_init(iface);

	LOG_INF("Iface initialized");
}

static int configure(struct device *dev, enum ieee802154_config_type type,
		     const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		if (config->auto_ack_fpb.enabled) {
			switch (config->auto_ack_fpb.mode) {
			case IEEE802154_FPB_ADDR_MATCH_THREAD:
				LOG_ERR("Thread is not supported\n");
				return -ENOTSUP;

			case IEEE802154_FPB_ADDR_MATCH_ZIGBEE:
				nrf_802154_ack_data_src_addr_matching_method_set(
					NRF_802154_SRC_ADDR_MATCH_ZIGBEE);
				break;

			default:
				return -EINVAL;
			}
		}

		nrf_802154_ack_data_enable(config->auto_ack_fpb.enabled);
		break;

	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.enabled) {
			if (!nrf_802154_ack_data_for_addr_set(
				    config->ack_fpb.addr,
				    config->ack_fpb.extended,
				    NRF_802154_ACK_DATA_PENDING_BIT, NULL, 0)) {
				return -ENOMEM;
			}

			break;
		}

		if (config->ack_fpb.addr != NULL) {
			if (!nrf_802154_ack_data_for_addr_clear(
				    config->ack_fpb.addr,
				    config->ack_fpb.extended,
				    NRF_802154_ACK_DATA_PENDING_BIT)) {
				return -ENOENT;
			}
		} else {
			nrf_802154_ack_data_reset(
				config->ack_fpb.extended,
				NRF_802154_ACK_DATA_PENDING_BIT);
		}

		break;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		nrf_802154_pib_pan_coord_set(config->pan_coordinator);
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		nrf_802154_pib_promiscuous_set(config->promiscuous);
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		radio_data.event_handler = config->event_handler;

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * This function is called on BS_RADIO_EVENT_RX_DONE event reception.
 *
 * It copies the received data into the next free entry in global object of
 * native_posix_802154.rx_buffers.
 *
 * Arguments:
 * data                   -       Received data.
 * power                  -       The power of the received signal.
 * lqi                    -       The link quality inference.
 * time [not supported]   -       The timestamp of reception.
 */
static void on_rx_done(const uint8_t *psdu, int8_t power, uint8_t lqi,
		       uint32_t time)
{
	const uint8_t frame_type = nrf_802154_frame_parser_frame_type_get(psdu);
	const uint8_t frame_len = psdu[0];
	uint8_t num_bytes = frame_len;
	const uint8_t filter_error =
		nrf_802154_filter_frame_part(psdu, &num_bytes);

	/* First level filtering */
	if (!fcs_check(psdu)) {
		LOG_DBG("Rejecting frame - FCS Error");
		return;
	}

	/* Reject invalid frames */
	if (filter_error && !nrf_802154_pib_promiscuous_get()) {
		LOG_DBG("Rejecting frame - Error (len = %d): %u\n", num_bytes,
			filter_error);
		return;
	}

	/* Handle received ack */
	if (FRAME_TYPE_ACK == frame_type) {
		if (0 != rx_frame_alloc(&radio_data.ack_frame, psdu[0] + 1)) {
			LOG_ERR("Not enough memory to allocate rx buffer");
			return;
		}

		memcpy(radio_data.ack_frame.psdu, psdu, psdu[0] + 1);
		radio_data.ack_frame.rssi = power;
		radio_data.ack_frame.lqi = lqi;
		radio_data.ack_frame.time = time;

		if (0 == handle_ack()) {
			/* Notify tx function that ack has been received */
			k_sem_give(&radio_data.tx_ack_wait);
		}

		return;
	}

	/* Generate ack if required */
	if (nrf_802154_frame_parser_ar_bit_is_set(psdu) &&
	    nrf_802154_pib_auto_ack_get() && !filter_error) {
		const uint8_t *ack_frame =
			nrf_802154_ack_generator_create(psdu);
		send_ack(ack_frame);
	}

	/* Push received frame onto the queue */
	for (uint32_t i = 0; i < ARRAY_SIZE(radio_data.rx_frames); i++) {
		if (radio_data.rx_frames[i].psdu != NULL) {
			continue;
		}

		if (0 !=
		    rx_frame_alloc(&radio_data.rx_frames[i], psdu[0] + 1)) {
			LOG_ERR("Not enough memory to allocate rx buffer");
			break;
		}
		memcpy(radio_data.rx_frames[i].psdu, psdu, psdu[0]);
		radio_data.rx_frames[i].time = time;
		radio_data.rx_frames[i].rssi = power;
		radio_data.rx_frames[i].lqi = lqi;

		if (nrf_802154_frame_parser_ar_bit_is_set(psdu)) {
			radio_data.rx_frames[i].ack_fpb =
				radio_data.last_frame_ack_fpb;
		} else {
			radio_data.rx_frames[i].ack_fpb = false;
		}

		radio_data.last_frame_ack_fpb = false;

		k_fifo_put(&radio_data.rx_fifo, &radio_data.rx_frames[i]);

		return;
	}

	__ASSERT(false, "Not enough rx frames allocated for 15.4 driver");
}

static void bs_radio_event_cb(struct bs_radio_event_data *event_data)
{
	switch (event_data->type) {
	case BS_RADIO_EVENT_TX_DONE:
		LOG_DBG("BS_RADIO_EVENT_TX_DONE");
		k_sem_give(&radio_data.tx_wait);
		break;
	case BS_RADIO_EVENT_TX_FAILED:
		LOG_DBG("BS_RADIO_EVENT_TX_FAILED");
		k_sem_give(&radio_data.tx_wait);
		break;
	case BS_RADIO_EVENT_RX_DONE:
		on_rx_done(event_data->rx_done.psdu, event_data->rx_done.rssi,
			   0, 0);
		break;
	case BS_RADIO_EVENT_RX_FAILED:
		LOG_DBG("BS_RADIO_RSSI_RX_FAILED");
		break;
	case BS_RADIO_EVENT_CCA_DONE:
		LOG_DBG("BS_RADIO_EVENT_CCA_DONE");
		break;
	case BS_RADIO_EVENT_CCA_FAILED:
		LOG_DBG("BS_RADIO_EVENT_CCA_FAILED");
		break;
	case BS_RADIO_EVENT_RSSI_DONE:
		LOG_DBG("BS_RADIO_EVENT_RSSI_DONE");
		break;
	case BS_RADIO_EVENT_RSSI_FAILED:
		LOG_DBG("BS_RADIO_EVENT_RSSI_FAILED");
		break;
	}
}

static void send_ack(const uint8_t *psdu)
{
	bs_radio_tx((char *)psdu, false);
}

static int rx_frame_alloc(struct native_posix_802154_rx_frame *rx_frame,
			  uint16_t psdu_len)
{
	uint8_t *buf = malloc((psdu_len + 1) * sizeof(*rx_frame->psdu));

	if (buf == NULL) {
		return -ENOMEM;
	}

	rx_frame->psdu = buf;
	return 0;
}

static void rx_frame_free(struct native_posix_802154_rx_frame *rx_frame)
{
	free(rx_frame->psdu);
	rx_frame->psdu = NULL;
}

/** TODO: Calculate FCS
 * Calculate the fcs of a given frame
 *
 */
static void fcs_fill(uint8_t *psdu)
{
	uint8_t frame_length = psdu[0];
	psdu[frame_length] = 0;
	psdu[frame_length - 1] = 0;
}

/**
 * Checks fcs of a frame
 *
 * Arguments:
 * psdu         -       Points to frame, where psdu[0] is length of a frame
 *
 * Returns:
 * true         -       Fcs of a frame is valid
 * false        -       Fcs of a frame is invalid
 */
static bool fcs_check(const uint8_t *psdu)
{
	uint8_t buf[128];
	uint8_t psdu_len = psdu[0];

	memcpy(buf, psdu, psdu[0] + 1);
	fcs_fill(buf);

	if (psdu[psdu_len] == buf[psdu_len] &&
	    psdu[psdu_len - 1] == buf[psdu_len - 1]) {
		return true;
	}
	return false;
}

static struct ieee802154_radio_api native_posix_radio_api = {
	.iface_api.init = iface_init,
	.get_capabilities = get_caps,
	.cca = cca,
	.set_channel = set_channel,
	.filter = filter,
	.set_txpower = set_txpower,
	.start = start,
	.stop = stop,
	.tx = tx,
	.ed_scan = energy_scan_start,
	.configure = configure,
};

#if CONFIG_IEEE802154_RAW_MODE
DEVICE_AND_API_INIT(native_posix_802154_radio,
		    CONFIG_IEEE802154_NATIVE_POSIX_DRV_NAME, driver802154_init,
		    &radio_data, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &native_posix_radio_api);
#else
NET_DEVICE_INIT(native_posix_802154_radio,
		CONFIG_IEEE802154_NATIVE_POSIX_DRV_NAME, driver802154_init,
		device_pm_control_nop, &radio_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &native_posix_radio_api,
		IEEE802154_L2, NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
#endif
