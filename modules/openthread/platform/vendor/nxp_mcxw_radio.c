/*
 * Copyright (c) 2022-2026, The OpenThread Authors
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for radio communication.
 *
 */
#include <openthread/config.h>
#include "openthread-system.h"

#include "EmbeddedTypes.h"
#include "Phy.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "utils/code_utils.h"
#include "utils/link_metrics.h"
#include "utils/mac_frame.h"

#include "fwk_platform_ot.h"

#include <zephyr/drivers/counter.h>

#if USE_NBU
void PLATFORM_RemoteActiveReq(void);
void PLATFORM_RemoteActiveRel(void);
#else /* USE_NBU */
#define PLATFORM_RemoteActiveReq(void)
#define PLATFORM_RemoteActiveRel(void)
#endif /* USE_NBU */

#if CONFIG_OPENTHREAD_CSL_RECEIVER

#define CMP_OVHD (4 * IEEE802154_SYMBOL_TIME_US) /* 2 LPTRM (32 kHz) ticks */

static bool_t csl_rx = FALSE;

static void set_csl_sample_time(void);
static void start_csl_receiver(void);
static void stop_csl_receiver(void);

#else /* CONFIG_OPENTHREAD_CSL_RECEIVER */
#define start_csl_receiver(void)
#define stop_csl_receiver(void)
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */

#define TX_ENCRYPT_DELAY_SYM 200

/* clang-format off */
#ifndef DEFAULT_CCA_MODE
#define DEFAULT_CCA_MODE                 (gPhyCCAMode1_c)
#endif

#define IEEE802154_FP                    (1 << 4)
#define IEEE802154_ACK_REQUEST           (1 << 5)
#define IEEE802154_MIN_LENGTH            (5)
#define IEEE802154_FRM_CTL_LO_OFFSET     (0)
#define IEEE802154_DSN_OFFSET            (2)
#define IEEE802154_FRM_TYPE_MASK         (0x7)
#define IEEE802154_FRM_TYPE_ACK          (0x2)
#define IEEE802154_SYMBOL_TIME_US        (16)
#define IEEE802154_TURNAROUND_LEN_SYM    (12)
#define IEEE802154_CCA_LEN_SYM           (8)
#define IEEE802154_PHY_SHR_LEN_SYM       (10)
#define IEEE802154_IMM_ACK_WAIT_SYM      (54)
#define IEEE802154_ENH_ACK_WAIT_SYM      (90)

#define PHY_TMR_MAX_VALUE                (gPhyTimeMask_c)
/* clang-format on */

#ifndef MCXW_RADIO_NUM_OF_RX_BUFS
#if USE_NBU
#define MCXW_RADIO_NUM_OF_RX_BUFS (16) /* max number of RX buffers */
#else
#define MCXW_RADIO_NUM_OF_RX_BUFS (8)
#endif
#endif

#if (MCXW_RADIO_NUM_OF_RX_BUFS) & (MCXW_RADIO_NUM_OF_RX_BUFS - 1)
#error "MCXW_RADIO_NUM_OF_RX_BUFS must be power of 2"
#endif
/* The Uncertainty of the scheduling CSL of transmission by the parent, in ±10 us units */
#define CSL_UNCERT 20

#define OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT 100
#define RX_TIME_POLL                                                                               \
	(((uint32_t)OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT * 1000) / IEEE802154_SYMBOL_TIME_US)

#define PLATFORM_CSL_ACCURACY 50

typedef struct {
	otRadioFrame RxFrame;
	void *pPhyBuffer; /* PHY allocated frame. Free it after rx done */
} extended_radio_frame;

typedef struct {
	extended_radio_frame r[MCXW_RADIO_NUM_OF_RX_BUFS];
	volatile uint8_t head;
	volatile uint8_t tail;
	volatile uint8_t next;
	volatile uint8_t last;
} radio_rx_ring_buffer;

static otRadioState radio_state = OT_RADIO_STATE_DISABLED;
static uint16_t pan_id;
static uint8_t radio_channel;
static int8_t radio_max_ed;
static bool_t radio_promisc_enable = FALSE;
static int8_t radio_auto_tx_pwr_lvl;

/* internal state flags */
static bool_t rx_on_idle_disabled = FALSE;
static bool_t is_rx_on_idle = FALSE;
static bool_t is_ed_scan = FALSE;
static bool_t radio_tx_done = FALSE;
static bool_t radio_ed_scan_done = FALSE;
static bool_t is_tx_poll = FALSE;
static bool_t is_rx_after_poll = FALSE;
static bool_t rx_after_poll_done = FALSE;
static bool_t is_radio_event = FALSE;
static otError radio_tx_status;

static otRadioFrame radio_tx_frame;
static uint8_t radio_tx_data[sizeof(macToPdDataMessage_t) + OT_RADIO_FRAME_MAX_SIZE];

static radio_rx_ring_buffer radio_rx_ring;     /* Receive Ring Buffer */
static otRadioFrame radio_rx_ack_frame; /* RX Ack Buffers */
static uint8_t radio_rx_ack_data[OT_RADIO_FRAME_MAX_SIZE];

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
static uint32_t radio_csl_period;
static uint32_t radio_csl_sample_time_us;
#endif

/* Pointer to the LPTMR counter device structure*/
static const struct device *mcxw_lptmr = DEVICE_DT_GET(DT_NODELABEL(lptmr1));

/* Private functions */
static uint16_t rf_compute_csl_phase(uint32_t aTimeUs);
static void rf_abort(void);
static void rf_set_channel(uint8_t channel);
static void rf_set_tx_power(int8_t tx_power);
static uint64_t rf_adjust_tstamp_from_phy(uint64_t ts);
static uint32_t rf_adjust_tstamp_from_ot(uint32_t time);
static void rf_set_rx_on_idle(bool_t val);
static void rf_set_rx_time_poll(uint32_t t);

