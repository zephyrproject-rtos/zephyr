/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction
 *   for radio communication.
 *
 */

#define LOG_MODULE_NAME net_otPlat_radio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_time.h>
#include <zephyr/sys/__assert.h>

#include <openthread/ip6.h>
#include <openthread-system.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/time.h>
#include <openthread/message.h>

#include "platform-zephyr.h"

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
#include <openthread/nat64.h>
#endif

#define PKT_IS_IPv6(_p) ((NET_IPV6_HDR(_p)->vtc & 0xf0) == 0x60)

#define SHORT_ADDRESS_SIZE 2

#define FCS_SIZE     2
#if defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
#define ACK_PKT_LENGTH 5
#else
#define ACK_PKT_LENGTH 127
#endif

#define FRAME_TYPE_MASK 0x07
#define FRAME_TYPE_ACK 0x02

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define OT_WORKER_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#else
#define OT_WORKER_PRIORITY K_PRIO_PREEMPT(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#endif

#define CHANNEL_COUNT OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN + 1

/* PHY header duration in us (i.e. 2 symbol periods @ 62.5k symbol rate), see
 * IEEE 802.15.4, sections 12.1.3.1, 12.2.5 and 12.3.3.
 */
#define PHR_DURATION_US 32U

enum pending_events {
	PENDING_EVENT_FRAME_TO_SEND, /* There is a tx frame to send  */
	PENDING_EVENT_FRAME_RECEIVED, /* Radio has received new frame */
	PENDING_EVENT_RX_FAILED, /* The RX failed */
	PENDING_EVENT_TX_STARTED, /* Radio has started transmitting */
	PENDING_EVENT_TX_DONE, /* Radio transmission finished */
	PENDING_EVENT_DETECT_ENERGY, /* Requested to start Energy Detection procedure */
	PENDING_EVENT_DETECT_ENERGY_DONE, /* Energy Detection finished */
	PENDING_EVENT_SLEEP, /* Sleep if idle */
	PENDING_EVENT_COUNT /* Keep last */
};

K_SEM_DEFINE(radio_sem, 0, 1);

static otRadioState sState = OT_RADIO_STATE_DISABLED;

static otRadioFrame sTransmitFrame;
static otRadioFrame ack_frame;
static uint8_t ack_psdu[ACK_PKT_LENGTH];

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
static otRadioIeInfo tx_ie_info;
#endif

static struct net_pkt *tx_pkt;
static struct net_buf *tx_payload;

static const struct device *const radio_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));
static struct ieee802154_radio_api *radio_api;

/* Get the default tx output power from Kconfig */
static int8_t tx_power = CONFIG_OPENTHREAD_DEFAULT_TX_POWER;
static uint16_t channel;
static bool promiscuous;

static uint16_t energy_detection_time;
static uint8_t energy_detection_channel;
static int16_t energy_detected_value;

static int8_t max_tx_power_table[CHANNEL_COUNT];

ATOMIC_DEFINE(pending_events, PENDING_EVENT_COUNT);
K_KERNEL_STACK_DEFINE(ot_task_stack,
		      CONFIG_OPENTHREAD_RADIO_WORKQUEUE_STACK_SIZE);
static struct k_work_q ot_work_q;
static otError rx_result;
static otError tx_result;

K_FIFO_DEFINE(rx_pkt_fifo);
K_FIFO_DEFINE(tx_pkt_fifo);

static int8_t get_transmit_power_for_channel(uint8_t aChannel)
{
	int8_t channel_max_power = OT_RADIO_POWER_INVALID;
	int8_t power = 0; /* 0 dbm as default value */

	if (aChannel >= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN &&
	    aChannel <= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX) {
		channel_max_power =
			max_tx_power_table[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN];
	}

	if (tx_power != OT_RADIO_POWER_INVALID) {
		power = (channel_max_power < tx_power) ? channel_max_power : tx_power;
	} else if (channel_max_power != OT_RADIO_POWER_INVALID) {
		power = channel_max_power;
	}

	return power;
}

static inline bool is_pending_event_set(enum pending_events event)
{
	return atomic_test_bit(pending_events, event);
}

static void set_pending_event(enum pending_events event)
{
	atomic_set_bit(pending_events, event);
	otSysEventSignalPending();
}

static void reset_pending_event(enum pending_events event)
{
	atomic_clear_bit(pending_events, event);
}

static inline void clear_pending_events(void)
{
	atomic_clear(pending_events);
}

void energy_detected(const struct device *dev, int16_t max_ed)
{
	if (dev == radio_dev) {
		energy_detected_value = max_ed;
		set_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
	}
}

enum net_verdict ieee802154_handle_ack(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	size_t ack_len = net_pkt_get_len(pkt);

	if (ack_len > ACK_PKT_LENGTH) {
		return NET_CONTINUE;
	}

	if ((*net_pkt_data(pkt) & FRAME_TYPE_MASK) != FRAME_TYPE_ACK) {
		return NET_CONTINUE;
	}

	if (ack_frame.mLength != 0) {
		LOG_ERR("Overwriting unhandled ACK frame.");
	}

	if (net_pkt_read(pkt, ack_psdu, ack_len) < 0) {
		LOG_ERR("Failed to read ACK frame.");
		return NET_CONTINUE;
	}

	ack_frame.mPsdu = ack_psdu;
	ack_frame.mLength = ack_len;
	ack_frame.mInfo.mRxInfo.mLqi = net_pkt_ieee802154_lqi(pkt);
	ack_frame.mInfo.mRxInfo.mRssi = net_pkt_ieee802154_rssi_dbm(pkt);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	ack_frame.mInfo.mRxInfo.mTimestamp = net_pkt_timestamp_ns(pkt) / NSEC_PER_USEC;
#endif

	return NET_OK;
}

