/* ieee802154_mcxw.c - NXP MCXW 802.15.4 driver */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcxw_ieee802154

#define LOG_MODULE_NAME ieee802154_mcxw

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio_openthread.h>
#endif

#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>

#include <zephyr/net/ieee802154_radio.h>

#include <soc.h>
#include <errno.h>
#include <string.h>

#include "EmbeddedTypes.h"
#include "Phy.h"

#include "fwk_platform_ot.h"

#include "ieee802154_mcxw.h"
#include "ieee802154_mcxw_utils.h"

void PLATFORM_RemoteActiveReq(void);
void PLATFORM_RemoteActiveRel(void);

#if CONFIG_IEEE802154_CSL_ENDPOINT

#define CMP_OVHD (4 * IEEE802154_SYMBOL_TIME_US) /* 2 LPTRM (32 kHz) ticks */

static bool_t csl_rx = FALSE;

static void set_csl_sample_time(void);
static void start_csl_receiver(void);
static void stop_csl_receiver(void);
static uint16_t rf_compute_csl_phase(uint32_t aTimeUs);

#else /* CONFIG_IEEE802154_CSL_ENDPOINT */
#define start_csl_receiver()
#define stop_csl_receiver()
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */

static volatile uint32_t sun_rx_mode = RX_ON_IDLE_START;

/* Private functions */
static void rf_abort(void);
static void rf_set_channel(uint8_t channel);
static void rf_set_tx_power(int8_t tx_power);
static uint64_t rf_adjust_tstamp_from_phy(uint64_t ts);

#if CONFIG_IEEE802154_CSL_ENDPOINT || CONFIG_NET_PKT_TXTIME
static uint32_t rf_adjust_tstamp_from_app(uint32_t time);
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT || CONFIG_NET_PKT_TXTIME */

static void rf_rx_on_idle(uint32_t newValue);

static uint8_t ot_phy_ctx = (uint8_t)(-1);

static struct mcxw_context mcxw_ctx;

/**
 * Stub function used for controlling low power mode
 */
WEAK void app_allow_device_to_slepp(void)
{
}

/**
 * Stub function used for controlling low power mode
 */
WEAK void app_disallow_device_to_slepp(void)
{
}

void mcxw_get_eui64(uint8_t *eui64)
{
	__ASSERT_NO_MSG(eui64);

	/* PLATFORM_GetIeee802_15_4Addr(); */
	sys_rand_get(eui64, sizeof(mcxw_ctx.mac));

	eui64[0] = (eui64[0] & ~0x01) | 0x02;
}

static int mcxw_set_pan_id(const struct device *dev, uint16_t aPanId)
{
	struct mcxw_context *mcxw_radio = dev->data;

	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibPanId_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aPanId;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	mcxw_radio->pan_id = aPanId;

	return 0;
}

static int mcxw_set_extended_address(const struct device *dev, const uint8_t *ieee_addr)
{
	struct mcxw_context *mcxw_radio = dev->data;

	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibLongAddress_c;
	msg.msgData.setReq.PibAttributeValue = *(uint64_t *)ieee_addr;

	memcpy(mcxw_radio->mac, ieee_addr, 8);

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	return 0;
}

static int mcxw_set_short_address(const struct device *dev, uint16_t aShortAddress)
{
	ARG_UNUSED(dev);

	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibShortAddress_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aShortAddress;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	return 0;
}

static int mcxw_filter(const struct device *dev, bool set, enum ieee802154_filter_type type,
		       const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return mcxw_set_extended_address(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return mcxw_set_short_address(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return mcxw_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

void mcxw_radio_receive(void)
{
	macToPlmeMessage_t msg;
	phyStatus_t phy_status;

	app_disallow_device_to_slepp();

	__ASSERT(mcxw_ctx.state != RADIO_STATE_DISABLED, "Radio RX invalid state");

	mcxw_ctx.state = RADIO_STATE_RECEIVE;

	rf_abort();
	rf_set_channel(mcxw_ctx.channel);

	if (sun_rx_mode) {
		start_csl_receiver();

		/* restart Rx on idle only if it was enabled */
		msg.msgType = gPlmeSetReq_c;
		msg.msgData.setReq.PibAttribute = gPhyPibRxOnWhenIdle;
		msg.msgData.setReq.PibAttributeValue = (uint64_t)1;

		phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);
		__ASSERT_NO_MSG(phy_status == gPhySuccess_c);
	}
}

static uint8_t mcxw_get_acc(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IEEE802154_MCXW_CSL_ACCURACY;
}

static int mcxw_start(const struct device *dev)
{
	struct mcxw_context *mcxw_radio = dev->data;

	__ASSERT(mcxw_radio->state == RADIO_STATE_DISABLED, "%s", __func__);

	mcxw_radio->state = RADIO_STATE_SLEEP;

	rf_rx_on_idle(RX_ON_IDLE_START);

	mcxw_radio_receive();

	return 0;
}

static int mcxw_stop(const struct device *dev)
{
	struct mcxw_context *mcxw_radio = dev->data;

	__ASSERT(mcxw_radio->state != RADIO_STATE_DISABLED, "%s", __func__);

	stop_csl_receiver();

	mcxw_radio->state = RADIO_STATE_DISABLED;

	return 0;
}

void mcxw_radio_sleep(void)
{
	__ASSERT_NO_MSG(((mcxw_ctx.state != RADIO_STATE_TRANSMIT) &&
			 (mcxw_ctx.state != RADIO_STATE_DISABLED)));

	rf_abort();

	stop_csl_receiver();

	app_allow_device_to_slepp();

	mcxw_ctx.state = RADIO_STATE_SLEEP;
}

static void mcxw_enable_src_match(bool enable)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetSAMState_c;
	msg.msgData.SAMState = enable;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static int mcxw_src_match_entry(bool extended, uint8_t *address)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeAddToSapTable_c;
	msg.msgData.deviceAddr.panId = mcxw_ctx.pan_id;

	if (extended) {
		msg.msgData.deviceAddr.mode = 3;
		memcpy(msg.msgData.deviceAddr.addr, address, 8);
	} else {
		msg.msgData.deviceAddr.mode = 2;
		memcpy(msg.msgData.deviceAddr.addr, address, 2);
	}

	if (gPhySuccess_c != MAC_PLME_SapHandler(&msg, ot_phy_ctx)) {
		/* the status is not returned from PHY over RPMSG */
		return -ENOMEM;
	}

	return 0;
}

static int mcxw_src_clear_entry(bool extended, uint8_t *address)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeRemoveFromSAMTable_c;
	msg.msgData.deviceAddr.panId = mcxw_ctx.pan_id;

	if (extended) {
		msg.msgData.deviceAddr.mode = 3;
		memcpy(msg.msgData.deviceAddr.addr, address, 8);
	} else {
		msg.msgData.deviceAddr.mode = 2;
		memcpy(msg.msgData.deviceAddr.addr, address, 2);
	}

	if (gPhySuccess_c != MAC_PLME_SapHandler(&msg, ot_phy_ctx)) {
		/* the status is not returned from PHY over RPMSG */
		return -ENOENT;
	}

	return 0;
}