/* lockless rx ring buffer */
static void reset_rx_ring(void);
static void push_free_frame_rx_ring(void);
static void pop_frame_rx_ring(void);
static extended_radio_frame *get_frame_rx_ring(void);
static extended_radio_frame *get_free_frame_rx_ring(void);
static bool_t is_rx_ring_full(void);

/* otPlatTimeGet utility */
static int64_t mcxw_get_time_ns(void);

static uint8_t ot_phy_ctx = (uint8_t)(-1);

/* rx ring buffer reset */
static void reset_rx_ring(void)
{
	radio_rx_ring.head = MCXW_RADIO_NUM_OF_RX_BUFS - 1;
	radio_rx_ring.tail = MCXW_RADIO_NUM_OF_RX_BUFS - 1;
	radio_rx_ring.next = 0;
	radio_rx_ring.last = 0;
}

/* push a newly received frame to the rx ring buffer */
static void push_free_frame_rx_ring(void)
{
	radio_rx_ring.head = radio_rx_ring.next;
}

/* pop the oldest received frame from the rx ring buffer */
static void pop_frame_rx_ring(void)
{
	radio_rx_ring.tail = radio_rx_ring.last;
}

/* get the reference to the oldest received frame from the rx ring buffer */
static extended_radio_frame *get_frame_rx_ring(void)
{
	extended_radio_frame *f = NULL;
	uint8_t tail = radio_rx_ring.tail;

	if (tail == radio_rx_ring.head) {
		/* ring is empty */
		return NULL;
	}

	tail = (tail + 1) & (MCXW_RADIO_NUM_OF_RX_BUFS - 1);
	f = &radio_rx_ring.r[tail];
	radio_rx_ring.last = tail;

	return f;
}

/* get the reference to the first free frame from the rx ring buffer */
static extended_radio_frame *get_free_frame_rx_ring(void)
{
	extended_radio_frame *f = NULL;
	uint8_t next = (radio_rx_ring.head + 1) & (MCXW_RADIO_NUM_OF_RX_BUFS - 1);

	if (next == radio_rx_ring.tail) {
		/* ring is full */
		return NULL;
	}

	f = &radio_rx_ring.r[next];
	radio_rx_ring.next = next;

	return f;
}

static bool_t is_rx_ring_full(void)
{
	uint8_t next = (radio_rx_ring.head + 1) & (MCXW_RADIO_NUM_OF_RX_BUFS - 1);

	return (next == radio_rx_ring.tail);
}

static int64_t mcxw_get_time_ns(void)
{
	static uint64_t sw_timestamp; /* µs, last timestamp, monotonous */
	static uint64_t hw_timestamp; /* µs, last converted raw reading */
	/* Get new 32bit HW timestamp */
	uint32_t ticks;
	uint64_t hw_timestamp_new;
	uint64_t wrapped_val = 0;
	uint64_t increment;
	unsigned int key;
	/* initialization status */
	static bool initialized;

	if (!initialized) {
		if (!device_is_ready(mcxw_lptmr)) {
			return (int64_t)-1;
		}

		/* Start counter */
		if (counter_start(mcxw_lptmr)) {
			return (int64_t)-1;
		}

		initialized = true;
	}

	key = irq_lock();

	if (counter_get_value(mcxw_lptmr, &ticks)) {
		irq_unlock(key);
		return (int64_t)-1;
	}

	hw_timestamp_new = counter_ticks_to_us(mcxw_lptmr, ticks);

	/* Check if the timestamp has wrapped around */
	if (hw_timestamp > hw_timestamp_new) {
		wrapped_val =
			COUNT_TO_USEC(((uint64_t)1 << 32), counter_get_frequency(mcxw_lptmr));
	}

	increment = (hw_timestamp_new + wrapped_val) - hw_timestamp;
	sw_timestamp += increment;

	/* Store new HW timestamp for next iteration */
	hw_timestamp = hw_timestamp_new;

	irq_unlock(key);

	return (int64_t)sw_timestamp * NSEC_PER_USEC;
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return radio_state;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
	OT_UNUSED_VARIABLE(aInstance);

	PLATFORM_GetIeee802_15_4Addr(aIeeeEui64);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibPanId_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aPanId;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	pan_id = aPanId;
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibLongAddress_c;
	msg.msgData.setReq.PibAttributeValue = *(uint64_t *)aExtAddress->m8;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibShortAddress_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aShortAddress;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	otEXPECT(!otPlatRadioIsEnabled(aInstance));

#if !USE_NBU
	PHY_Enable();
#endif
	/* disable rx on idle */
	otPlatRadioSetRxOnWhenIdle(aInstance, false);

	radio_state = OT_RADIO_STATE_SLEEP;
	reset_rx_ring();

exit:
	return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
	otEXPECT(otPlatRadioIsEnabled(aInstance));

	/* disable rx on idle */
	otPlatRadioSetRxOnWhenIdle(aInstance, false);
	stop_csl_receiver();

#if !USE_NBU
	PHY_Disable();
#endif
	radio_state = OT_RADIO_STATE_DISABLED;

exit:
	return OT_ERROR_NONE;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return radio_state != OT_RADIO_STATE_DISABLED;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError status = OT_ERROR_NONE;

	otEXPECT_ACTION(
			((radio_state != OT_RADIO_STATE_TRANSMIT) &&
			(radio_state != OT_RADIO_STATE_DISABLED)),
			status = OT_ERROR_INVALID_STATE);

	stop_csl_receiver();

	if (is_rx_after_poll) {
		is_rx_after_poll = FALSE;
		radio_state = OT_RADIO_STATE_SLEEP;
		return status;
	}

	otEXPECT(radio_state != OT_RADIO_STATE_SLEEP);

	rf_abort();

	radio_state = OT_RADIO_STATE_SLEEP;

exit:
	return status;
}