void handle_radio_event(const struct device *dev, enum ieee802154_event evt,
			void *event_params)
{
	ARG_UNUSED(event_params);

	switch (evt) {
	case IEEE802154_EVENT_TX_STARTED:
		if (sState == OT_RADIO_STATE_TRANSMIT) {
			set_pending_event(PENDING_EVENT_TX_STARTED);
		}
		break;
	case IEEE802154_EVENT_RX_FAILED:
		if (sState == OT_RADIO_STATE_RECEIVE) {
			switch (*(enum ieee802154_rx_fail_reason *) event_params) {
			case IEEE802154_RX_FAIL_NOT_RECEIVED:
				rx_result = OT_ERROR_NO_FRAME_RECEIVED;
				break;

			case IEEE802154_RX_FAIL_INVALID_FCS:
				rx_result = OT_ERROR_FCS;
				break;

			case IEEE802154_RX_FAIL_ADDR_FILTERED:
				rx_result = OT_ERROR_DESTINATION_ADDRESS_FILTERED;
				break;

			case IEEE802154_RX_FAIL_OTHER:
			default:
				rx_result = OT_ERROR_FAILED;
				break;
			}
			set_pending_event(PENDING_EVENT_RX_FAILED);
		}
		break;
	case IEEE802154_EVENT_RX_OFF:
		set_pending_event(PENDING_EVENT_SLEEP);
		break;
	default:
		/* do nothing - ignore event */
		break;
	}
}

#if defined(CONFIG_NET_PKT_TXTIME) || defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
/**
 * @brief Convert 32-bit (potentially wrapped) OpenThread microsecond timestamps
 * to 64-bit Zephyr network subsystem nanosecond timestamps.
 *
 * This is a workaround until OpenThread is able to schedule 64-bit RX/TX time.
 *
 * @param target_time_ns_wrapped time in nanoseconds referred to the radio clock
 * modulo UINT32_MAX.
 *
 * @return 64-bit nanosecond timestamp
 */