static int handle_ack(struct mcxw_context *mcxw_radio)
{
	uint8_t len;
	struct net_pkt *pkt;
	int err = 0;

	len = mcxw_radio->rx_ack_frame.length;
	pkt = net_pkt_rx_alloc_with_buffer(mcxw_radio->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No free packet available.");
		err = -ENOMEM;
		goto exit;
	}

	if (net_pkt_write(pkt, mcxw_radio->rx_ack_frame.psdu, len) < 0) {
		LOG_ERR("Failed to write to a packet.");
		err = -ENOMEM;
		goto free_ack;
	}

	/* Use some fake values for LQI and RSSI. */
	net_pkt_set_ieee802154_lqi(pkt, 80);
	net_pkt_set_ieee802154_rssi_dbm(pkt, -40);

	net_pkt_set_timestamp_ns(pkt, mcxw_radio->rx_ack_frame.timestamp);

	net_pkt_cursor_init(pkt);

	if (ieee802154_handle_ack(mcxw_radio->iface, pkt) != NET_OK) {
		LOG_ERR("ACK packet not handled - releasing.");
	}

free_ack:
	net_pkt_unref(pkt);

exit:
	mcxw_radio->rx_ack_frame.length = 0;
	return err;
}

static int mcxw_tx(const struct device *dev, enum ieee802154_tx_mode mode, struct net_pkt *pkt,
		   struct net_buf *frag)
{
	struct mcxw_context *mcxw_radio = dev->data;
	/* tx_data buffer has reserved memory for both macToPdDataMessage_t and actual data frame
	 * after
	 */
	macToPdDataMessage_t *msg = (macToPdDataMessage_t *)mcxw_radio->tx_data;
	phyStatus_t phy_status;

	uint8_t payload_len = frag->len;
	uint8_t *payload = frag->data;

	app_disallow_device_to_slepp();

	__ASSERT(mcxw_radio->state != RADIO_STATE_DISABLED, "%s: radio disabled", __func__);

	if (payload_len > IEEE802154_MTU) {
		LOG_ERR("Payload too large: %d", payload_len);
		return -EMSGSIZE;
	}

	mcxw_radio->tx_frame.length = payload_len + IEEE802154_FCS_LENGTH;
	memcpy(mcxw_radio->tx_frame.psdu, payload, payload_len);

	mcxw_radio->tx_frame.sec_processed = net_pkt_ieee802154_frame_secured(pkt);
	mcxw_radio->tx_frame.hdr_updated = net_pkt_ieee802154_mac_hdr_rdy(pkt);

	rf_set_channel(mcxw_radio->channel);

	msg->msgType = gPdDataReq_c;
	msg->msgData.dataReq.slottedTx = gPhyUnslottedMode_c;
	msg->msgData.dataReq.psduLength = mcxw_radio->tx_frame.length;
	msg->msgData.dataReq.CCABeforeTx = gPhyNoCCABeforeTx_c;
	msg->msgData.dataReq.startTime = gPhySeqStartAsap_c;

	/* tx_frame.psdu will point to tx_frame.tx_data data buffer after macToPdDataMessage_t
	 * structure
	 */
	msg->msgData.dataReq.pPsdu = mcxw_radio->tx_frame.psdu;

	if (ieee802154_is_ar_flag_set(frag)) {
		msg->msgData.dataReq.ackRequired = gPhyRxAckRqd_c;
		/* The 3 bytes are 1 byte frame length and 2 bytes FCS */
		msg->msgData.dataReq.txDuration =
			IEEE802154_CCA_LEN_SYM + IEEE802154_PHY_SHR_LEN_SYM +
			(3 + mcxw_radio->tx_frame.length) * RADIO_SYMBOLS_PER_OCTET +
			IEEE802154_TURNAROUND_LEN_SYM;

		if (is_frame_version_2015(frag->data, frag->len)) {
			/* Because enhanced ack can be of variable length we need to set the timeout
			 * value to account for the FCF and addressing fields only, and stop the
			 * timeout timer after they are received and validated as a valid ACK
			 */
			msg->msgData.dataReq.txDuration += IEEE802154_ENH_ACK_WAIT_SYM;
		} else {
			msg->msgData.dataReq.txDuration += IEEE802154_IMM_ACK_WAIT_SYM;
		}
	} else {
		msg->msgData.dataReq.ackRequired = gPhyNoAckRqd_c;
		msg->msgData.dataReq.txDuration = 0xFFFFFFFFU;
	}

