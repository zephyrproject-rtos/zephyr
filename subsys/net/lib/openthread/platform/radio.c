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

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel.h>
#include <device.h>
#include <net/ieee802154_radio.h>
#include <net/net_pkt.h>
#include <sys/__assert.h>

#include <openthread/ip6.h>
#include <openthread-system.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>
#include <openthread/message.h>

#include "platform-zephyr.h"

#define SHORT_ADDRESS_SIZE 2

#define FCS_SIZE 2
#define ACK_PKT_LENGTH 3

#define FRAME_TYPE_MASK 0x07
#define FRAME_TYPE_ACK 0x02

#define OT_WORKER_STACK_SIZE 512
#define OT_WORKER_PRIORITY   K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)

enum pending_events {
	PENDING_EVENT_FRAME_TO_SEND, /* There is a tx frame to send  */
	PENDING_EVENT_FRAME_RECEIVED, /* Radio has received new frame */
	PENDING_EVENT_TX_STARTED, /* Radio has started transmitting */
	PENDING_EVENT_TX_DONE, /* Radio transmission finished */
	PENDING_EVENT_DETECT_ENERGY, /* Requested to start Energy Detection
				      * procedure.
				      */
	PENDING_EVENT_DETECT_ENERGY_DONE, /* Energy Detection finished. */
	PENDING_EVENT_COUNT /* keep last */
};

K_SEM_DEFINE(radio_sem, 0, 1);

static otRadioState sState = OT_RADIO_STATE_DISABLED;

static otRadioFrame sTransmitFrame;
static otRadioFrame ack_frame;
static uint8_t ack_psdu[ACK_PKT_LENGTH];

static struct net_pkt *tx_pkt;
static struct net_buf *tx_payload;

static struct device *radio_dev;
static struct ieee802154_radio_api *radio_api;

static int8_t tx_power;
static uint16_t channel;
static bool promiscuous;

static uint16_t energy_detection_time;
static uint8_t  energy_detection_channel;
static int16_t energy_detected_value;

ATOMIC_DEFINE(pending_events, PENDING_EVENT_COUNT);
K_KERNEL_STACK_DEFINE(ot_task_stack, OT_WORKER_STACK_SIZE);
static struct k_work_q ot_work_q;
static otError tx_result;

K_FIFO_DEFINE(rx_pkt_fifo);
K_FIFO_DEFINE(tx_pkt_fifo);

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

void energy_detected(struct device *dev, int16_t max_ed)
{
	if (dev == radio_dev) {
		energy_detected_value = max_ed;
		set_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
	}
}

enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
					     struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	size_t ack_len = net_pkt_get_len(pkt);

	if (ack_len != ACK_PKT_LENGTH) {
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
	ack_frame.mInfo.mRxInfo.mRssi = net_pkt_ieee802154_rssi(pkt);

	return NET_OK;
}

void handle_radio_event(struct device *dev, enum ieee802154_event evt,
			void *event_params)
{
	ARG_UNUSED(event_params);

	switch (evt) {
	case IEEE802154_EVENT_TX_STARTED:
		if (sState == OT_RADIO_STATE_TRANSMIT) {
			set_pending_event(PENDING_EVENT_TX_STARTED);
		}
		break;
	default:
		/* do nothing - ignore event */
		break;
	}
}

static void dataInit(void)
{
	tx_pkt = net_pkt_alloc(K_NO_WAIT);
	__ASSERT_NO_MSG(tx_pkt != NULL);

	tx_payload = net_pkt_get_reserve_tx_data(K_NO_WAIT);
	__ASSERT_NO_MSG(tx_payload != NULL);

	net_pkt_append_buffer(tx_pkt, tx_payload);

	sTransmitFrame.mPsdu = tx_payload->data;
}