void otPlatRadioSetRxOnWhenIdle(otInstance *aInstance, bool aEnable)
{
	OT_UNUSED_VARIABLE(aInstance);

	rx_on_idle_disabled = FALSE;

	if (aEnable) {
		is_rx_on_idle = TRUE;
		rf_set_rx_on_idle(TRUE);

		if (radio_state != OT_RADIO_STATE_TRANSMIT) {
			radio_state = OT_RADIO_STATE_RECEIVE;
		}
		return;
	}

	is_rx_on_idle = FALSE;

	if (!is_ed_scan && (radio_state != OT_RADIO_STATE_TRANSMIT)) {
		rf_abort();
		radio_state = OT_RADIO_STATE_SLEEP;
	} else {
		rf_set_rx_on_idle(FALSE);
	}
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;
	otError status = OT_ERROR_NONE;
	phyStatus_t phy_status;

	otEXPECT_ACTION(
			((radio_state != OT_RADIO_STATE_TRANSMIT) &&
			(radio_state != OT_RADIO_STATE_DISABLED)),
			status = OT_ERROR_INVALID_STATE);

	/* already in rx on the same channel */
	otEXPECT((radio_state != OT_RADIO_STATE_RECEIVE) || (radio_channel != aChannel));
	/*
	 * Ignore rx request if packet reception (data_indication)
	 * after poll was processed.
	 * It can happen if data_conf (with FP=1) and data_ind are handled
	 * during the same otPlatRadioProcess() iteration.
	 */
	otEXPECT(!rx_after_poll_done);

	radio_state = OT_RADIO_STATE_RECEIVE;

	rf_set_channel(aChannel);

	msg.msgType = gPlmeRxReq_c;
	msg.msgData.setTRxStateReq.state = gPhySetRxOn_c;
	msg.msgData.setTRxStateReq.rxDuration = 0xFFFFFFFFU;
	msg.msgData.setTRxStateReq.startTime = gPhySeqStartAsap_c;

	phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);
	if (phy_status != gPhySuccess_c) {
		status = OT_ERROR_INVALID_STATE;
	}

exit:
	rx_after_poll_done = FALSE;

	return status;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetSAMState_c;
	msg.msgData.SAMState = aEnable;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError status = OT_ERROR_NONE;
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeAddToSapTable_c;
	msg.msgData.deviceAddr.mode = 2;
	msg.msgData.deviceAddr.panId = pan_id;

	memcpy(msg.msgData.deviceAddr.addr, (const uint8_t *)&aShortAddress, 2);

	if (gPhySuccess_c != MAC_PLME_SapHandler(&msg, ot_phy_ctx)) {
		/* the status is not returned from PHY over RPMSG */
		status = OT_ERROR_NO_BUFS;
	}

	return status;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError status = OT_ERROR_NONE;
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeAddToSapTable_c;
	msg.msgData.deviceAddr.mode = 3;
	msg.msgData.deviceAddr.panId = pan_id;

	/* Received byte order is little endian */
	memcpy(msg.msgData.deviceAddr.addr, (uint8_t *)aExtAddress, 8);

	if (gPhySuccess_c != MAC_PLME_SapHandler(&msg, ot_phy_ctx)) {
		/* the status is not returned from PHY over RPMSG */
		status = OT_ERROR_NO_BUFS;
	}

	return status;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError status = OT_ERROR_NONE;
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeRemoveFromSAMTable_c;
	msg.msgData.deviceAddr.mode = 2;
	msg.msgData.deviceAddr.panId = pan_id;

	memcpy(msg.msgData.deviceAddr.addr, (const uint8_t *)&aShortAddress, 2);

	if (gPhySuccess_c != MAC_PLME_SapHandler(&msg, ot_phy_ctx)) {
		/* the status is not returned from PHY over RPMSG */
		status = OT_ERROR_NO_ADDRESS;
	}

	return status;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError status = OT_ERROR_NONE;
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeRemoveFromSAMTable_c;
	msg.msgData.deviceAddr.mode = 3;
	msg.msgData.deviceAddr.panId = pan_id;

	memcpy(msg.msgData.deviceAddr.addr, (uint8_t *)aExtAddress, 8);

	if (gPhySuccess_c != MAC_PLME_SapHandler(&msg, ot_phy_ctx)) {
		/* the status is not returned from PHY over RPMSG */
		status = OT_ERROR_NO_ADDRESS;
	}

	return status;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
	/* This must be implemented */

	OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
	/* This must be implemented */

	OT_UNUSED_VARIABLE(aInstance);
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return &radio_tx_frame;
}