	switch (mode) {
	case IEEE802154_TX_MODE_DIRECT:
		msg->msgData.dataReq.CCABeforeTx = gPhyNoCCABeforeTx_c;
		break;
	case IEEE802154_TX_MODE_CCA:
		msg->msgData.dataReq.CCABeforeTx = gPhyCCAMode1_c;
		break;

#if defined(CONFIG_NET_PKT_TXTIME)
	case IEEE802154_TX_MODE_TXTIME:
	case IEEE802154_TX_MODE_TXTIME_CCA:
		mcxw_radio->tx_frame.tx_delay = net_pkt_timestamp_ns(pkt);
		msg->msgData.dataReq.startTime =
			rf_adjust_tstamp_from_app(mcxw_radio->tx_frame.tx_delay);
		msg->msgData.dataReq.startTime /= IEEE802154_SYMBOL_TIME_US;
		break;
#endif
	default:
		break;
	}

	msg->msgData.dataReq.flags = 0;

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
	if (is_keyid_mode_1(mcxw_radio->tx_frame.psdu, mcxw_radio->tx_frame.length)) {
		if (!net_pkt_ieee802154_frame_secured(pkt)) {
			msg->msgData.dataReq.flags |= gPhyEncFrame;

			if (!net_pkt_ieee802154_mac_hdr_rdy(pkt)) {
				msg->msgData.dataReq.flags |= gPhyUpdHDr;

#if CONFIG_IEEE802154_CSL_ENDPOINT
				/* Previously aFrame->mInfo.mTxInfo.mCslPresent was used to
				 * determine if the radio code should update the IE header. This
				 * field is no longer set by the OT stack. Until the issue is fixed
				 * in OT stack check if CSL period is > 0 and always update CSL IE
				 * in that case.
				 */
				if (mcxw_radio->csl_period) {
					uint32_t hdr_time_us;

					start_csl_receiver();

					/* Add TX_ENCRYPT_DELAY_SYM symbols delay to allow
					 * encryption to finish
					 */
					msg->msgData.dataReq.startTime =
						PhyTime_ReadClock() + TX_ENCRYPT_DELAY_SYM;

					hdr_time_us = mcxw_get_time(NULL) +
						    (TX_ENCRYPT_DELAY_SYM +
						     IEEE802154_PHY_SHR_LEN_SYM) *
							    IEEE802154_SYMBOL_TIME_US;
					set_csl_ie(mcxw_radio->tx_frame.psdu,
						 mcxw_radio->tx_frame.length,
						 mcxw_radio->csl_period,
						 rf_compute_csl_phase(hdr_time_us));
				}
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */
			}
		}
	}

#endif

	k_sem_reset(&mcxw_radio->tx_wait);

	phy_status = MAC_PD_SapHandler(msg, ot_phy_ctx);
	if (phy_status == gPhySuccess_c) {
		mcxw_radio->tx_status = 0;
		mcxw_radio->state = RADIO_STATE_TRANSMIT;
	} else {
		return -EIO;
	}

	k_sem_take(&mcxw_radio->tx_wait, K_FOREVER);

	/* PWR_AllowDeviceToSleep(); */

	mcxw_radio_receive();

	switch (mcxw_radio->tx_status) {
	case 0:
		if (mcxw_radio->rx_ack_frame.length) {
			return handle_ack(mcxw_radio);
		}
		return 0;

	default:
		return -(mcxw_radio->tx_status);
	}
}

void mcxw_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct mcxw_context *mcxw_radio = (struct mcxw_context *)arg1;
	struct net_pkt *pkt;
	struct mcxw_rx_frame rx_frame;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		pkt = NULL;

		LOG_DBG("Waiting for frame");

		if (k_msgq_get(&mcxw_radio->rx_msgq, &rx_frame, K_FOREVER) < 0) {
			LOG_ERR("Failed to get RX data from message queue");
			continue;
		}

		pkt = net_pkt_rx_alloc_with_buffer(mcxw_radio->iface, rx_frame.length, AF_UNSPEC, 0,
						   K_FOREVER);

		if (net_pkt_write(pkt, rx_frame.psdu, rx_frame.length)) {
			goto drop;
		}

		net_pkt_set_ieee802154_lqi(pkt, rx_frame.lqi);
		net_pkt_set_ieee802154_rssi_dbm(pkt, rx_frame.rssi);
		net_pkt_set_ieee802154_ack_fpb(pkt, rx_frame.ack_fpb);

#if defined(CONFIG_NET_PKT_TIMESTAMP)
		net_pkt_set_timestamp_ns(pkt, rx_frame.timestamp);
#endif

#if defined(CONFIG_NET_L2_OPENTHREAD)
		net_pkt_set_ieee802154_ack_seb(pkt, rx_frame.ack_seb);
#endif
		if (net_recv_data(mcxw_radio->iface, pkt) < 0) {
			LOG_ERR("Packet dropped by NET stack");
			goto drop;
		}

		k_free(rx_frame.phy_buffer);
		rx_frame.phy_buffer = NULL;

		/* restart rx on idle if enough space in message queue */
		if (k_msgq_num_free_get(&mcxw_radio->rx_msgq) >= 2) {
			rf_rx_on_idle(RX_ON_IDLE_START);
		}

		continue;