void platformRadioInit(void)
{
	struct ieee802154_config cfg;

	dataInit();

	radio_dev = device_get_binding(CONFIG_NET_CONFIG_IEEE802154_DEV_NAME);
	__ASSERT_NO_MSG(radio_dev != NULL);

	radio_api = (struct ieee802154_radio_api *)radio_dev->api;
	if (!radio_api) {
		return;
	}

	k_work_q_start(&ot_work_q, ot_task_stack,
		       K_KERNEL_STACK_SIZEOF(ot_task_stack),
		       OT_WORKER_PRIORITY);

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
	ARG_UNUSED(tx_job);

	tx_result = OT_ERROR_NONE;
	/*
	 * The payload is already in tx_payload->data,
	 * but we need to set the length field
	 * according to sTransmitFrame.length.
	 * We subtract the FCS size as radio driver
	 * adds CRC and increases frame length on its own.
	 */
	tx_payload->len = sTransmitFrame.mLength - FCS_SIZE;

	channel = sTransmitFrame.mChannel;

	radio_api->set_channel(radio_dev, sTransmitFrame.mChannel);
	radio_api->set_txpower(radio_dev, tx_power);

	if (sTransmitFrame.mInfo.mTxInfo.mCsmaCaEnabled) {
		if (radio_api->get_capabilities(radio_dev) &
		    IEEE802154_HW_CSMA) {
			if (radio_api->tx(radio_dev,
					  IEEE802154_TX_MODE_CSMA_CA,
					  tx_pkt, tx_payload) != 0) {
				tx_result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
			}
		} else if (radio_api->cca(radio_dev) != 0 ||
			   radio_api->tx(radio_dev, IEEE802154_TX_MODE_DIRECT,
					 tx_pkt, tx_payload) != 0) {
			tx_result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
		}
	} else {
		if (radio_api->tx(radio_dev, IEEE802154_TX_MODE_DIRECT,
				  tx_pkt, tx_payload)) {
			tx_result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
		}
	}

	set_pending_event(PENDING_EVENT_TX_DONE);
}

static inline void handle_tx_done(otInstance *aInstance)
{
	if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
		otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame,
					    tx_result);
	} else {
		if (sTransmitFrame.mPsdu[0] & IEEE802154_AR_FLAG_SET) {
			if (ack_frame.mLength == 0) {
				LOG_DBG("No ACK received.");
				otPlatRadioTxDone(aInstance, &sTransmitFrame,
						  NULL, OT_ERROR_NO_ACK);
			} else {
				otPlatRadioTxDone(aInstance, &sTransmitFrame,
						  &ack_frame, tx_result);
			}
		} else {
			otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL,
					  tx_result);
		}
		ack_frame.mLength = 0;
	}
}

static void openthread_handle_received_frame(otInstance *instance,
					     struct net_pkt *pkt)
{
	otRadioFrame recv_frame;

	recv_frame.mPsdu = net_buf_frag_last(pkt->buffer)->data;
	/* Length inc. CRC. */
	recv_frame.mLength = net_buf_frags_len(pkt->buffer);
	recv_frame.mChannel = platformRadioChannelGet(instance);
	recv_frame.mInfo.mRxInfo.mLqi = net_pkt_ieee802154_lqi(pkt);
	recv_frame.mInfo.mRxInfo.mRssi = net_pkt_ieee802154_rssi(pkt);
	recv_frame.mInfo.mRxInfo.mAckedWithFramePending =
						net_pkt_ieee802154_ack_fpb(pkt);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	struct net_ptp_time *time = net_pkt_timestamp(pkt);

	recv_frame.mInfo.mRxInfo.mTimestamp = time->second * USEC_PER_SEC +
					      time->nanosecond / NSEC_PER_USEC;
#endif

	if (IS_ENABLED(CONFIG_OPENTHREAD_DIAG) && otPlatDiagModeGet()) {
		otPlatDiagRadioReceiveDone(instance,
					   &recv_frame, OT_ERROR_NONE);
	} else {
		otPlatRadioReceiveDone(instance,
				       &recv_frame, OT_ERROR_NONE);
	}

	net_pkt_unref(pkt);
}