static net_time_t convert_32bit_us_wrapped_to_64bit_ns(uint32_t target_time_us_wrapped)
{
	/**
	 * OpenThread provides target time as a (potentially wrapped) 32-bit
	 * integer defining a moment in time in the microsecond domain.
	 *
	 * The target time can point to a moment in the future, but can be
	 * overdue as well. In order to determine what's the case and correctly
	 * set the absolute (non-wrapped) target time, it's necessary to compare
	 * the least significant 32 bits of the current 64-bit network subsystem
	 * time with the provided 32-bit target time. Let's assume that half of
	 * the 32-bit range can be used for specifying target times in the
	 * future, and the other half - in the past.
	 */
	uint64_t now_us = otPlatTimeGet();
	uint32_t now_us_wrapped = (uint32_t)now_us;
	uint32_t time_diff = target_time_us_wrapped - now_us_wrapped;
	uint64_t result = UINT64_C(0);

	if (time_diff < 0x80000000) {
		/**
		 * Target time is assumed to be in the future. Check if a 32-bit overflow
		 * occurs between the current time and the target time.
		 */
		if (now_us_wrapped > target_time_us_wrapped) {
			/**
			 * Add a 32-bit overflow and replace the least significant 32 bits
			 * with the provided target time.
			 */
			result = now_us + UINT32_MAX + 1;
			result &= ~(uint64_t)UINT32_MAX;
			result |= target_time_us_wrapped;
		} else {
			/**
			 * Leave the most significant 32 bits and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = (now_us & (~(uint64_t)UINT32_MAX)) | target_time_us_wrapped;
		}
	} else {
		/**
		 * Target time is assumed to be in the past. Check if a 32-bit overflow
		 * occurs between the target time and the current time.
		 */
		if (now_us_wrapped > target_time_us_wrapped) {
			/**
			 * Leave the most significant 32 bits and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = (now_us & (~(uint64_t)UINT32_MAX)) | target_time_us_wrapped;
		} else {
			/**
			 * Subtract a 32-bit overflow and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = now_us - UINT32_MAX - 1;
			result &= ~(uint64_t)UINT32_MAX;
			result |= target_time_us_wrapped;
		}
	}

	__ASSERT_NO_MSG(result <= INT64_MAX / NSEC_PER_USEC);
	return (net_time_t)result * NSEC_PER_USEC;
}
#endif /* CONFIG_NET_PKT_TXTIME || CONFIG_OPENTHREAD_CSL_RECEIVER */

static void dataInit(void)
{
	tx_pkt = net_pkt_alloc(K_NO_WAIT);
	__ASSERT_NO_MSG(tx_pkt != NULL);

	tx_payload = net_pkt_get_reserve_tx_data(IEEE802154_MAX_PHY_PACKET_SIZE,
						 K_NO_WAIT);
	__ASSERT_NO_MSG(tx_payload != NULL);

	net_pkt_append_buffer(tx_pkt, tx_payload);

	sTransmitFrame.mPsdu = tx_payload->data;

	for (size_t i = 0; i < CHANNEL_COUNT; i++) {
		max_tx_power_table[i] = OT_RADIO_POWER_INVALID;
	}

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
	sTransmitFrame.mInfo.mTxInfo.mIeInfo = &tx_ie_info;
#endif
}

void platformRadioInit(void)
{
	struct ieee802154_config cfg;

	dataInit();

	__ASSERT_NO_MSG(device_is_ready(radio_dev));

	radio_api = (struct ieee802154_radio_api *)radio_dev->api;
	if (!radio_api) {
		return;
	}

	k_work_queue_start(&ot_work_q, ot_task_stack,
			   K_KERNEL_STACK_SIZEOF(ot_task_stack),
			   OT_WORKER_PRIORITY, NULL);
	k_thread_name_set(&ot_work_q.thread, "ot_radio_workq");

	if ((radio_api->get_capabilities(radio_dev) &
	     IEEE802154_HW_TX_RX_ACK) != IEEE802154_HW_TX_RX_ACK) {
		LOG_ERR("Only radios with automatic ack handling "
			"are currently supported");
		k_panic();
	}

	cfg.event_handler = handle_radio_event;
	radio_api->configure(radio_dev, IEEE802154_CONFIG_EVENT_HANDLER, &cfg);
}

void transmit_message(struct k_work *tx_job)
{
	int tx_err;

	ARG_UNUSED(tx_job);

	/*
	 * The payload is already in tx_payload->data,
	 * but we need to set the length field
	 * according to sTransmitFrame.length.
	 * We subtract the FCS size as radio driver
	 * adds CRC and increases frame length on its own.
	 */
	tx_payload->len = sTransmitFrame.mLength - FCS_SIZE;

	channel = sTransmitFrame.mChannel;

	radio_api->set_channel(radio_dev, channel);
	radio_api->set_txpower(radio_dev, get_transmit_power_for_channel(channel));

#if defined(CONFIG_OPENTHREAD_TIME_SYNC)
	if (sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset != 0) {
		uint8_t *time_ie =
			sTransmitFrame.mPsdu + sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset;
		uint64_t offset_plat_time =
			otPlatTimeGet() + sTransmitFrame.mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset;

		*(time_ie++) = sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeSyncSeq;
		sys_put_le64(offset_plat_time, time_ie);
	}
#endif

	net_pkt_set_ieee802154_frame_secured(tx_pkt,
					     sTransmitFrame.mInfo.mTxInfo.mIsSecurityProcessed);
	net_pkt_set_ieee802154_mac_hdr_rdy(tx_pkt, sTransmitFrame.mInfo.mTxInfo.mIsHeaderUpdated);

	if ((radio_api->get_capabilities(radio_dev) & IEEE802154_HW_TXTIME) &&
	    (sTransmitFrame.mInfo.mTxInfo.mTxDelay != 0)) {
#if defined(CONFIG_NET_PKT_TXTIME)
		uint32_t tx_at = sTransmitFrame.mInfo.mTxInfo.mTxDelayBaseTime +
				 sTransmitFrame.mInfo.mTxInfo.mTxDelay;
		net_pkt_set_timestamp_ns(tx_pkt, convert_32bit_us_wrapped_to_64bit_ns(tx_at));
#endif
		tx_err =
			radio_api->tx(radio_dev, IEEE802154_TX_MODE_TXTIME_CCA, tx_pkt, tx_payload);
	} else if (sTransmitFrame.mInfo.mTxInfo.mCsmaCaEnabled) {
		if (radio_api->get_capabilities(radio_dev) & IEEE802154_HW_CSMA) {
			tx_err = radio_api->tx(radio_dev, IEEE802154_TX_MODE_CSMA_CA, tx_pkt,
					       tx_payload);
		} else {
			tx_err = radio_api->cca(radio_dev);
			if (tx_err == 0) {
				tx_err = radio_api->tx(radio_dev, IEEE802154_TX_MODE_DIRECT, tx_pkt,
						       tx_payload);
			}
		}
	} else {
		tx_err = radio_api->tx(radio_dev, IEEE802154_TX_MODE_DIRECT, tx_pkt, tx_payload);
	}

	/*
	 * OpenThread handles the following errors:
	 * - OT_ERROR_NONE
	 * - OT_ERROR_NO_ACK
	 * - OT_ERROR_CHANNEL_ACCESS_FAILURE
	 * - OT_ERROR_ABORT
	 * Any other error passed to `otPlatRadioTxDone` will result in assertion.
	 */
	switch (tx_err) {
	case 0:
		tx_result = OT_ERROR_NONE;
		break;
	case -ENOMSG:
		tx_result = OT_ERROR_NO_ACK;
		break;
	case -EBUSY:
		tx_result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
		break;
	case -EIO:
		tx_result = OT_ERROR_ABORT;
		break;
	default:
		tx_result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
		break;
	}

	set_pending_event(PENDING_EVENT_TX_DONE);
}

static inline void handle_tx_done(otInstance *aInstance)
{
	sTransmitFrame.mInfo.mTxInfo.mIsSecurityProcessed =
		net_pkt_ieee802154_frame_secured(tx_pkt);
	sTransmitFrame.mInfo.mTxInfo.mIsHeaderUpdated = net_pkt_ieee802154_mac_hdr_rdy(tx_pkt);

	if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
		otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, tx_result);
	} else {
		otPlatRadioTxDone(aInstance, &sTransmitFrame, ack_frame.mLength ? &ack_frame : NULL,
				  tx_result);
		ack_frame.mLength = 0;
	}
}

static void openthread_handle_received_frame(otInstance *instance,
					     struct net_pkt *pkt)
{
	otRadioFrame recv_frame;
	memset(&recv_frame, 0, sizeof(otRadioFrame));

	recv_frame.mPsdu = net_buf_frag_last(pkt->buffer)->data;
	/* Length inc. CRC. */
	recv_frame.mLength = net_buf_frags_len(pkt->buffer);
	recv_frame.mChannel = platformRadioChannelGet(instance);
	recv_frame.mInfo.mRxInfo.mLqi = net_pkt_ieee802154_lqi(pkt);
	recv_frame.mInfo.mRxInfo.mRssi = net_pkt_ieee802154_rssi_dbm(pkt);
	recv_frame.mInfo.mRxInfo.mAckedWithFramePending = net_pkt_ieee802154_ack_fpb(pkt);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	recv_frame.mInfo.mRxInfo.mTimestamp = net_pkt_timestamp_ns(pkt) / NSEC_PER_USEC;
#endif

	recv_frame.mInfo.mRxInfo.mAckedWithSecEnhAck = net_pkt_ieee802154_ack_seb(pkt);
	recv_frame.mInfo.mRxInfo.mAckFrameCounter = net_pkt_ieee802154_ack_fc(pkt);
	recv_frame.mInfo.mRxInfo.mAckKeyId = net_pkt_ieee802154_ack_keyid(pkt);

	if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
		otPlatDiagRadioReceiveDone(instance, &recv_frame, OT_ERROR_NONE);
	} else {
		otPlatRadioReceiveDone(instance, &recv_frame, OT_ERROR_NONE);
	}

	net_pkt_unref(pkt);
}

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

static otMessage *openthread_ip4_new_msg(otInstance *instance, otMessageSettings *settings)
{
	return otIp4NewMessage(instance, settings);
}

static otError openthread_nat64_send(otInstance *instance, otMessage *message)
{
	return otNat64Send(instance, message);
}

#else /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

static otMessage *openthread_ip4_new_msg(otInstance *instance, otMessageSettings *settings)
{
	return NULL;
}

static otError openthread_nat64_send(otInstance *instance, otMessage *message)
{
	return OT_ERROR_DROP;
}

#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

static void openthread_handle_frame_to_send(otInstance *instance, struct net_pkt *pkt)
{
	otError error;
	struct net_buf *buf;
	otMessage *message;
	otMessageSettings settings;
	bool is_ip6 = PKT_IS_IPv6(pkt);

	NET_DBG("Sending %s packet to ot stack", is_ip6 ? "IPv6" : "IPv4");

	settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;
	settings.mLinkSecurityEnabled = true;

	message = is_ip6 ? otIp6NewMessage(instance, &settings)
			 : openthread_ip4_new_msg(instance, &settings);
	if (!message) {
		NET_ERR("Cannot allocate new message buffer");
		goto exit;
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD)) {
		/* Set multicast loop so the stack can process multicast packets for
		 * subscribed addresses.
		 */
		otMessageSetMulticastLoopEnabled(message, true);
	}

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		if (otMessageAppend(message, buf->data, buf->len) != OT_ERROR_NONE) {
			NET_ERR("Error while appending to otMessage");
			otMessageFree(message);
			goto exit;
		}
	}

	error = is_ip6 ? otIp6Send(instance, message) : openthread_nat64_send(instance, message);

	if (error != OT_ERROR_NONE) {
		NET_ERR("Error while calling %s [error: %d]",
			is_ip6 ? "otIp6Send" : "openthread_nat64_send", error);
	}