static void set_tx_req(pdDataReq_t *tx_req, otRadioFrame *aFrame)
{
	rf_set_channel(aFrame->mChannel);

	tx_req->psduLength = aFrame->mLength;
	tx_req->CCABeforeTx = DEFAULT_CCA_MODE;

	/* aFrame->mPsdu will point to radio_tx_data data buffer after
	 * macToPdDataMessage_t structure.
	 */
	tx_req->pPsdu = aFrame->mPsdu;

	if (aFrame->mPsdu[IEEE802154_FRM_CTL_LO_OFFSET] & IEEE802154_ACK_REQUEST) {
		tx_req->ackRequired = gPhyRxAckRqd_c;
		/* The 3 bytes are 1 byte frame length and 2 bytes FCS */
		tx_req->txDuration = IEEE802154_CCA_LEN_SYM + IEEE802154_PHY_SHR_LEN_SYM +
				     (3 + aFrame->mLength) * OT_RADIO_SYMBOLS_PER_OCTET +
				     IEEE802154_TURNAROUND_LEN_SYM;

		if (otMacFrameIsVersion2015(aFrame)) {
			/*
			 * Because enhanced ack can be of variable length we need to set the timeout
			 * value to account for the FCF and addressing fields only, and stop the
			 * timeout timer after they are received and validated as a valid ACK.
			 */
			tx_req->txDuration += IEEE802154_ENH_ACK_WAIT_SYM;
		} else {
			tx_req->txDuration += IEEE802154_IMM_ACK_WAIT_SYM;
		}
	} else {
		tx_req->ackRequired = gPhyNoAckRqd_c;
		tx_req->txDuration = 0xFFFFFFFFU;
	}

	tx_req->flags = 0;
	tx_req->startTime = gPhySeqStartAsap_c;

	if (aFrame->mInfo.mTxInfo.mTxDelay) {
		tx_req->startTime = rf_adjust_tstamp_from_ot(
			aFrame->mInfo.mTxInfo.mTxDelayBaseTime + aFrame->mInfo.mTxInfo.mTxDelay);
		tx_req->startTime /= IEEE802154_SYMBOL_TIME_US;
		tx_req->startTime -= IEEE802154_PHY_SHR_LEN_SYM;
	}

	if (otMacFrameIsSecurityEnabled(aFrame) && otMacFrameIsKeyIdMode1(aFrame)) {
		if (!aFrame->mInfo.mTxInfo.mIsSecurityProcessed) {
			tx_req->flags |= gPhyEncFrame;

			if (!aFrame->mInfo.mTxInfo.mIsHeaderUpdated) {
				tx_req->flags |= gPhyUpdHDr;

#if CONFIG_OPENTHREAD_CSL_RECEIVER
				/* Previously aFrame->mInfo.mTxInfo.mCslPresent was used to
				 * determine if the radio code should update the IE header. This
				 * field is no longer set by the OT stack. Until the issue is fixed
				 * in OT stack check if CSL period is > 0 and always update CSL IE
				 * in that case.
				 */
				if (radio_csl_period) {
					uint32_t hdrTimeUs;

					start_csl_receiver();

					/*
					 * Add TX_ENCRYPT_DELAY_SYM symbols delay to allow
					 * encryption to finish.
					 */
					tx_req->startTime =
						PhyTime_ReadClock() + TX_ENCRYPT_DELAY_SYM;

					hdrTimeUs =
						otPlatTimeGet() + (TX_ENCRYPT_DELAY_SYM +
								   IEEE802154_PHY_SHR_LEN_SYM) *
									  IEEE802154_SYMBOL_TIME_US;
					otMacFrameSetCslIe(aFrame, radio_csl_period,
							   rf_compute_csl_phase(hdrTimeUs));

					is_tx_poll = FALSE;
				}
#endif
			}
		}
	}
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
	otError status = OT_ERROR_NONE;
	/*
	 * radio_tx_data buffer has reserved memory for both macToPdDataMessage_t
	 * and actual data frame after.
	 */
	macToPdDataMessage_t *msg = (macToPdDataMessage_t *)radio_tx_data;
	phyStatus_t phy_status;
	otRadioState tmp_state;

	otEXPECT_ACTION((radio_state != OT_RADIO_STATE_DISABLED), status = OT_ERROR_INVALID_STATE);

	is_tx_poll = otMacFrameIsDataRequest(aFrame);
	is_rx_after_poll = FALSE;

	msg->msgType = gPdDataReq_c;

	set_tx_req(&msg->msgData.dataReq, aFrame);

	tmp_state = radio_state;
	radio_state = OT_RADIO_STATE_TRANSMIT;

	phy_status = MAC_PD_SapHandler(msg, ot_phy_ctx);
	if (phy_status == gPhySuccess_c) {
		otPlatRadioTxStarted(aInstance, aFrame);
	} else {
		radio_state = tmp_state;
		status = OT_ERROR_INVALID_STATE;
	}
exit:
	return status;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeGetReq_c;
	msg.msgData.getReq.PibAttribute = gPhyGetRSSILevel_c;
	msg.msgData.getReq.PibAttributeValue = 127; /* RSSI is invalid */

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	return (int8_t)msg.msgData.getReq.PibAttributeValue;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
	otRadioCaps caps = OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_SLEEP_TO_TX |
			   OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_RX_ON_WHEN_IDLE;

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
	caps |= OT_RADIO_CAPS_TRANSMIT_SEC | OT_RADIO_CAPS_TRANSMIT_TIMING |
		OT_RADIO_CAPS_RECEIVE_TIMING;
#endif

	return caps;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return radio_promisc_enable;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	radio_promisc_enable = aEnable;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibPromiscuousMode_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aEnable;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError status = OT_ERROR_NONE;
	phyStatus_t phy_status;
	macToPlmeMessage_t msg;

	otEXPECT_ACTION(
			((radio_state != OT_RADIO_STATE_TRANSMIT) &&
			(radio_state != OT_RADIO_STATE_DISABLED)),
			status = OT_ERROR_INVALID_STATE);

	otEXPECT_ACTION(!is_ed_scan, status = OT_ERROR_BUSY);

	radio_max_ed = -128;
	rf_set_channel(aScanChannel);

	msg.msgType = gPlmeEdReq_c;
	msg.msgData.edReq.startTime = gPhySeqStartAsap_c;
	msg.msgData.edReq.measureDurationSym = (aScanDuration * 1000) / OT_RADIO_SYMBOL_TIME;

	phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);
	if (phy_status != gPhySuccess_c) {
		status = OT_ERROR_INVALID_STATE;
	} else {
		is_ed_scan = TRUE;
	}
exit:
	return status;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
	otError error = OT_ERROR_NONE;

	otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
	*aPower = radio_auto_tx_pwr_lvl;