static void openthread_handle_frame_to_send(otInstance *instance,
					    struct net_pkt *pkt)
{
	struct net_buf *buf;
	otMessage *message;
	otMessageSettings settings;

	NET_DBG("Sending Ip6 packet to ot stack");

	settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;
	settings.mLinkSecurityEnabled = true;
	message = otIp6NewMessage(instance, &settings);
	if (message == NULL) {
		goto exit;
	}

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		if (otMessageAppend(message, buf->data,
				    buf->len) != OT_ERROR_NONE) {
			NET_ERR("Error while appending to otMessage");
			otMessageFree(message);
			goto exit;
		}
	}

	if (otIp6Send(instance, message) != OT_ERROR_NONE) {
		NET_ERR("Error while calling otIp6Send");
		goto exit;
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
	static struct k_work tx_job;

	ARG_UNUSED(aInstance);

	if (k_work_pending(&tx_job) == 0) {
		sState = OT_RADIO_STATE_TRANSMIT;

		k_work_init(&tx_job, transmit_message);
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
		struct net_pkt *tx_pkt;

		reset_pending_event(PENDING_EVENT_FRAME_TO_SEND);
		while ((tx_pkt = (struct net_pkt *)k_fifo_get(&tx_pkt_fifo,
							      K_NO_WAIT))
		      != NULL) {
			openthread_handle_frame_to_send(aInstance, tx_pkt);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_FRAME_RECEIVED)) {
		struct net_pkt *rx_pkt;

		reset_pending_event(PENDING_EVENT_FRAME_RECEIVED);
		while ((rx_pkt = (struct net_pkt *)k_fifo_get(&rx_pkt_fifo,
							      K_NO_WAIT))
		      != NULL) {
			openthread_handle_received_frame(aInstance, rx_pkt);
		}
	}

	if (is_pending_event_set(PENDING_EVENT_TX_STARTED)) {
		reset_pending_event(PENDING_EVENT_TX_STARTED);
		otPlatRadioTxStarted(aInstance, &sTransmitFrame);
	}

	if (is_pending_event_set(PENDING_EVENT_TX_DONE)) {
		reset_pending_event(PENDING_EVENT_TX_DONE);

		if (sState == OT_RADIO_STATE_TRANSMIT) {
			sState = OT_RADIO_STATE_RECEIVE;
			handle_tx_done(aInstance);
		}
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
			otPlatRadioEnergyScanDone(aInstance,
						(int8_t)energy_detected_value);
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
		sState = OT_RADIO_STATE_SLEEP;
		radio_api->stop(radio_dev);
	}

	return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
	ARG_UNUSED(aInstance);

	channel = aChannel;

	radio_api->set_channel(radio_dev, aChannel);
	radio_api->set_txpower(radio_dev, tx_power);
	radio_api->start(radio_dev);
	sState = OT_RADIO_STATE_RECEIVE;

	return OT_ERROR_NONE;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aPacket)
{
	otError error = OT_ERROR_INVALID_STATE;

	ARG_UNUSED(aInstance);
	ARG_UNUSED(aPacket);

	__ASSERT_NO_MSG(aPacket == &sTransmitFrame);

	enum ieee802154_hw_caps radio_caps;

	radio_caps = radio_api->get_capabilities(radio_dev);

	if ((sState == OT_RADIO_STATE_RECEIVE) ||
		(radio_caps & IEEE802154_HW_SLEEP_TO_TX)) {
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

static void get_rssi_energy_detected(struct device *dev, int16_t max_ed)
{
	ARG_UNUSED(dev);
	energy_detected_value = max_ed;
	k_sem_give(&radio_sem);
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	int8_t ret_rssi = INT8_MAX;
	int error = 0;
	const uint16_t energy_detection_time = 1;
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
		error = radio_api->ed_scan(radio_dev, energy_detection_time,
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

	return caps;
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

	if (radio_api->ed_scan(radio_dev, energy_detection_time,
				    energy_detected) != 0) {
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

	return -100;
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