exit:
	net_pkt_unref(pkt);
}

int notify_new_rx_frame(struct net_pkt *pkt)
{
	k_fifo_put(&rx_pkt_fifo, pkt);
	set_pending_event(PENDING_EVENT_FRAME_RECEIVED);

	return 0;
}

int notify_new_tx_frame(struct net_pkt *pkt)
{
	k_fifo_put(&tx_pkt_fifo, pkt);
	set_pending_event(PENDING_EVENT_FRAME_TO_SEND);

	return 0;
}

static int run_tx_task(otInstance *aInstance)
{
	static K_WORK_DEFINE(tx_job, transmit_message);

	ARG_UNUSED(aInstance);

	if (!k_work_is_pending(&tx_job)) {
		sState = OT_RADIO_STATE_TRANSMIT;

		k_work_submit_to_queue(&ot_work_q, &tx_job);
		return 0;
	} else {
		return -EBUSY;
	}
}

void platformRadioProcess(otInstance *aInstance)
{
	bool event_pending = false;

	if (is_pending_event_set(PENDING_EVENT_FRAME_TO_SEND)) {
		struct net_pkt *evt_pkt;

		reset_pending_event(PENDING_EVENT_FRAME_TO_SEND);
		while ((evt_pkt = (struct net_pkt *) k_fifo_get(&tx_pkt_fifo, K_NO_WAIT)) != NULL) {
			if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR_RCP)) {
				net_pkt_unref(evt_pkt);
			} else {
				openthread_handle_frame_to_send(aInstance, evt_pkt);
			}
		}
	}

	if (is_pending_event_set(PENDING_EVENT_FRAME_RECEIVED)) {
		struct net_pkt *rx_pkt;

		reset_pending_event(PENDING_EVENT_FRAME_RECEIVED);
		while ((rx_pkt = (struct net_pkt *) k_fifo_get(&rx_pkt_fifo, K_NO_WAIT)) != NULL) {
			openthread_handle_received_frame(aInstance, rx_pkt);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_RX_FAILED)) {
		reset_pending_event(PENDING_EVENT_RX_FAILED);
		if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
			otPlatDiagRadioReceiveDone(aInstance, NULL, rx_result);
		} else {
			otPlatRadioReceiveDone(aInstance, NULL, rx_result);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_TX_STARTED)) {
		reset_pending_event(PENDING_EVENT_TX_STARTED);
		otPlatRadioTxStarted(aInstance, &sTransmitFrame);
	}

	if (is_pending_event_set(PENDING_EVENT_TX_DONE)) {
		reset_pending_event(PENDING_EVENT_TX_DONE);

		if (sState == OT_RADIO_STATE_TRANSMIT ||
		    radio_api->get_capabilities(radio_dev) & IEEE802154_HW_SLEEP_TO_TX) {
			sState = OT_RADIO_STATE_RECEIVE;
			handle_tx_done(aInstance);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_SLEEP)) {
		reset_pending_event(PENDING_EVENT_SLEEP);
		ARG_UNUSED(otPlatRadioSleep(aInstance));
	}

	/* handle events that can't run during transmission */
	if (sState != OT_RADIO_STATE_TRANSMIT) {
		if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY)) {
			radio_api->set_channel(radio_dev,
					       energy_detection_channel);

			if (!radio_api->ed_scan(radio_dev,
						energy_detection_time,
						energy_detected)) {
				reset_pending_event(
					PENDING_EVENT_DETECT_ENERGY);
			} else {
				event_pending = true;
			}
		}

		if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY_DONE)) {
			otPlatRadioEnergyScanDone(aInstance, (int8_t) energy_detected_value);
			reset_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
		}
	}

	if (event_pending) {
		otSysEventSignalPending();
	}
}