exit:
	return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
	OT_UNUSED_VARIABLE(aInstance);

	/* Set Power level for TX */
	rf_set_tx_power(aPower);
	radio_auto_tx_pwr_lvl = aPower;

	return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aThreshold);

	return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return -102;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
#if CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT
otError otPlatRadioConfigureEnhAckProbing(otInstance *aInstance, otLinkMetrics aLinkMetrics,
					  const otShortAddress aShortAddress,
					  const otExtAddress *aExtAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError error = OT_ERROR_NONE;
	uint32_t ieDataLen = 0;
	uint32_t ieParam = 0;
	otMacAddress macAddress;
	macToPlmeMessage_t msg;

	macAddress.mType = OT_MAC_ADDRESS_TYPE_SHORT;
	macAddress.mAddress.mShortAddress = aShortAddress;

	error = otLinkMetricsConfigureEnhAckProbing(aShortAddress, aExtAddress, aLinkMetrics);
	otEXPECT(error == OT_ERROR_NONE);

	ieDataLen = otLinkMetricsEnhAckGetDataLen(&macAddress);

	if (ieDataLen > 0) {
		ieDataLen = otMacFrameGenerateEnhAckProbingIe(msg.msgData.AckIeData.data, NULL,
							      ieDataLen);
	}

	ieParam = (aLinkMetrics.mLqi > 0 ? IeData_Lqi_c : 0) |
		  (aLinkMetrics.mLinkMargin > 0 ? IeData_LinkMargin_c : 0) |
		  (aLinkMetrics.mRssi > 0 ? IeData_Rssi_c : 0);

	/* If ieDataLen remains 0 we will delete the IE data */
	msg.msgType = gPlmeConfigureAckIeData_c;
	msg.msgData.AckIeData.param = (ieDataLen > 0 ? IeData_MSB_VALID_DATA : 0);
	msg.msgData.AckIeData.param |= ieParam;
	msg.msgData.AckIeData.shortAddr = aShortAddress;
	memcpy(msg.msgData.AckIeData.extAddr, aExtAddress, 8);

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

exit:
	return error;
}
#endif

void otPlatRadioSetMacKey(otInstance *aInstance, uint8_t aKeyIdMode, uint8_t aKeyId,
			  const otMacKeyMaterial *aPrevKey, const otMacKeyMaterial *aCurrKey,
			  const otMacKeyMaterial *aNextKey, otRadioKeyType aKeyType)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aKeyIdMode);
	macToPlmeMessage_t msg;

	assert(OT_KEY_TYPE_LITERAL_KEY == aKeyType || OT_KEY_TYPE_KEY_REF == aKeyType);
	assert(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

	msg.msgType = gPlmeSetMacKey_c;
	msg.msgData.MacKeyData.keyId = aKeyId;

	if (OT_KEY_TYPE_LITERAL_KEY == aKeyType) {
		memcpy(msg.msgData.MacKeyData.prevKey, aPrevKey, 16);
		memcpy(msg.msgData.MacKeyData.currKey, aCurrKey, 16);
		memcpy(msg.msgData.MacKeyData.nextKey, aNextKey, 16);
	}

#if (OPENTHREAD_CONFIG_CRYPTO_LIB == OPENTHREAD_CONFIG_CRYPTO_LIB_PSA)
	else if (OT_KEY_TYPE_KEY_REF == aKeyType) {
		size_t keyLength;
		otError status = OT_ERROR_NONE;

		status = otPlatCryptoExportKey(aPrevKey->mKeyMaterial.mKeyRef,
					       msg.msgData.MacKeyData.prevKey, 16, &keyLength);
		if (OT_ERROR_NONE != status || 16 != keyLength) {
			return;
		}

		status = otPlatCryptoExportKey(aCurrKey->mKeyMaterial.mKeyRef,
					       msg.msgData.MacKeyData.currKey, 16, &keyLength);
		if (OT_ERROR_NONE != status || 16 != keyLength) {
			return;
		}

		status = otPlatCryptoExportKey(aNextKey->mKeyMaterial.mKeyRef,
					       msg.msgData.MacKeyData.nextKey, 16, &keyLength);
		if (OT_ERROR_NONE != status || 16 != keyLength) {
			return;
		}
	}
#endif
	else {

	}

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetMacFrameCounter_c;
	msg.msgData.MacFrameCounter = aMacFrameCounter;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

void otPlatRadioSetMacFrameCounterIfLarger(otInstance *aInstance, uint32_t aMacFrameCounter)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetMacFrameCounterIfLarger_c;
	msg.msgData.MacFrameCounter = aMacFrameCounter;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return otPlatTimeGet();
}

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return PLATFORM_CSL_ACCURACY;
}

uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return CSL_UNCERT;
}

otError otPlatRadioReceiveAt(otInstance *aInstance, uint8_t aChannel, uint32_t aStart,
			     uint32_t aDuration)
{
	OT_UNUSED_VARIABLE(aInstance);
	macToPlmeMessage_t msg;
	otError status = OT_ERROR_NONE;

	otEXPECT_ACTION((radio_state == OT_RADIO_STATE_SLEEP), status = OT_ERROR_FAILED);

	start_csl_receiver();

	rf_set_channel(aChannel);

	aStart = rf_adjust_tstamp_from_ot(aStart);

	msg.msgType = gPlmeRxReq_c;
	msg.msgData.setTRxStateReq.state = gPhySetRxOn_c;
	msg.msgData.setTRxStateReq.rxDuration = aDuration / IEEE802154_SYMBOL_TIME_US;
	msg.msgData.setTRxStateReq.startTime = aStart / IEEE802154_SYMBOL_TIME_US;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

exit:
	return status;
}