drop:
		/* PWR_AllowDeviceToSleep(); */
		net_pkt_unref(pkt);
	}
}

int8_t mcxw_get_rssi(void)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeGetReq_c;
	msg.msgData.getReq.PibAttribute = gPhyGetRSSILevel_c;
	msg.msgData.getReq.PibAttributeValue = 127; /* RSSI is invalid*/

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	return (int8_t)msg.msgData.getReq.PibAttributeValue;
}

void mcxw_set_promiscuous(bool aEnable)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibPromiscuousMode_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aEnable;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

void mcxw_set_pan_coord(bool aEnable)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibPanCoordinator_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)aEnable;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static int mcxw_energy_scan(const struct device *dev, uint16_t duration,
			    energy_scan_done_cb_t done_cb)
{

	int status = 0;
	phyStatus_t phy_status;
	macToPlmeMessage_t msg;

	app_disallow_device_to_slepp();

	struct mcxw_context *mcxw_radio = dev->data;

	__ASSERT_NO_MSG(((mcxw_radio->state != RADIO_STATE_TRANSMIT) &&
			 (mcxw_radio->state != RADIO_STATE_DISABLED)));

	rf_abort();

	rf_set_channel(mcxw_radio->channel);

	mcxw_radio->energy_scan_done = done_cb;

	msg.msgType = gPlmeEdReq_c;
	msg.msgData.edReq.startTime = gPhySeqStartAsap_c;
	msg.msgData.edReq.measureDurationSym = duration * 1000;

	phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);
	if (phy_status != gPhySuccess_c) {
		mcxw_radio->energy_scan_done = NULL;
		status = -EIO;
	}

	return status;
}

static int mcxw_set_txpower(const struct device *dev, int16_t dbm)
{
	struct mcxw_context *mcxw_radio = dev->data;

	LOG_DBG("%d", dbm);

	if (dbm != mcxw_radio->tx_pwr_lvl) {
		/* Set Power level for TX */
		rf_set_tx_power(dbm);
		mcxw_radio->tx_pwr_lvl = dbm;
	}

	return 0;
}