uint16_t platformRadioChannelGet(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return channel;
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
	ARG_UNUSED(aInstance);

	radio_api->filter(radio_dev, true, IEEE802154_FILTER_TYPE_PAN_ID,
			  (struct ieee802154_filter *) &aPanId);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance,
				   const otExtAddress *aExtAddress)
{
	ARG_UNUSED(aInstance);

	radio_api->filter(radio_dev, true, IEEE802154_FILTER_TYPE_IEEE_ADDR,
			  (struct ieee802154_filter *) &aExtAddress);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
	ARG_UNUSED(aInstance);

	radio_api->filter(radio_dev, true, IEEE802154_FILTER_TYPE_SHORT_ADDR,
			  (struct ieee802154_filter *) &aShortAddress);
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return (sState != OT_RADIO_STATE_DISABLED) ? true : false;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	if (!otPlatRadioIsEnabled(aInstance)) {
		sState = OT_RADIO_STATE_SLEEP;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
	if (otPlatRadioIsEnabled(aInstance)) {
		sState = OT_RADIO_STATE_DISABLED;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	otError error = OT_ERROR_INVALID_STATE;

	if (sState == OT_RADIO_STATE_SLEEP ||
	    sState == OT_RADIO_STATE_RECEIVE ||
	    sState == OT_RADIO_STATE_TRANSMIT) {
		error = OT_ERROR_NONE;
		radio_api->stop(radio_dev);
		sState = OT_RADIO_STATE_SLEEP;
	}

	return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
	ARG_UNUSED(aInstance);

	channel = aChannel;

	radio_api->set_channel(radio_dev, aChannel);
	radio_api->set_txpower(radio_dev, get_transmit_power_for_channel(channel));
	radio_api->start(radio_dev);
	sState = OT_RADIO_STATE_RECEIVE;

	return OT_ERROR_NONE;
}

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
otError otPlatRadioReceiveAt(otInstance *aInstance, uint8_t aChannel,
			     uint32_t aStart, uint32_t aDuration)
{
	int result;

	ARG_UNUSED(aInstance);

	struct ieee802154_config config = {
		.rx_slot.channel = aChannel,
		.rx_slot.start = convert_32bit_us_wrapped_to_64bit_ns(aStart),
		.rx_slot.duration = (net_time_t)aDuration * NSEC_PER_USEC,
	};

	result = radio_api->configure(radio_dev, IEEE802154_CONFIG_RX_SLOT,
				      &config);

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}
#endif

otError platformRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
	if (radio_api->continuous_carrier == NULL) {
		return OT_ERROR_NOT_IMPLEMENTED;
	}

	if ((aEnable) && (sState == OT_RADIO_STATE_RECEIVE)) {
		radio_api->set_txpower(radio_dev, get_transmit_power_for_channel(channel));

		if (radio_api->continuous_carrier(radio_dev) != 0) {
			return OT_ERROR_FAILED;
		}

		sState = OT_RADIO_STATE_TRANSMIT;
	} else if ((!aEnable) && (sState == OT_RADIO_STATE_TRANSMIT)) {
		return otPlatRadioReceive(aInstance, channel);
	} else {
		return OT_ERROR_INVALID_STATE;
	}

	return OT_ERROR_NONE;
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return sState;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aPacket)
{
	otError error = OT_ERROR_INVALID_STATE;

	ARG_UNUSED(aInstance);
	ARG_UNUSED(aPacket);

	__ASSERT_NO_MSG(aPacket == &sTransmitFrame);

	enum ieee802154_hw_caps radio_caps;

	radio_caps = radio_api->get_capabilities(radio_dev);

	if ((sState == OT_RADIO_STATE_RECEIVE) || (radio_caps & IEEE802154_HW_SLEEP_TO_TX)) {
		if (run_tx_task(aInstance) == 0) {
			error = OT_ERROR_NONE;
		}
	}

	return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return &sTransmitFrame;
}

static void get_rssi_energy_detected(const struct device *dev, int16_t max_ed)
{
	ARG_UNUSED(dev);
	energy_detected_value = max_ed;
	k_sem_give(&radio_sem);
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	int8_t ret_rssi = INT8_MAX;
	int error = 0;
	const uint16_t detection_time = 1;
	enum ieee802154_hw_caps radio_caps;
	ARG_UNUSED(aInstance);

	radio_caps = radio_api->get_capabilities(radio_dev);

	if (!(radio_caps & IEEE802154_HW_ENERGY_SCAN)) {
		/*
		 * TODO: No API in Zephyr to get the RSSI
		 * when IEEE802154_HW_ENERGY_SCAN is not available
		 */
		ret_rssi = 0;
	} else {
		/*
		 * Blocking implementation of get RSSI
		 * using no-blocking ed_scan
		 */
		error = radio_api->ed_scan(radio_dev, detection_time,
					   get_rssi_energy_detected);

		if (error == 0) {
			k_sem_take(&radio_sem, K_FOREVER);

			ret_rssi = (int8_t)energy_detected_value;
		}
	}

	return ret_rssi;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
	otRadioCaps caps = OT_RADIO_CAPS_NONE;

	enum ieee802154_hw_caps radio_caps;
	ARG_UNUSED(aInstance);
	__ASSERT(radio_api,
	    "platformRadioInit needs to be called prior to otPlatRadioGetCaps");

	radio_caps = radio_api->get_capabilities(radio_dev);

	if (radio_caps & IEEE802154_HW_ENERGY_SCAN) {
		caps |= OT_RADIO_CAPS_ENERGY_SCAN;
	}

	if (radio_caps & IEEE802154_HW_CSMA) {
		caps |= OT_RADIO_CAPS_CSMA_BACKOFF;
	}

	if (radio_caps & IEEE802154_HW_TX_RX_ACK) {
		caps |= OT_RADIO_CAPS_ACK_TIMEOUT;
	}

	if (radio_caps & IEEE802154_HW_SLEEP_TO_TX) {
		caps |= OT_RADIO_CAPS_SLEEP_TO_TX;
	}

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
	if (radio_caps & IEEE802154_HW_TX_SEC) {
		caps |= OT_RADIO_CAPS_TRANSMIT_SEC;
	}
#endif

#if defined(CONFIG_NET_PKT_TXTIME)
	if (radio_caps & IEEE802154_HW_TXTIME) {
		caps |= OT_RADIO_CAPS_TRANSMIT_TIMING;
	}
#endif

	if (radio_caps & IEEE802154_HW_RXTIME) {
		caps |= OT_RADIO_CAPS_RECEIVE_TIMING;
	}

	if (radio_caps & IEEE802154_RX_ON_WHEN_IDLE) {
		caps |= OT_RADIO_CAPS_RX_ON_WHEN_IDLE;
	}

	return caps;
}

void otPlatRadioSetRxOnWhenIdle(otInstance *aInstance, bool aRxOnWhenIdle)
{
	struct ieee802154_config config = {
		.rx_on_when_idle = aRxOnWhenIdle
	};

	ARG_UNUSED(aInstance);

	LOG_DBG("RxOnWhenIdle=%d", aRxOnWhenIdle ? 1 : 0);

	radio_api->configure(radio_dev, IEEE802154_CONFIG_RX_ON_WHEN_IDLE, &config);
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	LOG_DBG("PromiscuousMode=%d", promiscuous ? 1 : 0);

	return promiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
	struct ieee802154_config config = {
		.promiscuous = aEnable
	};

	ARG_UNUSED(aInstance);

	LOG_DBG("PromiscuousMode=%d", aEnable ? 1 : 0);

	promiscuous = aEnable;
	/* TODO: Should check whether the radio driver actually supports
	 *       promiscuous mode, see net_if_l2(iface)->get_flags() and
	 *       ieee802154_radio_get_hw_capabilities(iface).
	 */
	radio_api->configure(radio_dev, IEEE802154_CONFIG_PROMISCUOUS, &config);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel,
			      uint16_t aScanDuration)
{
	energy_detection_time    = aScanDuration;
	energy_detection_channel = aScanChannel;

	if (radio_api->ed_scan == NULL) {
		return OT_ERROR_NOT_IMPLEMENTED;
	}

	reset_pending_event(PENDING_EVENT_DETECT_ENERGY);
	reset_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);

	radio_api->set_channel(radio_dev, aScanChannel);

	if (radio_api->ed_scan(radio_dev, energy_detection_time, energy_detected) != 0) {
		/*
		 * OpenThread API does not accept failure of this function,
		 * it can return 'No Error' or 'Not Implemented' error only.
		 * If ed_scan start failed event is set to schedule the scan at
		 * later time.
		 */
		LOG_ERR("Failed do start energy scan, scheduling for later");
		set_pending_event(PENDING_EVENT_DETECT_ENERGY);
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance,
					       int8_t *aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance,
					       int8_t aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = {
		.auto_ack_fpb.enabled = aEnable,
		.auto_ack_fpb.mode = IEEE802154_FPB_ADDR_MATCH_THREAD,
	};

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_AUTO_ACK_FPB,
				   &config);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance,
					 const uint16_t aShortAddress)
{
	ARG_UNUSED(aInstance);

	uint8_t short_address[SHORT_ADDRESS_SIZE];
	struct ieee802154_config config = {
		.ack_fpb.enabled = true,
		.ack_fpb.addr = short_address,
		.ack_fpb.extended = false
	};

	sys_put_le16(aShortAddress, short_address);

	if (radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				 &config) != 0) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance,
				       const otExtAddress *aExtAddress)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = {
		.ack_fpb.enabled = true,
		.ack_fpb.addr = (uint8_t *)aExtAddress->m8,
		.ack_fpb.extended = true
	};

	if (radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				 &config) != 0) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance,
					   const uint16_t aShortAddress)
{
	ARG_UNUSED(aInstance);

	uint8_t short_address[SHORT_ADDRESS_SIZE];
	struct ieee802154_config config = {
		.ack_fpb.enabled = false,
		.ack_fpb.addr = short_address,
		.ack_fpb.extended = false
	};

	sys_put_le16(aShortAddress, short_address);

	if (radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				 &config) != 0) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance,
					 const otExtAddress *aExtAddress)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = {
		.ack_fpb.enabled = false,
		.ack_fpb.addr = (uint8_t *)aExtAddress->m8,
		.ack_fpb.extended = true
	};

	if (radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				 &config) != 0) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = {
		.ack_fpb.enabled = false,
		.ack_fpb.addr = NULL,
		.ack_fpb.extended = false
	};

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				   &config);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = {
		.ack_fpb.enabled = false,
		.ack_fpb.addr = NULL,
		.ack_fpb.extended = true
	};

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				   &config);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return CONFIG_OPENTHREAD_DEFAULT_RX_SENSITIVITY;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
	ARG_UNUSED(aInstance);

	if (aPower == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	*aPower = tx_power;

	return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
	ARG_UNUSED(aInstance);

	tx_power = aPower;

	return OT_ERROR_NONE;
}