#if CONFIG_OPENTHREAD_CSL_RECEIVER
otError otPlatRadioEnableCsl(otInstance *aInstance, uint32_t aCslPeriod, otShortAddress aShortAddr,
			     const otExtAddress *aExtAddr)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aExtAddr);
	OT_UNUSED_VARIABLE(aShortAddr);

	if (aCslPeriod) {
		rf_set_rx_time_poll(0);
	} else {
		rf_set_rx_time_poll(RX_TIME_POLL);
	}

	radio_csl_period = aCslPeriod;

	macToPlmeMessage_t msg;

	msg.msgType = gPlmeCslEnable_c;
	msg.msgData.cslPeriod = aCslPeriod;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	return OT_ERROR_NONE;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
	OT_UNUSED_VARIABLE(aInstance);

	radio_csl_sample_time_us = aCslSampleTime;
}

static void set_csl_sample_time(void)
{
	if (radio_csl_period == 0) {
		return;
	}

	macToPlmeMessage_t msg;
	uint32_t csl_period = radio_csl_period * 10 * IEEE802154_SYMBOL_TIME_US;
	uint32_t dt = radio_csl_sample_time_us - (uint32_t)otPlatTimeGet();

	/* next channel sample should be in the future */
	while ((dt <= CMP_OVHD) || (dt > (CMP_OVHD + 2 * csl_period))) {
		radio_csl_sample_time_us += csl_period;
		dt = radio_csl_sample_time_us - (uint32_t)otPlatTimeGet();
	}

	/* The CSL sample time is in microseconds and PHY function expects also microseconds */
	msg.msgType = gPlmeCslSetSampleTime_c;
	msg.msgData.cslSampleTime = rf_adjust_tstamp_from_ot(radio_csl_sample_time_us);

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void start_csl_receiver(void)
{
	if (radio_csl_period == 0) {
		return;
	}
	/*
	 * NBU has to be awake during CSL receiver trx  so that conversion from
	 * PHY timebase (NBU) to TMR timebase (host) is valid.
	 */
	if (!csl_rx) {
		PLATFORM_RemoteActiveReq();

		csl_rx = TRUE;
	}

	/* sample time is converted to PHY time */
	set_csl_sample_time();
}

static void stop_csl_receiver(void)
{
	if (csl_rx) {
		PLATFORM_RemoteActiveRel();

		csl_rx = FALSE;
	}
}
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVER */

/*
 * Compute the CSL Phase for the aTimeUs - i.e. the time from the aTimeUs
 * to radio_csl_sample_time_us.
 * The assumption is that radio_csl_sample_time_us > aTimeUs.
 * Since the time is kept with a limited timer in
 * reality it means that sometimes radio_csl_sample_time_us < aTimeUs,
 *  when the timer overflows. Therefore the formula should be:
 *
 * if (aTimeUs <= radio_csl_sample_time_us)
 *         cslPhaseUs = radio_csl_sample_time_us - aTimeUs;
 * else
 *         cslPhaseUs = MAX_TIMER_VALUE - aTimeUs + radio_csl_sample_time_us;
 *
 * For simplicity the formula below has been used.
 */
static uint16_t rf_compute_csl_phase(uint32_t aTimeUs)
{
	/* convert CSL Period in microseconds - it was given in 10 symbols */
	uint32_t cslPeriodUs = radio_csl_period * 10 * IEEE802154_SYMBOL_TIME_US;
	uint32_t cslPhaseUs =
		(cslPeriodUs - (aTimeUs % cslPeriodUs) + (radio_csl_sample_time_us % cslPeriodUs)) %
		cslPeriodUs;

	return (uint16_t)(cslPhaseUs / (10 * IEEE802154_SYMBOL_TIME_US) + 1);
}
#endif /* OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2 */

/*************************************************************************************************/
static void rf_abort(void)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetTRxStateReq_c;
	msg.msgData.setTRxStateReq.state = gPhyForceTRxOff_c;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void rf_set_channel(uint8_t channel)
{
	/* change channel only if it is different */
	if (channel != radio_channel) {
		macToPlmeMessage_t msg;

		msg.msgType = gPlmeSetReq_c;
		msg.msgData.setReq.PibAttribute = gPhyPibCurrentChannel_c;
		msg.msgData.setReq.PibAttributeValue = (uint64_t)channel;

		(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

		radio_channel = channel;
	}
}

static void rf_set_tx_power(int8_t tx_power)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibTransmitPower_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)tx_power;

	MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}
/*
 * Used to convert from phy clock timestamp (in symbols) to platform time (in us)
 * the reception timestamp which must use a true 64bit timestamp source.
 */
static uint64_t rf_adjust_tstamp_from_phy(uint64_t ts)
{
	uint64_t now = PhyTime_ReadClock();
	uint64_t delta;

	delta = (now >= ts) ? (now - ts) : ((PHY_TMR_MAX_VALUE + now) - ts);
	delta *= IEEE802154_SYMBOL_TIME_US;

	return otPlatTimeGet() - delta;
}

static uint32_t rf_adjust_tstamp_from_ot(uint32_t time)
{
	/* The phy timestamp is in symbols so we need to convert it to microseconds */
	uint64_t ts = PhyTime_ReadClock() * IEEE802154_SYMBOL_TIME_US;
	uint32_t delta = time - (uint32_t)otPlatTimeGet();

	return (uint32_t)(ts + delta);
}

static void convert_ack(otRadioFrame *ack, pdDataCnf_t *cnf)
{
	ack->mChannel = radio_channel;
	ack->mLength = cnf->ackLength;
	ack->mInfo.mRxInfo.mLqi = cnf->ppduLinkQuality;
	ack->mInfo.mRxInfo.mRssi = cnf->ppduRssi;
	memcpy(ack->mPsdu, cnf->ackData, ack->mLength);
}