static void mcxw_configure_enh_ack_probing(const struct ieee802154_config *config)
{
	uint32_t ie_param = 0;
	macToPlmeMessage_t msg;

	uint8_t *header_ie_buf = (uint8_t *)(config->ack_ie.header_ie);

	ie_param = (header_ie_buf[6] == 0x03 ? IeData_Lqi_c : 0) |
		   (header_ie_buf[7] == 0x02 ? IeData_LinkMargin_c : 0) |
		   (header_ie_buf[8] == 0x01 ? IeData_Rssi_c : 0);

	msg.msgType = gPlmeConfigureAckIeData_c;
	msg.msgData.AckIeData.param = (ie_param > 0 ? IeData_MSB_VALID_DATA : 0);
	msg.msgData.AckIeData.param |= ie_param;
	msg.msgData.AckIeData.shortAddr = config->ack_ie.short_addr;
	memcpy(msg.msgData.AckIeData.extAddr, config->ack_ie.ext_addr, 8);
	memcpy(msg.msgData.AckIeData.data, config->ack_ie.header_ie,
	       config->ack_ie.header_ie->length);

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void mcxw_set_mac_key(struct ieee802154_key *mac_keys)
{
	macToPlmeMessage_t msg;

	__ASSERT_NO_MSG(mac_keys);
	__ASSERT_NO_MSG(mac_keys[0].key_id && mac_keys[0].key_value);
	__ASSERT_NO_MSG(mac_keys[1].key_id && mac_keys[1].key_value);
	__ASSERT_NO_MSG(mac_keys[2].key_id && mac_keys[2].key_value);

	msg.msgType = gPlmeSetMacKey_c;
	msg.msgData.MacKeyData.keyId = *(mac_keys[1].key_id);

	memcpy(msg.msgData.MacKeyData.prevKey, mac_keys[0].key_value, 16);
	memcpy(msg.msgData.MacKeyData.currKey, mac_keys[1].key_value, 16);
	memcpy(msg.msgData.MacKeyData.nextKey, mac_keys[2].key_value, 16);

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

void mcxw_set_mac_frame_counter(uint32_t frame_counter)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetMacFrameCounter_c;
	msg.msgData.MacFrameCounter = frame_counter;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

void mcxw_set_mac_frame_counter_if_larger(uint32_t frame_counter)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetMacFrameCounterIfLarger_c;
	msg.msgData.MacFrameCounter = frame_counter;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

#if CONFIG_IEEE802154_CSL_ENDPOINT

static void mcxw_receive_at(uint8_t channel, uint32_t start, uint32_t duration)
{
	macToPlmeMessage_t msg;

	__ASSERT_NO_MSG(mcxw_ctx.state == RADIO_STATE_SLEEP);
	mcxw_ctx.state = RADIO_STATE_RECEIVE;

	/* checks internally if the channel needs to be changed */
	rf_set_channel(mcxw_ctx.channel);

	start = rf_adjust_tstamp_from_app(start);

	msg.msgType = gPlmeSetTRxStateReq_c;
	msg.msgData.setTRxStateReq.slottedMode = gPhyUnslottedMode_c;
	msg.msgData.setTRxStateReq.state = gPhySetRxOn_c;
	msg.msgData.setTRxStateReq.rxDuration = duration / IEEE802154_SYMBOL_TIME_US;
	msg.msgData.setTRxStateReq.startTime = start / IEEE802154_SYMBOL_TIME_US;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void mcxw_enable_csl(uint16_t period)
{
	mcxw_ctx.csl_period = period;

	macToPlmeMessage_t msg;

	msg.msgType = gPlmeCslEnable_c;
	msg.msgData.cslPeriod = period;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void set_csl_sample_time(void)
{
	if (!mcxw_ctx.csl_period) {
		return;
	}

	macToPlmeMessage_t msg;
	uint32_t csl_period = mcxw_ctx.csl_period * 10 * IEEE802154_SYMBOL_TIME_US;
	uint32_t dt = mcxw_ctx.csl_sample_time - (uint32_t)mcxw_get_time(NULL);

	/* next channel sample should be in the future */
	while ((dt <= CMP_OVHD) || (dt > (CMP_OVHD + 2 * csl_period))) {
		mcxw_ctx.csl_sample_time += csl_period;
		dt = mcxw_ctx.csl_sample_time - (uint32_t)mcxw_get_time(NULL);
	}

	/* The CSL sample time is in microseconds and PHY function expects also microseconds */
	msg.msgType = gPlmeCslSetSampleTime_c;
	msg.msgData.cslSampleTime = rf_adjust_tstamp_from_app(mcxw_ctx.csl_sample_time);

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void start_csl_receiver(void)
{
	if (!mcxw_ctx.csl_period) {
		return;
	}

	/* NBU has to be awake during CSL receiver trx  so that conversion from
	 * PHY timebase (NBU) to TMR timebase (host) is valid
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

/*
 * Compute the CSL Phase for the time_us - i.e. the time from the time_us to
 * mcxw_ctx.csl_sample_time. The assumption is that mcxw_ctx.csl_sample_time > time_us. Since the
 * time is kept with a limited timer in reality it means that sometimes mcxw_ctx.csl_sample_time <
 * time_us, when the timer overflows. Therefore the formula should be:
 *
 * if (time_us <= mcxw_ctx.csl_sample_time)
 *         csl_phase_us = mcxw_ctx.csl_sample_time - time_us;
 * else
 *         csl_phase_us = MAX_TIMER_VALUE - time_us + mcxw_ctx.csl_sample_time;
 *
 * For simplicity the formula below has been used.
 */
static uint16_t rf_compute_csl_phase(uint32_t time_us)
{
	/* convert CSL Period in microseconds - it was given in 10 symbols */
	uint32_t csl_period_us = mcxw_ctx.csl_period * 10 * IEEE802154_SYMBOL_TIME_US;
	uint32_t csl_phase_us =
		(csl_period_us - (time_us % csl_period_us) +
		(mcxw_ctx.csl_sample_time % csl_period_us)) % csl_period_us;

	return (uint16_t)(csl_phase_us / (10 * IEEE802154_SYMBOL_TIME_US) + 1);
}
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */

/*************************************************************************************************/
static void rf_abort(void)
{
	macToPlmeMessage_t msg;

	sun_rx_mode = RX_ON_IDLE_START;
	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibRxOnWhenIdle;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)0;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	msg.msgType = gPlmeSetTRxStateReq_c;
	msg.msgData.setTRxStateReq.state = gPhyForceTRxOff_c;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static void rf_set_channel(uint8_t channel)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibCurrentChannel_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)channel;

	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

static int mcxw_cca(const struct device *dev)
{
	macToPlmeMessage_t msg;
	phyStatus_t phy_status;

	struct mcxw_context *mcxw_radio = dev->data;

	msg.msgType = gPlmeCcaReq_c;
	msg.msgData.ccaReq.ccaType = gPhyCCAMode1_c;
	msg.msgData.ccaReq.contCcaMode = gPhyContCcaDisabled;

	phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	__ASSERT_NO_MSG(phy_status == gPhySuccess_c);

	k_sem_take(&mcxw_radio->cca_wait, K_FOREVER);

	return (mcxw_radio->tx_status == OT_ERROR_CHANNEL_ACCESS_FAILURE) ? -EBUSY : 0;
}

static int mcxw_set_channel(const struct device *dev, uint16_t channel)
{
	struct mcxw_context *mcxw_radio = dev->data;

	LOG_DBG("%u", channel);

	if (channel != mcxw_radio->channel) {

		if (channel < 11 || channel > 26) {
			return channel < 11 ? -ENOTSUP : -EINVAL;
		}

		mcxw_radio->channel = channel;
	}

	return 0;
}

static net_time_t mcxw_get_time(const struct device *dev)
{
	static uint64_t sw_timestamp;
	static uint64_t hw_timestamp;

	ARG_UNUSED(dev);

	/* Get new 32bit HW timestamp */
	uint32_t ticks;
	uint64_t hw_timestamp_new;
	uint64_t wrapped_val = 0;
	uint64_t increment;
	unsigned int key;

	key = irq_lock();

	if (counter_get_value(mcxw_ctx.counter, &ticks)) {
		irq_unlock(key);
		return -1;
	}

	hw_timestamp_new = counter_ticks_to_us(mcxw_ctx.counter, ticks);

	/* Check if the timestamp has wrapped around */
	if (hw_timestamp > hw_timestamp_new) {
		wrapped_val =
			COUNT_TO_USEC(((uint64_t)1 << 32), counter_get_frequency(mcxw_ctx.counter));
	}

	increment = (hw_timestamp_new + wrapped_val) - hw_timestamp;
	sw_timestamp += increment;

	/* Store new HW timestamp for next iteration */
	hw_timestamp = hw_timestamp_new;

	irq_unlock(key);

	return (net_time_t)sw_timestamp;
}

static void rf_set_tx_power(int8_t tx_power)
{
	macToPlmeMessage_t msg;

	msg.msgType = gPlmeSetReq_c;
	msg.msgData.setReq.PibAttribute = gPhyPibTransmitPower_c;
	msg.msgData.setReq.PibAttributeValue = (uint64_t)tx_power;

	MAC_PLME_SapHandler(&msg, ot_phy_ctx);
}

/* Used to convert from phy clock timestamp (in symbols) to platform time (in us)
 * the reception timestamp which must use a true 64bit timestamp source
 */
static uint64_t rf_adjust_tstamp_from_phy(uint64_t ts)
{
	uint64_t now = PhyTime_ReadClock();
	uint64_t delta;

	delta = (now >= ts) ? (now - ts) : ((PHY_TMR_MAX_VALUE + now) - ts);
	delta *= IEEE802154_SYMBOL_TIME_US;

	return mcxw_get_time(NULL) - delta;
}

#if CONFIG_IEEE802154_CSL_ENDPOINT || CONFIG_NET_PKT_TXTIME
static uint32_t rf_adjust_tstamp_from_app(uint32_t time)
{
	/* The phy timestamp is in symbols so we need to convert it to microseconds */
	uint64_t ts = PhyTime_ReadClock() * IEEE802154_SYMBOL_TIME_US;
	uint32_t delta = time - (uint32_t)mcxw_get_time(NULL);

	return (uint32_t)(ts + delta);
}
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT || CONFIG_NET_PKT_TXTIME */

/* Phy Data Service Access Point handler
 * Called by Phy to notify when Tx has been done or Rx data is available
 */
phyStatus_t pd_mac_sap_handler(void *msg, instanceId_t instance)
{
	pdDataToMacMessage_t *data_msg = (pdDataToMacMessage_t *)msg;

	__ASSERT_NO_MSG(msg != NULL);

	/* PWR_DisallowDeviceToSleep(); */

	switch (data_msg->msgType) {
	case gPdDataCnf_c:
		/* TX is done */
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
		if (is_keyid_mode_1(mcxw_ctx.tx_frame.psdu, mcxw_ctx.tx_frame.length) &&
		    !mcxw_ctx.tx_frame.sec_processed && !mcxw_ctx.tx_frame.hdr_updated) {
			set_frame_counter(mcxw_ctx.tx_frame.psdu, mcxw_ctx.tx_frame.length,
					data_msg->fc);
			mcxw_ctx.tx_frame.hdr_updated = true;
		}
#endif

		mcxw_ctx.tx_frame.length = 0;
		mcxw_ctx.tx_status = 0;
		mcxw_ctx.state = RADIO_STATE_RECEIVE;

		mcxw_ctx.rx_ack_frame.channel = mcxw_ctx.channel;
		mcxw_ctx.rx_ack_frame.length = data_msg->msgData.dataCnf.ackLength;
		mcxw_ctx.rx_ack_frame.timestamp = data_msg->msgData.dataCnf.timeStamp;
		memcpy(mcxw_ctx.rx_ack_frame.psdu, data_msg->msgData.dataCnf.ackData,
		       mcxw_ctx.rx_ack_frame.length);

		k_sem_give(&mcxw_ctx.tx_wait);

		k_free(msg);
		break;

	case gPdDataInd_c:
		/* RX is done */
		struct mcxw_rx_frame rx_frame;

		/* retrieve frame information and data */
		rx_frame.lqi = data_msg->msgData.dataInd.ppduLinkQuality;
		rx_frame.rssi = data_msg->msgData.dataInd.ppduRssi;
		rx_frame.timestamp = rf_adjust_tstamp_from_phy(data_msg->msgData.dataInd.timeStamp);
		rx_frame.ack_fpb = data_msg->msgData.dataInd.rxAckFp;
		rx_frame.length = data_msg->msgData.dataInd.psduLength;
		rx_frame.psdu = data_msg->msgData.dataInd.pPsdu;
		rx_frame.ack_seb = data_msg->msgData.dataInd.ackedWithSecEnhAck;

		rx_frame.phy_buffer = (void *)msg;

		/* stop rx on idle if message queue is almost full */
		if (k_msgq_num_free_get(&mcxw_ctx.rx_msgq) == 1) {
			rf_rx_on_idle(RX_ON_IDLE_STOP);
		}

		/* add the rx message in queue */
		if (k_msgq_put(&mcxw_ctx.rx_msgq, &rx_frame, K_NO_WAIT) < 0) {
			LOG_ERR("Failed to push RX data to message queue");
		}
		break;

	default:
		/* PWR_AllowDeviceToSleep(); */
		break;
	}

	stop_csl_receiver();

	return gPhySuccess_c;
}

/* Phy Layer Management Entities Service Access Point handler
 * Called by Phy to notify PLME event
 */
phyStatus_t plme_mac_sap_handler(void *msg, instanceId_t instance)
{
	plmeToMacMessage_t *plme_msg = (plmeToMacMessage_t *)msg;

	__ASSERT_NO_MSG(msg != NULL);

	/* PWR_DisallowDeviceToSleep(); */

	switch (plme_msg->msgType) {
	case gPlmeCcaCnf_c:
		if (plme_msg->msgData.ccaCnf.status == gPhyChannelBusy_c) {
			/* Channel is busy */
			mcxw_ctx.tx_status = EBUSY;
		} else {
			mcxw_ctx.tx_status = 0;
		}
		mcxw_ctx.state = RADIO_STATE_RECEIVE;

		k_sem_give(&mcxw_ctx.cca_wait);
		break;
	case gPlmeEdCnf_c:
		/* Scan done */
		if (mcxw_ctx.energy_scan_done != NULL) {
			energy_scan_done_cb_t callback = mcxw_ctx.energy_scan_done;

			mcxw_ctx.max_ed = plme_msg->msgData.edCnf.maxEnergyLeveldB;

			mcxw_ctx.energy_scan_done = NULL;
			callback(net_if_get_device(mcxw_ctx.iface), mcxw_ctx.max_ed);
		}
		break;
	case gPlmeTimeoutInd_c:
		if (RADIO_STATE_TRANSMIT == mcxw_ctx.state) {
			/* Ack timeout */
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
			if (is_keyid_mode_1(mcxw_ctx.tx_frame.psdu, mcxw_ctx.tx_frame.length) &&
			    !mcxw_ctx.tx_frame.sec_processed && !mcxw_ctx.tx_frame.hdr_updated) {
				set_frame_counter(mcxw_ctx.tx_frame.psdu, mcxw_ctx.tx_frame.length,
						plme_msg->fc);
				mcxw_ctx.tx_frame.hdr_updated = true;
			}
#endif

			mcxw_ctx.state = RADIO_STATE_RECEIVE;
			/* No ack */
			mcxw_ctx.tx_status = ENOMSG;

			k_sem_give(&mcxw_ctx.tx_wait);
		} else if (RADIO_STATE_RECEIVE == mcxw_ctx.state) {
			/* CSL Receive AT state has ended with timeout and we are returning to SLEEP
			 * state
			 */
			mcxw_ctx.state = RADIO_STATE_SLEEP;
			/* PWR_AllowDeviceToSleep(); */
		}
		break;
	case gPlmeAbortInd_c:
		/* TX Packet was loaded into TX Packet RAM but the TX/TR seq did not ended ok */
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
		if (is_keyid_mode_1(mcxw_ctx.tx_frame.psdu, mcxw_ctx.tx_frame.length) &&
		    !mcxw_ctx.tx_frame.sec_processed && !mcxw_ctx.tx_frame.hdr_updated) {
			set_frame_counter(mcxw_ctx.tx_frame.psdu, mcxw_ctx.tx_frame.length,
					plme_msg->fc);
			mcxw_ctx.tx_frame.hdr_updated = true;
		}
#endif

		mcxw_ctx.state = RADIO_STATE_RECEIVE;
		mcxw_ctx.tx_status = EIO;

		k_sem_give(&mcxw_ctx.tx_wait);
		break;
	default:
		/* PWR_AllowDeviceToSleep(); */
		break;
	}
	/* The message has been allocated by the Phy, we have to free it */
	k_free(msg);

	stop_csl_receiver();

	return gPhySuccess_c;
}

static int mcxw_configure(const struct device *dev, enum ieee802154_config_type type,
			  const struct ieee802154_config *config)
{
	ARG_UNUSED(dev);

	switch (type) {

	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		if (config->auto_ack_fpb.mode == IEEE802154_FPB_ADDR_MATCH_THREAD) {
			mcxw_enable_src_match(config->auto_ack_fpb.enabled);
		}
		/* TODO IEEE802154_FPB_ADDR_MATCH_ZIGBEE */
		break;

	case IEEE802154_CONFIG_ACK_FPB:
		if (config->ack_fpb.enabled) {
			return mcxw_src_match_entry(config->ack_fpb.extended, config->ack_fpb.addr);
		} else {
			return mcxw_src_clear_entry(config->ack_fpb.extended, config->ack_fpb.addr);
		}

		/* TODO otPlatRadioClearSrcMatchShortEntries */
		/* TODO otPlatRadioClearSrcMatchExtEntries */
		break;

	case IEEE802154_CONFIG_PAN_COORDINATOR:
		mcxw_set_pan_coord(config->pan_coordinator);
		break;

	case IEEE802154_CONFIG_PROMISCUOUS:
		mcxw_set_promiscuous(config->promiscuous);
		break;

	case IEEE802154_CONFIG_MAC_KEYS:
		mcxw_set_mac_key(config->mac_keys);
		break;

	case IEEE802154_CONFIG_FRAME_COUNTER:
		mcxw_set_mac_frame_counter(config->frame_counter);
		break;

	case IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER:
		mcxw_set_mac_frame_counter_if_larger(config->frame_counter);
		break;

	case IEEE802154_CONFIG_ENH_ACK_HEADER_IE:
		mcxw_configure_enh_ack_probing(config);
		break;

#if defined(CONFIG_IEEE802154_CSL_ENDPOINT)
	case IEEE802154_CONFIG_EXPECTED_RX_TIME:
		mcxw_ctx.csl_sample_time = config->expected_rx_time;
		break;

	case IEEE802154_CONFIG_RX_SLOT:
		mcxw_receive_at(config->rx_slot.channel, config->rx_slot.start / NSEC_PER_USEC,
				config->rx_slot.duration / NSEC_PER_USEC);
		break;

	case IEEE802154_CONFIG_CSL_PERIOD:
		mcxw_enable_csl(config->csl_period);
		break;
#endif /* CONFIG_IEEE802154_CSL_ENDPOINT */

	case IEEE802154_CONFIG_RX_ON_WHEN_IDLE:
		if (config->rx_on_when_idle) {
			rf_rx_on_idle(RX_ON_IDLE_START);
		} else {
			rf_rx_on_idle(RX_ON_IDLE_STOP);
		}
		break;

	case IEEE802154_CONFIG_EVENT_HANDLER:
		break;

	case IEEE802154_OPENTHREAD_CONFIG_MAX_EXTRA_CCA_ATTEMPTS:
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

static int mcxw_attr_get(const struct device *dev, enum ieee802154_attr attr,
			 struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	if (ieee802154_attr_get_channel_page_and_range(
		    attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		    &drv_attr.phy_supported_channels, value) == 0) {
		return 0;
	}

	return -EIO;
}

static enum ieee802154_hw_caps mcxw_get_capabilities(const struct device *dev)
{
	enum ieee802154_hw_caps caps;

	caps = IEEE802154_HW_FCS | IEEE802154_HW_PROMISC | IEEE802154_HW_FILTER |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_RX_TX_ACK | IEEE802154_HW_ENERGY_SCAN |
	       IEEE802154_HW_TXTIME | IEEE802154_HW_RXTIME | IEEE802154_HW_SLEEP_TO_TX |
	       IEEE802154_RX_ON_WHEN_IDLE | IEEE802154_HW_TX_SEC |
	       IEEE802154_OPENTHREAD_HW_MULTIPLE_CCA | IEEE802154_HW_SELECTIVE_TXCHANNEL |
	       IEEE802154_OPENTHREAD_HW_CST;
	return caps;
}

static int mcxw_init(const struct device *dev)
{
	struct mcxw_context *mcxw_radio = dev->data;

	macToPlmeMessage_t msg;

	if (PLATFORM_InitOT() < 0) {
		return -EIO;
	}

	Phy_Init();

	ot_phy_ctx = PHY_get_ctx();

	/* Register Phy Data Service Access Point and Phy Layer Management Entities Service Access
	 * Point handlers
	 */
	Phy_RegisterSapHandlers((PD_MAC_SapHandler_t)pd_mac_sap_handler,
				(PLME_MAC_SapHandler_t)plme_mac_sap_handler, ot_phy_ctx);

	msg.msgType = gPlmeEnableEncryption_c;
	(void)MAC_PLME_SapHandler(&msg, ot_phy_ctx);

	mcxw_radio->state = RADIO_STATE_DISABLED;
	mcxw_radio->energy_scan_done = NULL;

	mcxw_radio->channel = DEFAULT_CHANNEL;
	rf_set_channel(mcxw_radio->channel);

	mcxw_radio->tx_frame.length = 0;
	/* Make the psdu point to the space after macToPdDataMessage_t in the data buffer */
	mcxw_radio->tx_frame.psdu = mcxw_radio->tx_data + sizeof(macToPdDataMessage_t);

	/* Get and start LPTRM counter */
	mcxw_radio->counter = DEVICE_DT_GET(DT_NODELABEL(lptmr0));
	if (counter_start(mcxw_radio->counter)) {
		return -EIO;
	}

	/* Init TX semaphore */
	k_sem_init(&mcxw_radio->tx_wait, 0, 1);
	/* Init CCA semaphore */
	k_sem_init(&mcxw_radio->cca_wait, 0, 1);

	/* Init RX message queue */
	k_msgq_init(&mcxw_radio->rx_msgq, mcxw_radio->rx_msgq_buffer, sizeof(mcxw_rx_frame),
		    NMAX_RXRING_BUFFERS);

	memset(&(mcxw_radio->rx_ack_frame), 0, sizeof(mcxw_radio->rx_ack_frame));
	mcxw_radio->rx_ack_frame.psdu = mcxw_radio->rx_ack_data;

	k_thread_create(&mcxw_radio->rx_thread, mcxw_radio->rx_stack,
			CONFIG_IEEE802154_MCXW_RX_STACK_SIZE, mcxw_rx_thread, mcxw_radio, NULL,
			NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_thread_name_set(&mcxw_radio->rx_thread, "mcxw_rx");

	return 0;
}

static void mcxw_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct mcxw_context *mcxw_radio = dev->data;

	mcxw_get_eui64(mcxw_radio->mac);

	net_if_set_link_addr(iface, mcxw_radio->mac, sizeof(mcxw_radio->mac), NET_LINK_IEEE802154);
	mcxw_radio->iface = iface;
	ieee802154_init(iface);
}

static void rf_rx_on_idle(uint32_t new_val)
{
	macToPlmeMessage_t msg;
	phyStatus_t phy_status;

	new_val %= 2;
	if (sun_rx_mode != new_val) {
		sun_rx_mode = new_val;
		msg.msgType = gPlmeSetReq_c;
		msg.msgData.setReq.PibAttribute = gPhyPibRxOnWhenIdle;
		msg.msgData.setReq.PibAttributeValue = (uint64_t)sun_rx_mode;

		phy_status = MAC_PLME_SapHandler(&msg, ot_phy_ctx);

		__ASSERT_NO_MSG(phy_status == gPhySuccess_c);
	}
}

static const struct ieee802154_radio_api mcxw71_radio_api = {
	.iface_api.init = mcxw_iface_init,

	.get_capabilities = mcxw_get_capabilities,
	.cca = mcxw_cca,
	.set_channel = mcxw_set_channel,
	.filter = mcxw_filter,
	.set_txpower = mcxw_set_txpower,
	.start = mcxw_start,
	.stop = mcxw_stop,
	.configure = mcxw_configure,
	.tx = mcxw_tx,
	.ed_scan = mcxw_energy_scan,
	.get_time = mcxw_get_time,
	.get_sch_acc = mcxw_get_acc,
	.attr_get = mcxw_attr_get,
};

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2          IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU         IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2          OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU         1280
#elif defined(CONFIG_NET_L2_CUSTOM_IEEE802154)
#define L2          CUSTOM_IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(CUSTOM_IEEE802154_L2)
#define MTU         CONFIG_NET_L2_CUSTOM_IEEE802154_MTU
#endif

NET_DEVICE_DT_INST_DEFINE(0, mcxw_init, NULL, &mcxw_ctx, NULL, CONFIG_IEEE802154_MCXW_INIT_PRIO,
			  &mcxw71_radio_api, L2, L2_CTX_TYPE, MTU);