uint64_t otPlatTimeGet(void)
{
	if (radio_api == NULL || radio_api->get_time == NULL) {
		return k_ticks_to_us_floor64(k_uptime_ticks());
	} else {
		return radio_api->get_time(radio_dev) / NSEC_PER_USEC;
	}
}

#if defined(CONFIG_NET_PKT_TXTIME)
uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return otPlatTimeGet();
}
#endif

#if !defined(CONFIG_OPENTHREAD_THREAD_VERSION_1_1)
void otPlatRadioSetMacKey(otInstance *aInstance, uint8_t aKeyIdMode, uint8_t aKeyId,
			  const otMacKeyMaterial *aPrevKey, const otMacKeyMaterial *aCurrKey,
			  const otMacKeyMaterial *aNextKey, otRadioKeyType aKeyType)
{
	ARG_UNUSED(aInstance);
	__ASSERT_NO_MSG(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

#if defined(CONFIG_OPENTHREAD_PLATFORM_KEYS_EXPORTABLE_ENABLE)
	__ASSERT_NO_MSG(aKeyType == OT_KEY_TYPE_KEY_REF);
	size_t keyLen;
	otError error;

	error = otPlatCryptoExportKey(aPrevKey->mKeyMaterial.mKeyRef,
				      (uint8_t *)aPrevKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE,
				      &keyLen);
	__ASSERT_NO_MSG(error == OT_ERROR_NONE);
	error = otPlatCryptoExportKey(aCurrKey->mKeyMaterial.mKeyRef,
				      (uint8_t *)aCurrKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE,
				      &keyLen);
	__ASSERT_NO_MSG(error == OT_ERROR_NONE);
	error = otPlatCryptoExportKey(aNextKey->mKeyMaterial.mKeyRef,
				      (uint8_t *)aNextKey->mKeyMaterial.mKey.m8, OT_MAC_KEY_SIZE,
				      &keyLen);
	__ASSERT_NO_MSG(error == OT_ERROR_NONE);
#else
	__ASSERT_NO_MSG(aKeyType == OT_KEY_TYPE_LITERAL_KEY);
#endif

	uint8_t key_id_mode = aKeyIdMode >> 3;

	struct ieee802154_key keys[] = {
		{
			.key_id_mode = key_id_mode,
			.frame_counter_per_key = false,
		},
		{
			.key_id_mode = key_id_mode,
			.frame_counter_per_key = false,
		},
		{
			.key_id_mode = key_id_mode,
			.frame_counter_per_key = false,
		},
		{
			.key_value = NULL,
		},
	};

	struct ieee802154_key clear_keys[] = {
		{
			.key_value = NULL,
		},
	};

	if (key_id_mode == 1) {
		/* aKeyId in range: (1, 0x80) means valid keys */
		uint8_t prev_key_id = aKeyId == 1 ? 0x80 : aKeyId - 1;
		uint8_t next_key_id = aKeyId == 0x80 ? 1 : aKeyId + 1;

		keys[0].key_id = &prev_key_id;
		keys[0].key_value = (uint8_t *)aPrevKey->mKeyMaterial.mKey.m8;

		keys[1].key_id = &aKeyId;
		keys[1].key_value = (uint8_t *)aCurrKey->mKeyMaterial.mKey.m8;

		keys[2].key_id = &next_key_id;
		keys[2].key_value = (uint8_t *)aNextKey->mKeyMaterial.mKey.m8;
	} else {
		/* aKeyId == 0 is used only to clear keys for stack reset in RCP */
		__ASSERT_NO_MSG((key_id_mode == 0) && (aKeyId == 0));
	}

	struct ieee802154_config config = {
		.mac_keys = aKeyId == 0 ? clear_keys : keys,
	};

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_MAC_KEYS,
				   &config);
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance,
				   uint32_t aMacFrameCounter)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = { .frame_counter = aMacFrameCounter };

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_FRAME_COUNTER,
				   &config);
}

