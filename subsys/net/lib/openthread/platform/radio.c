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

#define LOG_LEVEL CONFIG_OPENTHREAD_LOG_LEVEL
#define LOG_MODULE_NAME net_otPlat_radio

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

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

static otRadioState sState = OT_RADIO_STATE_DISABLED;

static otRadioFrame sTransmitFrame;

static struct net_pkt *tx_pkt;
static struct net_buf *tx_payload;

static struct device *radio_dev;
static struct ieee802154_radio_api *radio_api;

static s8_t tx_power;
static u16_t channel;

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

#if OPENTHREAD_ENABLE_DIAG

		if (otPlatDiagModeGet()) {
			otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame,
						    result);
		} else
#endif
		{
			if (sTransmitFrame.mPsdu[0] & 0x20) {
				/*
				 * TODO: Get real ACK frame
				 * instead of making a spoofed one
				 */
				otRadioFrame ackFrame;
				u8_t ackPsdu[] = {0x02, 0x00, 0x00, 0x00, 0x00};

				ackPsdu[2] = sTransmitFrame.mPsdu[2];
				ackFrame.mPsdu = ackPsdu;
				ackFrame.mInfo.mRxInfo.mLqi = 80;
				ackFrame.mInfo.mRxInfo.mRssi = -40;
				ackFrame.mLength = 5;

				otPlatRadioTxDone(aInstance, &sTransmitFrame,
					&ackFrame, result);
			} else {
				otPlatRadioTxDone(aInstance, &sTransmitFrame,
					NULL, result);
			}
		}
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
	ARG_UNUSED(aInstance);

	return OT_RADIO_CAPS_NONE;
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
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aScanChannel);
	ARG_UNUSED(aScanDuration);

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