static void convert_rx_frame(otRadioFrame *rx_frm, pdDataInd_t *rx_ind)
{
	/* Retrieve frame information and data */
	rx_frm->mChannel = radio_channel;
	rx_frm->mInfo.mRxInfo.mLqi = rx_ind->ppduLinkQuality;
	rx_frm->mInfo.mRxInfo.mRssi = rx_ind->ppduRssi;
	rx_frm->mInfo.mRxInfo.mTimestamp = rf_adjust_tstamp_from_phy(rx_ind->timeStamp);
	rx_frm->mInfo.mRxInfo.mAckedWithFramePending = rx_ind->rxAckFp;
	rx_frm->mLength = rx_ind->psduLength;
	rx_frm->mPsdu = rx_ind->pPsdu;
	rx_frm->mInfo.mRxInfo.mAckedWithSecEnhAck = rx_ind->ackedWithSecEnhAck;

	if (true == rx_ind->ackedWithSecEnhAck) {
		rx_frm->mInfo.mRxInfo.mAckFrameCounter = rx_ind->ackFrameCounter;
		rx_frm->mInfo.mRxInfo.mAckKeyId = rx_ind->ackKeyId;
	}
}
/*
 * Phy Data Service Access Point handler
 * Called by Phy to notify when Tx has been done or Rx data is available.
 */
phyStatus_t PD_OT_MAC_SapHandler(void *pMsg, instanceId_t instance)
{
	pdDataToMacMessage_t *pDataMsg = (pdDataToMacMessage_t *)pMsg;
	extended_radio_frame *pRxFrame = NULL;

	assert(pMsg != NULL);

	switch (pDataMsg->msgType) {
	case gPdDataInd_c:
		if (is_rx_after_poll) {
			if (!(pDataMsg->msgData.dataInd.pPsdu[IEEE802154_FRM_CTL_LO_OFFSET] &
			      IEEE802154_ACK_REQUEST)) {
				/* ignore broadcast packets */
				k_free(pMsg);

				stop_csl_receiver();
				return gPhySuccess_c;
			}

			rx_after_poll_done = TRUE;
		}

		is_rx_after_poll = FALSE;

		/* RX activity is done */
		pRxFrame = get_free_frame_rx_ring();

		if (pRxFrame) {
			convert_rx_frame(&pRxFrame->RxFrame, &pDataMsg->msgData.dataInd);
			/*
			 * Keep Phy Allocated buffer and free it after OT process the packet in
			 * otPlatRadioProcess after otPlatRadioReceiveDone is completed.
			 */
			pRxFrame->pPhyBuffer = (void *)pMsg;

			/* Push received frame */
			push_free_frame_rx_ring();
		} else {
			k_free(pMsg);
		}

		if (!rx_on_idle_disabled && is_rx_on_idle && (!pRxFrame || is_rx_ring_full())) {
			/* no room to receive more */
			rf_set_rx_on_idle(FALSE);
			rx_on_idle_disabled = TRUE;
		}
		break;

	case gPdDataCnf_c:
	default:
		if (radio_state == OT_RADIO_STATE_TRANSMIT) {
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
			if (otMacFrameIsSecurityEnabled(&radio_tx_frame) &&
			    otMacFrameIsKeyIdMode1(&radio_tx_frame) &&
			    !radio_tx_frame.mInfo.mTxInfo.mIsSecurityProcessed &&
			    !radio_tx_frame.mInfo.mTxInfo.mIsHeaderUpdated) {
				otMacFrameSetFrameCounter(&radio_tx_frame, pDataMsg->fc);
			}
#endif
			radio_tx_status = OT_ERROR_ABORT;

			if (pDataMsg->msgType == gPdDataCnf_c) {
				radio_tx_status = OT_ERROR_NONE;

				convert_ack(&radio_rx_ack_frame, &pDataMsg->msgData.dataCnf);

				if (is_tx_poll &&
				    (radio_rx_ack_frame.mPsdu[IEEE802154_FRM_CTL_LO_OFFSET] &
				     IEEE802154_FP)) {
					is_rx_after_poll = TRUE;
				}
			}

			/* TX activity is done */
			radio_tx_done = TRUE;
			is_tx_poll = FALSE;
		}
		/* for Ack we can free PHY Allocated buffer */
		k_free(pMsg);
		break;
	}

	if (is_ed_scan) {
		/* Scan done */
		radio_ed_scan_done = TRUE;
		is_ed_scan = FALSE;
	}

	if (radio_tx_done || (radio_state != OT_RADIO_STATE_TRANSMIT)) {
		if (is_rx_on_idle || is_rx_after_poll) {
			radio_state = OT_RADIO_STATE_RECEIVE;

			if (radio_tx_done) {
				rf_set_channel(radio_tx_frame.mInfo.mTxInfo.mRxChannelAfterTxDone);
			}
		} else {
			radio_state = OT_RADIO_STATE_SLEEP;
		}
	}

	stop_csl_receiver();

	OSA_InterruptDisable();
	if (!is_radio_event) {
		is_radio_event = TRUE;
	}
	OSA_InterruptEnable();

	otSysEventSignalPending();
	return gPhySuccess_c;
}
/*
 * Phy Layer Management Entities Service Access Point handler.
 * Called by Phy to notify PLME event.
 */