void otPlatRadioSetMacFrameCounterIfLarger(otInstance *aInstance, uint32_t aMacFrameCounter)
{
	ARG_UNUSED(aInstance);

	struct ieee802154_config config = { .frame_counter = aMacFrameCounter };
	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER,
				   &config);
}
#endif

#if defined(CONFIG_OPENTHREAD_CSL_RECEIVER)
otError otPlatRadioEnableCsl(otInstance *aInstance, uint32_t aCslPeriod, otShortAddress aShortAddr,
			     const otExtAddress *aExtAddr)
{
	struct ieee802154_config config;
	/* CSL phase will be injected on-the-fly by the driver. */
	struct ieee802154_header_ie header_ie =
		IEEE802154_DEFINE_HEADER_IE_CSL_REDUCED(/* phase */ 0, aCslPeriod);
	int result;

	ARG_UNUSED(aInstance);

	/* Configure the CSL period first to give drivers a chance to validate
	 * the IE for consistency if they wish to.
	 */
	config.csl_period = aCslPeriod;
	result = radio_api->configure(radio_dev, IEEE802154_CONFIG_CSL_PERIOD, &config);
	if (result) {
		return OT_ERROR_FAILED;
	}

	/* Configure the CSL IE. */
	config.ack_ie.header_ie = aCslPeriod > 0 ? &header_ie : NULL;
	config.ack_ie.short_addr = aShortAddr;
	config.ack_ie.ext_addr = aExtAddr != NULL ? aExtAddr->m8 : NULL;
	config.ack_ie.purge_ie = false;

	result = radio_api->configure(radio_dev, IEEE802154_CONFIG_ENH_ACK_HEADER_IE, &config);

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

otError otPlatRadioResetCsl(otInstance *aInstance)
{
	struct ieee802154_config config = { 0 };
	int result;

	result = radio_api->configure(radio_dev, IEEE802154_CONFIG_CSL_PERIOD, &config);
	if (result) {
		return OT_ERROR_FAILED;
	}

	config.ack_ie.purge_ie = true;
	result = radio_api->configure(radio_dev, IEEE802154_CONFIG_ENH_ACK_HEADER_IE, &config);

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
	ARG_UNUSED(aInstance);

	/* CSL sample time points to "start of MAC" while the expected RX time
	 * refers to "end of SFD".
	 */
	struct ieee802154_config config = {
		.expected_rx_time =
			convert_32bit_us_wrapped_to_64bit_ns(aCslSampleTime - PHR_DURATION_US),
	};

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_EXPECTED_RX_TIME, &config);
}
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return radio_api->get_sch_acc(radio_dev);
}

