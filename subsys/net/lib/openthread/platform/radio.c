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

#include <openthread-system.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>

#include "platform-zephyr.h"

#define SHORT_ADDRESS_SIZE 2

#define FCS_SIZE 2
#define ACK_PKT_LENGTH 3

#define FRAME_TYPE_MASK 0x07
#define FRAME_TYPE_ACK 0x02


enum pending_events {
	PENDING_EVENT_DETECT_ENERGY, /* Requested to start Energy Detection
				      * procedure.
				      */
	PENDING_EVENT_DETECT_ENERGY_DONE, /* Energy Detection finished. */
	PENDING_EVENT_COUNT /* keep last */
};

static otRadioState sState = OT_RADIO_STATE_DISABLED;

static otRadioFrame sTransmitFrame;
static otRadioFrame ack_frame;
static u8_t ack_psdu[ACK_PKT_LENGTH];

static struct net_pkt *tx_pkt;
static struct net_buf *tx_payload;

static struct device *radio_dev;
static struct ieee802154_radio_api *radio_api;

static s8_t tx_power;
static u16_t channel;

static u16_t energy_detection_time;
static u8_t  energy_detection_channel;
static s16_t energy_detected_value;

ATOMIC_DEFINE(pending_events, PENDING_EVENT_COUNT);


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

void energy_detected(struct device *dev, s16_t max_ed)
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
	dataInit();

	radio_dev = device_get_binding(CONFIG_NET_CONFIG_IEEE802154_DEV_NAME);
	__ASSERT_NO_MSG(radio_dev != NULL);

	radio_api = (struct ieee802154_radio_api *)radio_dev->driver_api;
	if (!radio_api) {
		return;
	}

	__ASSERT(radio_api->get_capabilities(radio_dev)
		 & IEEE802154_HW_TX_RX_ACK,
		 "Only radios with automatic ack handling "
		 "are currently supported");
}

void platformRadioProcess(otInstance *aInstance)
{
	bool event_pending = false;
	if (sState == OT_RADIO_STATE_TRANSMIT) {
		otError result = OT_ERROR_NONE;

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
			if (radio_api->cca(radio_dev) ||
			    radio_api->tx(radio_dev, tx_pkt, tx_payload)) {
				result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
			}
		} else {
			if (radio_api->tx(radio_dev, tx_pkt, tx_payload)) {
				result = OT_ERROR_CHANNEL_ACCESS_FAILURE;
			}
		}

		sState = OT_RADIO_STATE_RECEIVE;

		if (IS_ENABLED(OPENTHREAD_ENABLE_DIAG) && otPlatDiagModeGet()) {
			otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame,
						    result);
		} else {
			if (sTransmitFrame.mPsdu[0] & IEEE802154_AR_FLAG_SET) {
				if (ack_frame.mLength == 0) {
					LOG_DBG("No ACK received.");

					result = OT_ERROR_NO_ACK;

					otPlatRadioTxDone(aInstance,
							  &sTransmitFrame,
							  NULL, result);

					return;
				}

				otPlatRadioTxDone(aInstance, &sTransmitFrame,
						  &ack_frame, result);

				ack_frame.mLength = 0;
			} else {
				otPlatRadioTxDone(aInstance, &sTransmitFrame,
						  NULL, result);
			}
		}
	}

	if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY)) {
		radio_api->set_channel(radio_dev, energy_detection_channel);

		if (!radio_api->ed_scan(radio_dev, energy_detection_time,
					energy_detected)) {
			reset_pending_event(PENDING_EVENT_DETECT_ENERGY);
		} else {
			event_pending = true;
		}
	}

	if (is_pending_event_set(PENDING_EVENT_DETECT_ENERGY_DONE)) {
		otPlatRadioEnergyScanDone(aInstance,
					  (s8_t)energy_detected_value);
		reset_pending_event(PENDING_EVENT_DETECT_ENERGY_DONE);
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

void otPlatRadioSetPanId(otInstance *aInstance, u16_t aPanId)
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

void otPlatRadioSetShortAddress(otInstance *aInstance, u16_t aShortAddress)
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

otError otPlatRadioReceive(otInstance *aInstance, u8_t aChannel)
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

	if (sState == OT_RADIO_STATE_RECEIVE) {
		error = OT_ERROR_NONE;
		sState = OT_RADIO_STATE_TRANSMIT;
		otSysEventSignalPending();
	}

	return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	/* TODO: No API in Zephyr to get the RSSI. */
	return 0;
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

	return caps;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	/* TODO: No API in Zephyr to get promiscuous mode. */
	bool result = false;

	LOG_DBG("PromiscuousMode=%d", result ? 1 : 0);
	return result;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aEnable);

	LOG_DBG("PromiscuousMode=%d", aEnable ? 1 : 0);
	/* TODO: No API in Zephyr to set promiscuous mode. */
}

otError otPlatRadioEnergyScan(otInstance *aInstance, u8_t aScanChannel,
			      u16_t aScanDuration)
{
	energy_detection_time    = aScanDuration;
	energy_detection_channel = aScanChannel;

	clear_pending_events();

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
		.auto_ack_fpb.enabled = aEnable
	};

	(void)radio_api->configure(radio_dev, IEEE802154_CONFIG_AUTO_ACK_FPB,
				   &config);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance,
					 const u16_t aShortAddress)
{
	ARG_UNUSED(aInstance);

	u8_t short_address[SHORT_ADDRESS_SIZE];
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
		.ack_fpb.addr = (u8_t *)aExtAddress->m8,
		.ack_fpb.extended = true
	};

	if (radio_api->configure(radio_dev, IEEE802154_CONFIG_ACK_FPB,
				 &config) != 0) {
		return OT_ERROR_NO_BUFS;
	}

	return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance,
					   const u16_t aShortAddress)
{
	ARG_UNUSED(aInstance);

	u8_t short_address[SHORT_ADDRESS_SIZE];
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
		.ack_fpb.addr = (u8_t *)aExtAddress->m8,
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