phyStatus_t PLME_OT_MAC_SapHandler(void *pMsg, instanceId_t instance)
{
	plmeToMacMessage_t *pPlmeMsg = (plmeToMacMessage_t *)pMsg;

	assert(pMsg != NULL);

	switch (pPlmeMsg->msgType) {
	case gPlmeCcaCnf_c:
		if (pPlmeMsg->msgData.ccaCnf.status == gPhyChannelBusy_c) {
			/* Channel is busy */
			radio_tx_status = OT_ERROR_CHANNEL_ACCESS_FAILURE;
			radio_tx_done = TRUE;
		}
		break;

	case gPlmeEdCnf_c:
		radio_max_ed = pPlmeMsg->msgData.edCnf.maxEnergyLeveldB;

	case gPlmeTimeoutInd_c:
	case gPlmeAbortInd_c:
	default:
		if (OT_RADIO_STATE_TRANSMIT == radio_state) {
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
			if (otMacFrameIsSecurityEnabled(&radio_tx_frame) &&
			    otMacFrameIsKeyIdMode1(&radio_tx_frame) &&
			    !radio_tx_frame.mInfo.mTxInfo.mIsSecurityProcessed &&
			    !radio_tx_frame.mInfo.mTxInfo.mIsHeaderUpdated) {
				otMacFrameSetFrameCounter(&radio_tx_frame, pPlmeMsg->fc);
			}
#endif
			radio_tx_status = OT_ERROR_ABORT;

			if (pPlmeMsg->msgType == gPlmeTimeoutInd_c) {
				radio_tx_status = OT_ERROR_NO_ACK; /* Ack timeout */
			}
			radio_tx_done = TRUE;
			is_tx_poll = FALSE;
			is_rx_after_poll = FALSE;
		}
		break;
	}

	/* The message has been allocated by the Phy, we have to free it */
	k_free(pMsg);

	if (is_ed_scan) {
		/* Scan done */
		radio_ed_scan_done = TRUE;
		is_ed_scan = FALSE;
	}

	if (is_rx_on_idle) {
		radio_state = OT_RADIO_STATE_RECEIVE;

		if (radio_tx_done) {
			rf_set_channel(radio_tx_frame.mInfo.mTxInfo.mRxChannelAfterTxDone);
		}
	} else {
		radio_state = OT_RADIO_STATE_SLEEP;
	}

	stop_csl_receiver();

	if (radio_tx_done || radio_ed_scan_done) {
		OSA_InterruptDisable();
		if (!is_radio_event) {
			is_radio_event = TRUE;
		}
		OSA_InterruptEnable();

		otSysEventSignalPending();
	}
	return gPhySuccess_c;
}

void platformRadioInit(void)
{
	macToPlmeMessage_t msg;

	PLATFORM_InitOT();

	Phy_Init();

	ot_phy_ctx = PHY_get_ctx();
	/*
	 * Register Phy Data Service Access Point (PD_SAP) and
	 * Phy Layer Management Entities Service Access Point (PLME_SAP)
	 * handlers
	 */
	Phy_RegisterSapHandlers((PD_MAC_SapHandler_t)PD_OT_MAC_SapHandler,
				(PLME_MAC_SapHandler_t)PLME_OT_MAC_SapHandler, ot_phy_ctx);

	msg.msgType = gPlmeEnableEncryption_c;
	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	rf_set_rx_time_poll(RX_TIME_POLL);

	radio_tx_frame.mLength = 0;
	/*
	 * Make the mPsdu point to the space after macToPdDataMessage_t in the data buffer.
	 * OT will put the data packet in this zone when requesting a data transmission.
	 */
	radio_tx_frame.mPsdu = radio_tx_data + sizeof(macToPdDataMessage_t);

	memset(&radio_rx_ack_frame, 0, sizeof(radio_rx_ack_frame));
	radio_rx_ack_frame.mPsdu = radio_rx_ack_data;
}

static void radio_rx_process(otInstance *aInstance)
{
	extended_radio_frame *f;
#if CONFIG_OPENTHREAD_DIAG
	bool diag_mode = otPlatDiagModeGet();
#endif

	while ((f = get_frame_rx_ring()) != NULL) {
#if CONFIG_OPENTHREAD_DIAG
		if (diag_mode) {
			otPlatDiagRadioReceiveDone(aInstance, &f->RxFrame, OT_ERROR_NONE);
		} else {
			otPlatRadioReceiveDone(aInstance, &f->RxFrame, OT_ERROR_NONE);
		}
#else
		otPlatRadioReceiveDone(aInstance, &f->RxFrame, OT_ERROR_NONE);
#endif
		/* free PHY Allocated buffer */
		k_free(f->pPhyBuffer);
		f->pPhyBuffer = NULL;

		pop_frame_rx_ring();
	}

	if (rx_on_idle_disabled) {
		rf_set_rx_on_idle(TRUE);
		rx_on_idle_disabled = FALSE;
	}
}

void platformRadioProcess(otInstance *aInstance)
{
	OSA_InterruptDisable();
	if (is_radio_event) {
		is_radio_event = FALSE;
	}
	OSA_InterruptEnable();

	if (radio_tx_done) {
		if (radio_tx_frame.mPsdu[IEEE802154_FRM_CTL_LO_OFFSET] & IEEE802154_ACK_REQUEST) {
			otPlatRadioTxDone(aInstance, &radio_tx_frame, &radio_rx_ack_frame,
					  radio_tx_status);
		} else {
			otPlatRadioTxDone(aInstance, &radio_tx_frame, NULL, radio_tx_status);
		}

		radio_tx_done = FALSE;
	}

	rx_after_poll_done = FALSE;

	radio_rx_process(aInstance);

	if (radio_ed_scan_done) {
		otPlatRadioEnergyScanDone(aInstance, radio_max_ed);
		radio_ed_scan_done = FALSE;
	}
}

static void rf_set_rx_on_idle(bool_t val)
{
	macToPlmeMessage_t msg;
	phyStatus_t phy_status;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibRxOnWhenIdle;
	msg.msgData.setReq.PibAttributeValue = val;

	phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	assert(phy_status == gPhySuccess_c);
	OT_UNUSED_VARIABLE(phy_status);
}

static void rf_set_rx_time_poll(uint32_t t)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibRxTimePoll_c;
	msg.msgData.setReq.PibAttributeValue = t;

	MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

uint64_t otPlatTimeGet(void)
{
	return mcxw_get_time_ns() / NSEC_PER_USEC;
}