#if defined(CONFIG_OPENTHREAD_PLATFORM_CSL_UNCERT)
uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return CONFIG_OPENTHREAD_PLATFORM_CSL_UNCERT;
}
#endif

#if defined(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)
/**
 * Header IE format - IEEE Std. 802.15.4-2015, 7.4.2.1 && 7.4.2.2
 *
 * +---------------------------------+----------------------+
 * | Length    | Element ID | Type=0 |      Vendor OUI      |
 * +-----------+------------+--------+----------------------+
 * |           Bytes: 0-1            |          2-4         |
 * +-----------+---------------------+----------------------+
 * | Bits: 0-6 |    7-14    |   15   | IE_VENDOR_THREAD_OUI |
 * +-----------+------------+--------+----------------------|
 *
 * Thread v1.2.1 Spec., 4.11.3.4.4.6
 * +---------------------------------+-------------------+------------------+
 * |                  Vendor Specific Information                           |
 * +---------------------------------+-------------------+------------------+
 * |                5                |         6         |   7 (optional)   |
 * +---------------------------------+-------------------+------------------+
 * | IE_VENDOR_THREAD_ACK_PROBING_ID | LINK_METRIC_TOKEN | LINK_METRIC_TOKEN|
 * |---------------------------------|-------------------|------------------|
 */
static void set_vendor_ie_header_lm(bool lqi, bool link_margin, bool rssi, uint8_t *ie_header)
{
	/* Vendor-specific IE identifier */
	const uint8_t ie_vendor_id = 0x00;
	/* Thread Vendor-specific ACK Probing IE subtype ID */
	const uint8_t ie_vendor_thread_ack_probing_id = 0x00;
	/* Thread Vendor-specific IE OUI */
	const uint32_t ie_vendor_thread_oui = 0xeab89b;
	/* Thread Vendor-specific ACK Probing IE RSSI value placeholder */
	const uint8_t ie_vendor_thread_rssi_token = 0x01;
	/* Thread Vendor-specific ACK Probing IE Link margin value placeholder */
	const uint8_t ie_vendor_thread_margin_token = 0x02;
	/* Thread Vendor-specific ACK Probing IE LQI value placeholder */
	const uint8_t ie_vendor_thread_lqi_token = 0x03;
	const uint8_t oui_size = 3;
	const uint8_t sub_type = 1;
	const uint8_t id_offset = 7;
	const uint16_t id_mask = 0x00ff << id_offset;
	const uint8_t type = 0x00;
	const uint8_t type_offset = 7;
	const uint8_t type_mask = 0x01 << type_offset;
	const uint8_t length_mask = 0x7f;
	uint8_t content_len;
	uint16_t element_id = 0x0000;
	uint8_t link_metrics_idx = 6;
	uint8_t link_metrics_data_len = (uint8_t)lqi + (uint8_t)link_margin + (uint8_t)rssi;

	__ASSERT(link_metrics_data_len <= 2, "Thread limits to 2 metrics at most");
	__ASSERT(ie_header, "Invalid argument");

	if (link_metrics_data_len == 0) {
		ie_header[0] = 0;
		return;
	}

	/* Set Element ID */
	element_id = (((uint16_t)ie_vendor_id) << id_offset) & id_mask;
	sys_put_le16(element_id, &ie_header[0]);

	/* Set Length - number of octets in content field. */
	content_len = oui_size + sub_type + link_metrics_data_len;
	ie_header[0] = (ie_header[0] & ~length_mask) | (content_len & length_mask);

	/* Set Type */
	ie_header[1] = (ie_header[1] & ~type_mask) | (type & type_mask);

	/* Set Vendor Oui */
	sys_put_le24(ie_vendor_thread_oui, &ie_header[2]);

	/* Set SubType */
	ie_header[5] = ie_vendor_thread_ack_probing_id;

	/* Set Link Metrics Tokens
	 * TODO: Thread requires the order of requested metrics by the Link Metrics Initiator
	 *       to be kept by the Link Metrics Subject in the ACKs.
	 */
	if (lqi) {
		ie_header[link_metrics_idx++] = ie_vendor_thread_lqi_token;
	}

	if (link_margin) {
		ie_header[link_metrics_idx++] = ie_vendor_thread_margin_token;
	}

	if (rssi) {
		ie_header[link_metrics_idx++] = ie_vendor_thread_rssi_token;
	}
}

otError otPlatRadioConfigureEnhAckProbing(otInstance *aInstance, otLinkMetrics aLinkMetrics,
					  const otShortAddress aShortAddress,
					  const otExtAddress *aExtAddress)
{
	struct ieee802154_config config = {
		.ack_ie.short_addr = aShortAddress,
		.ack_ie.ext_addr = aExtAddress->m8,
	};
	uint8_t header_ie_buf[OT_ACK_IE_MAX_SIZE];
	int result;

	ARG_UNUSED(aInstance);

	set_vendor_ie_header_lm(aLinkMetrics.mLqi, aLinkMetrics.mLinkMargin,
				aLinkMetrics.mRssi, header_ie_buf);
	config.ack_ie.header_ie = (struct ieee802154_header_ie *)header_ie_buf;
	result = radio_api->configure(radio_dev, IEEE802154_CONFIG_ENH_ACK_HEADER_IE, &config);

	return result ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

#endif /* CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT */

otError otPlatRadioSetChannelMaxTransmitPower(otInstance *aInstance, uint8_t aChannel,
					      int8_t aMaxPower)
{
	ARG_UNUSED(aInstance);

	if (aChannel < OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN ||
	    aChannel > OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX) {
		return OT_ERROR_INVALID_ARGS;
	}

	max_tx_power_table[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN] = aMaxPower;

	if (aChannel == channel) {
		radio_api->set_txpower(radio_dev, get_transmit_power_for_channel(aChannel));
	}

	return OT_ERROR_NONE;
}
