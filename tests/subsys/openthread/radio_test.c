/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/fff.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/message.h>
#include <openthread/platform/radio.h>
#include <platform-zephyr.h>

DEFINE_FFF_GLOBALS;

/**
 * @brief Tests for the radio.c - OpenThread radio api
 * @defgroup openthread_tests radio
 * @ingroup all_tests
 * @{
 */

#define ACK_PKT_LENGTH	3
#define FRAME_TYPE_MASK 0x07
#define FRAME_TYPE_ACK	0x02

K_SEM_DEFINE(ot_sem, 0, 1);

/**
 * Fake pointer as it should not be accessed by the code.
 * Should not be null to be sure it was properly passed.
 */
otInstance *ot = (otInstance *)0xAAAA;
otMessage *ip_msg = (otMessage *)0xBBBB;

/* forward declarations */
FAKE_VALUE_FUNC(int, scan_mock, const struct device *, uint16_t, energy_scan_done_cb_t);
FAKE_VALUE_FUNC(int, cca_mock, const struct device *);
FAKE_VALUE_FUNC(int, set_channel_mock, const struct device *, uint16_t);
FAKE_VALUE_FUNC(int, filter_mock, const struct device *, bool, enum ieee802154_filter_type,
		const struct ieee802154_filter *);
FAKE_VALUE_FUNC(int, set_txpower_mock, const struct device *, int16_t);
FAKE_VALUE_FUNC(int, tx_mock, const struct device *, enum ieee802154_tx_mode, struct net_pkt *,
		struct net_buf *);
FAKE_VALUE_FUNC(int, start_mock, const struct device *);
FAKE_VALUE_FUNC(int, stop_mock, const struct device *);
FAKE_VALUE_FUNC(int, configure_mock, const struct device *, enum ieee802154_config_type,
		const struct ieee802154_config *);
FAKE_VALUE_FUNC(int, configure_promiscuous_mock, const struct device *, enum ieee802154_config_type,
		const struct ieee802154_config *);
FAKE_VALUE_FUNC(enum ieee802154_hw_caps, get_capabilities_caps_mock, const struct device *);

static enum ieee802154_hw_caps get_capabilities(const struct device *dev);

/* mocks */
static struct ieee802154_radio_api rapi = {.get_capabilities = get_capabilities,
					   .cca = cca_mock,
					   .set_channel = set_channel_mock,
					   .filter = filter_mock,
					   .set_txpower = set_txpower_mock,
					   .tx = tx_mock,
					   .start = start_mock,
					   .stop = stop_mock,
					   .configure = configure_mock,
#ifdef CONFIG_NET_L2_IEEE802154_SUB_GHZ
					   .get_subg_channel_count = NULL,
#endif /* CONFIG_NET_L2_IEEE802154_SUB_GHZ */
					   .ed_scan = scan_mock};

#define DT_DRV_COMPAT vnd_ieee802154
DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, 0, &rapi);

static const struct device *const radio = DEVICE_DT_INST_GET(0);

static int16_t rssi_scan_mock_max_ed;
static int rssi_scan_mock(const struct device *dev, uint16_t duration,
			  energy_scan_done_cb_t done_cb)
{
	zassert_equal(dev, radio, "Device handle incorrect.");
	zassert_equal(duration, 1, "otPlatRadioGetRssi shall pass minimal allowed value.");

	/* use return value as callback param */
	done_cb(radio, rssi_scan_mock_max_ed);

	return 0;
}

FAKE_VOID_FUNC(otPlatRadioEnergyScanDone, otInstance *, int8_t);

void otSysEventSignalPending(void) { k_sem_give(&ot_sem); }

void otTaskletsSignalPending(otInstance *aInstance)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	k_sem_give(&ot_sem);
}

static void make_sure_sem_set(k_timeout_t timeout)
{
	zassert_equal(k_sem_take(&ot_sem, timeout), 0, "Sem not released.");
}

static otRadioFrame otPlatRadioReceiveDone_expected_aframe;
static otError otPlatRadioReceiveDone_expected_error;
void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	zassert_equal(otPlatRadioReceiveDone_expected_aframe.mChannel, aFrame->mChannel);
	zassert_equal(otPlatRadioReceiveDone_expected_aframe.mLength, aFrame->mLength);
	zassert_mem_equal(otPlatRadioReceiveDone_expected_aframe.mPsdu, aFrame->mPsdu,
			  aFrame->mLength, NULL);
	zassert_equal(otPlatRadioReceiveDone_expected_error, aError);
}

FAKE_VOID_FUNC(otPlatRadioTxDone, otInstance *, otRadioFrame *, otRadioFrame *, otError);

static enum ieee802154_hw_caps get_capabilities(const struct device *dev)
{
	zassert_equal(dev, radio, "Device handle incorrect.");

	return IEEE802154_HW_FCS | IEEE802154_HW_2_4_GHZ | IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_FILTER | IEEE802154_HW_ENERGY_SCAN | IEEE802154_HW_SLEEP_TO_TX;
}

FAKE_VALUE_FUNC(int, configure_match_mock, const struct device *, enum ieee802154_config_type,
		const struct ieee802154_config *);

FAKE_VALUE_FUNC(otError, otIp6Send, otInstance *, otMessage *);

otMessage *otIp6NewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	return ip_msg;
}

FAKE_VALUE_FUNC(otError, otMessageAppend, otMessage *, const void *, uint16_t);

FAKE_VOID_FUNC(otMessageFree, otMessage *);

void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
}

/**
 * @brief Test for immediate energy scan
 * Tests for case when radio energy scan returns success at the first call.
 *
 */
ZTEST(openthread_radio, test_energy_scan_immediate_test)
{
	const uint8_t chan = 10;
	const uint8_t dur = 100;
	const int16_t energy = -94;

	set_channel_mock_fake.return_val = 0;

	scan_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioEnergyScan(ot, chan, dur), OT_ERROR_NONE,
		      "Energy scan returned error.");
	zassert_equal(1, scan_mock_fake.call_count);
	zassert_equal(dur, scan_mock_fake.arg1_val);
	zassert_not_null(scan_mock_fake.arg2_val, "Scan callback not specified.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(chan, set_channel_mock_fake.arg1_val);

	scan_mock_fake.arg2_val(radio, energy);
	make_sure_sem_set(K_NO_WAIT);

	platformRadioProcess(ot);
	zassert_equal(1, otPlatRadioEnergyScanDone_fake.call_count);
	zassert_equal_ptr(ot, otPlatRadioEnergyScanDone_fake.arg0_val, NULL);
	zassert_equal(energy, otPlatRadioEnergyScanDone_fake.arg1_val);
}

/**
 * @brief Test for delayed energy scan
 * Tests for case when radio returns not being able to start energy scan and
 * the scan should be scheduled for later.
 *
 */
ZTEST(openthread_radio, test_energy_scan_delayed_test)
{
	const uint8_t chan = 10;
	const uint8_t dur = 100;
	const int16_t energy = -94;

	/* request scan */
	set_channel_mock_fake.return_val = 0;
	scan_mock_fake.return_val = -EBUSY;

	zassert_equal(otPlatRadioEnergyScan(ot, chan, dur), OT_ERROR_NONE,
		      "Energy scan returned error.");
	zassert_equal(1, scan_mock_fake.call_count);
	zassert_equal(dur, scan_mock_fake.arg1_val);
	zassert_not_null(scan_mock_fake.arg2_val, "Scan callback not specified.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(chan, set_channel_mock_fake.arg1_val);
	make_sure_sem_set(K_NO_WAIT);

	/* process reported event */
	RESET_FAKE(scan_mock);
	RESET_FAKE(set_channel_mock);
	FFF_RESET_HISTORY();

	scan_mock_fake.return_val = 0;
	set_channel_mock_fake.return_val = 0;

	platformRadioProcess(ot);
	zassert_equal(1, scan_mock_fake.call_count);
	zassert_equal(dur, scan_mock_fake.arg1_val);
	zassert_not_null(scan_mock_fake.arg2_val, "Scan callback not specified.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(chan, set_channel_mock_fake.arg1_val);

	/* invoke scan done */
	scan_mock_fake.arg2_val(radio, energy);
	make_sure_sem_set(K_NO_WAIT);

	platformRadioProcess(ot);
	zassert_equal(1, otPlatRadioEnergyScanDone_fake.call_count);
	zassert_equal_ptr(ot, otPlatRadioEnergyScanDone_fake.arg0_val, NULL);
	zassert_equal(energy, otPlatRadioEnergyScanDone_fake.arg1_val);
}

static void create_ack_frame(void)
{
	struct net_pkt *packet;
	struct net_buf *buf;
	const uint8_t lqi = 230;
	const int8_t rssi = -80;

	packet = net_pkt_alloc(K_NO_WAIT);
	buf = net_pkt_get_reserve_tx_data(ACK_PKT_LENGTH, K_NO_WAIT);
	net_pkt_append_buffer(packet, buf);

	buf->len = ACK_PKT_LENGTH;
	buf->data[0] = FRAME_TYPE_ACK;

	net_pkt_set_ieee802154_rssi(packet, rssi);
	net_pkt_set_ieee802154_lqi(packet, lqi);
	zassert_equal(ieee802154_radio_handle_ack(NULL, packet), NET_OK, "Handling ack failed.");
	net_pkt_unref(packet);
}

/**
 * @brief Test for tx data handling
 * Tests if OT frame is correctly passed to the radio driver.
 * Additionally verifies ACK frame passing back to the OT.
 *
 */
ZTEST(openthread_radio, test_tx_test)
{
	const uint8_t chan = 20;
	uint8_t chan2 = chan - 1;
	const int8_t power = -3;

	otRadioFrame *frm = otPlatRadioGetTransmitBuffer(ot);

	zassert_not_null(frm, "Transmit buffer is null.");

	zassert_equal(otPlatRadioSetTransmitPower(ot, power), OT_ERROR_NONE,
		      "Failed to set TX power.");

	set_channel_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioReceive(ot, chan), OT_ERROR_NONE, "Failed to receive.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(chan, set_channel_mock_fake.arg1_val);
	zassert_equal(1, set_txpower_mock_fake.call_count);
	zassert_equal(power, set_txpower_mock_fake.arg1_val);
	zassert_equal(1, start_mock_fake.call_count);
	zassert_equal_ptr(radio, start_mock_fake.arg0_val, NULL);
	RESET_FAKE(set_channel_mock);
	RESET_FAKE(set_txpower_mock);
	RESET_FAKE(start_mock);
	FFF_RESET_HISTORY();

	/* ACKed frame */
	frm->mChannel = chan2;
	frm->mInfo.mTxInfo.mCsmaCaEnabled = true;
	frm->mPsdu[0] = IEEE802154_AR_FLAG_SET;
	set_channel_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioTransmit(ot, frm), OT_ERROR_NONE, "Transmit failed.");

	create_ack_frame();
	make_sure_sem_set(Z_TIMEOUT_MS(100));

	platformRadioProcess(ot);
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(chan2, set_channel_mock_fake.arg1_val);
	zassert_equal(1, cca_mock_fake.call_count);
	zassert_equal_ptr(radio, cca_mock_fake.arg0_val, NULL);
	zassert_equal(1, set_txpower_mock_fake.call_count);
	zassert_equal(power, set_txpower_mock_fake.arg1_val);
	zassert_equal(1, tx_mock_fake.call_count);
	zassert_equal_ptr(frm->mPsdu, tx_mock_fake.arg3_val->data, NULL);
	zassert_equal(1, otPlatRadioTxDone_fake.call_count);
	zassert_equal_ptr(ot, otPlatRadioTxDone_fake.arg0_val, NULL);
	zassert_equal(OT_ERROR_NONE, otPlatRadioTxDone_fake.arg3_val);
	RESET_FAKE(set_channel_mock);
	RESET_FAKE(set_txpower_mock);
	RESET_FAKE(tx_mock);
	RESET_FAKE(otPlatRadioTxDone);
	FFF_RESET_HISTORY();

	/* Non-ACKed frame */
	frm->mChannel = --chan2;
	frm->mInfo.mTxInfo.mCsmaCaEnabled = false;
	frm->mPsdu[0] = 0;

	set_channel_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioTransmit(ot, frm), OT_ERROR_NONE, "Transmit failed.");
	make_sure_sem_set(Z_TIMEOUT_MS(100));
	platformRadioProcess(ot);
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(chan2, set_channel_mock_fake.arg1_val);
	zassert_equal(1, set_txpower_mock_fake.call_count);
	zassert_equal(power, set_txpower_mock_fake.arg1_val);
	zassert_equal(1, tx_mock_fake.call_count);
	zassert_equal_ptr(frm->mPsdu, tx_mock_fake.arg3_val->data, NULL);
	zassert_equal(1, otPlatRadioTxDone_fake.call_count);
	zassert_equal_ptr(ot, otPlatRadioTxDone_fake.arg0_val, NULL);
	zassert_equal(OT_ERROR_NONE, otPlatRadioTxDone_fake.arg3_val);
}

/**
 * @brief Test for tx power setting
 * Tests if tx power requested by the OT is correctly passed to the radio.
 *
 */
ZTEST(openthread_radio, test_tx_power_test)
{
	int8_t out_power = 0;

	zassert_equal(otPlatRadioSetTransmitPower(ot, -3), OT_ERROR_NONE,
		      "Failed to set TX power.");
	zassert_equal(otPlatRadioGetTransmitPower(ot, &out_power), OT_ERROR_NONE,
		      "Failed to obtain TX power.");
	zassert_equal(out_power, -3, "Got different power than set.");
	zassert_equal(otPlatRadioSetTransmitPower(ot, -6), OT_ERROR_NONE,
		      "Failed to set TX power.");
	zassert_equal(otPlatRadioGetTransmitPower(ot, &out_power), OT_ERROR_NONE,
		      "Failed to obtain TX power.");
	zassert_equal(out_power, -6, "Second call to otPlatRadioSetTransmitPower failed.");
}

/**
 * @brief Test for getting radio sensitivity
 * There is no api to get radio sensitivity from the radio so the value is
 * hardcoded in radio.c. Test only verifies if the value returned makes any
 * sense.
 *
 */
ZTEST(openthread_radio, test_sensitivity_test)
{
	/*
	 * Nothing to test actually as this is constant 100.
	 * When radio interface will be extended to get sensitivity this test
	 * can be extended with the radio api call. For now just verify if the
	 * value is reasonable.
	 */
	zassert_true(-80 > otPlatRadioGetReceiveSensitivity(ot), "Radio sensitivity not in range.");
}

static enum ieee802154_config_type custom_configure_match_mock_expected_type;
static struct ieee802154_config custom_configure_match_mock_expected_config;
static int custom_configure_match_mock(const struct device *dev, enum ieee802154_config_type type,
				       const struct ieee802154_config *config)
{
	zassert_equal_ptr(dev, radio, "Device handle incorrect.");
	zassert_equal(custom_configure_match_mock_expected_type, type);
	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		zassert_equal(custom_configure_match_mock_expected_config.auto_ack_fpb.mode,
			      config->auto_ack_fpb.mode, NULL);
		zassert_equal(custom_configure_match_mock_expected_config.auto_ack_fpb.enabled,
			      config->auto_ack_fpb.enabled, NULL);
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		zassert_equal(custom_configure_match_mock_expected_config.ack_fpb.extended,
			      config->ack_fpb.extended, NULL);
		zassert_equal(custom_configure_match_mock_expected_config.ack_fpb.enabled,
			      config->ack_fpb.enabled, NULL);
		if (custom_configure_match_mock_expected_config.ack_fpb.addr == NULL) {
			zassert_is_null(config->ack_fpb.addr, NULL);
		} else {
			zassert_mem_equal(custom_configure_match_mock_expected_config.ack_fpb.addr,
					  config->ack_fpb.addr,
					  (config->ack_fpb.extended) ? sizeof(otExtAddress) : 2,
					  NULL);
		}
		break;
	default:
		zassert_unreachable("Unexpected config type %d.", type);
		break;
	}

	return 0;
}
static void set_expected_match_values(enum ieee802154_config_type type, uint8_t *addr,
				      bool extended, bool enabled)
{
	custom_configure_match_mock_expected_type = type;
	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		custom_configure_match_mock_expected_config.auto_ack_fpb.enabled = enabled;
		custom_configure_match_mock_expected_config.auto_ack_fpb.mode =
			IEEE802154_FPB_ADDR_MATCH_THREAD;
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		custom_configure_match_mock_expected_config.ack_fpb.extended = extended;
		custom_configure_match_mock_expected_config.ack_fpb.enabled = enabled;
		custom_configure_match_mock_expected_config.ack_fpb.addr = addr;
		break;
	default:
		break;
	}
}

/**
 * @brief Test different types of OT source match.
 * Tests if Enable, Disable, Add and Clear Source Match calls are passed to the
 * radio driver correctly.
 *
 */
ZTEST(openthread_radio, test_source_match_test)
{
	otExtAddress ext_addr;
	configure_match_mock_fake.custom_fake = custom_configure_match_mock;

	rapi.configure = configure_match_mock;
	/* Enable/Disable */
	set_expected_match_values(IEEE802154_CONFIG_AUTO_ACK_FPB, NULL, false, true);
	otPlatRadioEnableSrcMatch(ot, true);

	set_expected_match_values(IEEE802154_CONFIG_AUTO_ACK_FPB, NULL, false, false);
	otPlatRadioEnableSrcMatch(ot, false);

	set_expected_match_values(IEEE802154_CONFIG_AUTO_ACK_FPB, NULL, false, true);
	otPlatRadioEnableSrcMatch(ot, true);

	/* Add */
	sys_put_le16(12345, ext_addr.m8);
	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, false, true);
	zassert_equal(otPlatRadioAddSrcMatchShortEntry(ot, 12345), OT_ERROR_NONE,
		      "Failed to add short src entry.");

	for (int i = 0; i < sizeof(ext_addr.m8); i++) {
		ext_addr.m8[i] = i;
	}
	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, true, true);
	zassert_equal(otPlatRadioAddSrcMatchExtEntry(ot, &ext_addr), OT_ERROR_NONE,
		      "Failed to add ext src entry.");

	/* Clear */
	sys_put_le16(12345, ext_addr.m8);
	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, false, false);
	zassert_equal(otPlatRadioClearSrcMatchShortEntry(ot, 12345), OT_ERROR_NONE,
		      "Failed to clear short src entry.");

	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, true, false);
	zassert_equal(otPlatRadioClearSrcMatchExtEntry(ot, &ext_addr), OT_ERROR_NONE,
		      "Failed to clear ext src entry.");

	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, NULL, false, false);
	otPlatRadioClearSrcMatchShortEntries(ot);

	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, NULL, true, false);
	otPlatRadioClearSrcMatchExtEntries(ot);

	rapi.configure = configure_mock;
}

static bool custom_configure_promiscuous_mock_promiscuous;
static int custom_configure_promiscuous_mock(const struct device *dev,
					     enum ieee802154_config_type type,
					     const struct ieee802154_config *config)
{
	zassert_equal(dev, radio, "Device handle incorrect.");
	zassert_equal(type, IEEE802154_CONFIG_PROMISCUOUS, "Config type incorrect.");
	custom_configure_promiscuous_mock_promiscuous = config->promiscuous;

	return 0;
}
/**
 * @brief Test for enabling or disabling promiscuous mode
 * Tests if OT can successfully enable or disable promiscuous mode.
 *
 */
ZTEST(openthread_radio, test_promiscuous_mode_set_test)
{
	rapi.configure = configure_promiscuous_mock;

	zassert_false(otPlatRadioGetPromiscuous(ot),
		      "By default promiscuous mode shall be disabled.");

	configure_promiscuous_mock_fake.custom_fake = custom_configure_promiscuous_mock;
	otPlatRadioSetPromiscuous(ot, true);
	zassert_true(otPlatRadioGetPromiscuous(ot), "Mode not enabled.");
	zassert_equal(1, configure_promiscuous_mock_fake.call_count);
	zassert_true(custom_configure_promiscuous_mock_promiscuous);

	RESET_FAKE(configure_promiscuous_mock);
	FFF_RESET_HISTORY();

	configure_promiscuous_mock_fake.custom_fake = custom_configure_promiscuous_mock;
	otPlatRadioSetPromiscuous(ot, false);
	zassert_false(otPlatRadioGetPromiscuous(ot), "Mode still enabled.");
	zassert_equal(1, configure_promiscuous_mock_fake.call_count);
	zassert_false(custom_configure_promiscuous_mock_promiscuous);

	rapi.configure = configure_mock;
}

/**
 * @brief Test of proper radio to OT capabilities mapping
 * Tests if different radio capabilities map for their corresponding OpenThread
 * capability
 *
 */
ZTEST(openthread_radio, test_get_caps_test)
{
	rapi.get_capabilities = get_capabilities_caps_mock;

	/* no caps */
	get_capabilities_caps_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	/* not used by OT */
	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_FCS;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");
	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_2_4_GHZ;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");
	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_SUB_GHZ;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	/* not implemented or not fully supported */
	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_TXTIME;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_PROMISC;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	/* proper mapping */
	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_CSMA;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_CSMA_BACKOFF,
		      "Incorrect capabilities returned.");

	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_ENERGY_SCAN;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_ENERGY_SCAN,
		      "Incorrect capabilities returned.");

	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_TX_RX_ACK;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_ACK_TIMEOUT,
		      "Incorrect capabilities returned.");

	get_capabilities_caps_mock_fake.return_val = IEEE802154_HW_SLEEP_TO_TX;
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_SLEEP_TO_TX,
		      "Incorrect capabilities returned.");

	/* all at once */
	get_capabilities_caps_mock_fake.return_val =
		IEEE802154_HW_FCS | IEEE802154_HW_PROMISC | IEEE802154_HW_FILTER |
		IEEE802154_HW_CSMA | IEEE802154_HW_2_4_GHZ | IEEE802154_HW_TX_RX_ACK |
		IEEE802154_HW_SUB_GHZ | IEEE802154_HW_ENERGY_SCAN | IEEE802154_HW_TXTIME |
		IEEE802154_HW_SLEEP_TO_TX;
	zassert_equal(otPlatRadioGetCaps(ot),
		      OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_ENERGY_SCAN |
			      OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_SLEEP_TO_TX,
		      "Incorrect capabilities returned.");

	rapi.get_capabilities = get_capabilities;
}

/**
 * @brief Test for getting the rssi value from the radio
 * Tests if correct value is returned from the otPlatRadioGetRssi function.
 *
 */
ZTEST(openthread_radio, test_get_rssi_test)
{
	const int8_t rssi = -103;

	rapi.ed_scan = rssi_scan_mock;

	rssi_scan_mock_max_ed = rssi;
	zassert_equal(otPlatRadioGetRssi(ot), rssi, "Invalid RSSI value received.");

	rapi.ed_scan = scan_mock;
}

/**
 * @brief Test switching between radio states
 * Tests if radio is correctly switched between states.
 *
 */
ZTEST(openthread_radio, test_radio_state_test)
{
	const uint8_t channel = 12;
	const uint8_t power = 10;

	zassert_equal(otPlatRadioSetTransmitPower(ot, power), OT_ERROR_NONE,
		      "Failed to set TX power.");
	zassert_equal(otPlatRadioDisable(ot), OT_ERROR_NONE, "Failed to disable radio.");

	zassert_false(otPlatRadioIsEnabled(ot), "Radio reports as enabled.");

	zassert_equal(otPlatRadioSleep(ot), OT_ERROR_INVALID_STATE,
		      "Changed to sleep regardless being disabled.");

	zassert_equal(otPlatRadioEnable(ot), OT_ERROR_NONE, "Enabling radio failed.");

	zassert_true(otPlatRadioIsEnabled(ot), "Radio reports disabled.");

	zassert_equal(otPlatRadioSleep(ot), OT_ERROR_NONE, "Failed to switch to sleep mode.");

	zassert_true(otPlatRadioIsEnabled(ot), "Radio reports as disabled.");

	set_channel_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioReceive(ot, channel), OT_ERROR_NONE, "Failed to receive.");
	zassert_equal(platformRadioChannelGet(ot), channel, "Channel number not remembered.");

	zassert_true(otPlatRadioIsEnabled(ot), "Radio reports as disabled.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(channel, set_channel_mock_fake.arg1_val);
	zassert_equal(1, set_txpower_mock_fake.call_count);
	zassert_equal(power, set_txpower_mock_fake.arg1_val);
	zassert_equal(1, start_mock_fake.call_count);
	zassert_equal_ptr(radio, start_mock_fake.arg0_val, NULL);
	zassert_equal(1, stop_mock_fake.call_count);
	zassert_equal_ptr(radio, stop_mock_fake.arg0_val, NULL);
}

static uint16_t custom_filter_mock_pan_id;
static uint16_t custom_filter_mock_short_addr;
static uint8_t *custom_filter_mock_ieee_addr;
static int custom_filter_mock(const struct device *dev, bool set, enum ieee802154_filter_type type,
			      const struct ieee802154_filter *filter)
{
	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		custom_filter_mock_ieee_addr = filter->ieee_addr;
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		custom_filter_mock_short_addr = filter->short_addr;
		break;
	case IEEE802154_FILTER_TYPE_PAN_ID:
		custom_filter_mock_pan_id = filter->pan_id;
		break;
	default:
		zassert_false(true, "Type not supported in mock: %d.", type);
		break;
	}
	return 0;
}

/**
 * @brief Test address filtering
 * Tests if short, extended address and PanID are correctly passed to the radio
 * driver.
 *
 */
ZTEST(openthread_radio, test_address_test)
{
	const uint16_t pan_id = 0xDEAD;
	const uint16_t short_add = 0xCAFE;
	otExtAddress ieee_addr;

	for (int i = 0; i < sizeof(ieee_addr.m8); i++) {
		ieee_addr.m8[i] = 'a' + i;
	}

	filter_mock_fake.custom_fake = custom_filter_mock;
	otPlatRadioSetPanId(ot, pan_id);
	zassert_equal(1, filter_mock_fake.call_count);
	zassert_true(filter_mock_fake.arg1_val);
	zassert_equal(IEEE802154_FILTER_TYPE_PAN_ID, filter_mock_fake.arg2_val);
	zassert_equal(pan_id, custom_filter_mock_pan_id);
	RESET_FAKE(filter_mock);
	FFF_RESET_HISTORY();

	filter_mock_fake.custom_fake = custom_filter_mock;
	otPlatRadioSetShortAddress(ot, short_add);
	zassert_equal(1, filter_mock_fake.call_count);
	zassert_true(filter_mock_fake.arg1_val);
	zassert_equal(IEEE802154_FILTER_TYPE_SHORT_ADDR, filter_mock_fake.arg2_val);
	zassert_equal(short_add, custom_filter_mock_short_addr);
	RESET_FAKE(filter_mock);
	FFF_RESET_HISTORY();

	filter_mock_fake.custom_fake = custom_filter_mock;
	otPlatRadioSetExtendedAddress(ot, &ieee_addr);
	zassert_equal(1, filter_mock_fake.call_count);
	zassert_true(filter_mock_fake.arg1_val);
	zassert_equal(IEEE802154_FILTER_TYPE_IEEE_ADDR, filter_mock_fake.arg2_val);
	zassert_mem_equal(ieee_addr.m8, custom_filter_mock_ieee_addr, OT_EXT_ADDRESS_SIZE, NULL);
}

uint8_t alloc_pkt(struct net_pkt **out_packet, uint8_t buf_ct, uint8_t offset)
{
	struct net_pkt *packet;
	struct net_buf *buf;
	uint8_t len = 0;
	uint8_t buf_num;

	packet = net_pkt_alloc(K_NO_WAIT);
	for (buf_num = 0; buf_num < buf_ct; buf_num++) {
		buf = net_pkt_get_reserve_tx_data(IEEE802154_MAX_PHY_PACKET_SIZE,
						  K_NO_WAIT);
		net_pkt_append_buffer(packet, buf);

		for (int i = 0; i < buf->size; i++) {
			buf->data[i] = (offset + i + buf_num) & 0xFF;
		}

		len = buf->size - 3;
		buf->len = len;
	}

	*out_packet = packet;
	return len;
}

/**
 * @brief Test received messages handling.
 * Tests if received frames are properly passed to the OpenThread
 *
 */
ZTEST(openthread_radio, test_receive_test)
{
	struct net_pkt *packet;
	struct net_buf *buf;
	const uint8_t channel = 21;
	const int8_t power = -5;
	const uint8_t lqi = 240;
	const int8_t rssi = -90;
	uint8_t len;

	len = alloc_pkt(&packet, 1, 'a');
	buf = packet->buffer;

	net_pkt_set_ieee802154_lqi(packet, lqi);
	net_pkt_set_ieee802154_rssi(packet, rssi);

	zassert_equal(otPlatRadioSetTransmitPower(ot, power), OT_ERROR_NONE,
		      "Failed to set TX power.");

	set_channel_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioReceive(ot, channel), OT_ERROR_NONE, "Failed to receive.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(channel, set_channel_mock_fake.arg1_val);
	zassert_equal(1, set_txpower_mock_fake.call_count);
	zassert_equal(power, set_txpower_mock_fake.arg1_val);
	zassert_equal(1, start_mock_fake.call_count);
	zassert_equal_ptr(radio, start_mock_fake.arg0_val, NULL);

	/*
	 * Not setting any expect values as nothing shall be called from
	 * notify_new_rx_frame calling thread. OT functions can be called only
	 * after semaphore for main thread is released.
	 */
	notify_new_rx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));
	otPlatRadioReceiveDone_expected_error = OT_ERROR_NONE;
	otPlatRadioReceiveDone_expected_aframe.mChannel = channel;
	otPlatRadioReceiveDone_expected_aframe.mLength = len;
	otPlatRadioReceiveDone_expected_aframe.mPsdu = buf->data;
	platformRadioProcess(ot);
}

/**
 * @brief Test received messages handling.
 * Tests if received frames are properly passed to the OpenThread
 *
 */
ZTEST(openthread_radio, test_net_pkt_transmit)
{
	void *expected_data_ptrs[2];
	struct net_pkt *packet;
	struct net_buf *buf;
	const uint8_t channel = 21;
	const int8_t power = -5;
	uint8_t len;

	/* success */
	len = alloc_pkt(&packet, 2, 'a');
	buf = packet->buffer;
	zassert_equal(otPlatRadioSetTransmitPower(ot, power), OT_ERROR_NONE,
		      "Failed to set TX power.");

	set_channel_mock_fake.return_val = 0;
	zassert_equal(otPlatRadioReceive(ot, channel), OT_ERROR_NONE, "Failed to receive.");
	zassert_equal(1, set_channel_mock_fake.call_count);
	zassert_equal(channel, set_channel_mock_fake.arg1_val);
	zassert_equal(1, set_txpower_mock_fake.call_count);
	zassert_equal(power, set_txpower_mock_fake.arg1_val);
	zassert_equal(1, start_mock_fake.call_count);
	zassert_equal_ptr(radio, start_mock_fake.arg0_val, NULL);

	notify_new_tx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));

	otMessageAppend_fake.return_val = OT_ERROR_NONE;
	otIp6Send_fake.return_val = OT_ERROR_NONE;

	/* Do not expect free in case of success */

	expected_data_ptrs[0] = buf->data;
	expected_data_ptrs[1] = buf->frags->data;
	platformRadioProcess(ot);
	zassert_equal(2, otMessageAppend_fake.call_count);
	zassert_equal_ptr(ip_msg, otMessageAppend_fake.arg0_history[0], NULL);
	zassert_equal_ptr(ip_msg, otMessageAppend_fake.arg0_history[1], NULL);
	zassert_equal_ptr(expected_data_ptrs[0], otMessageAppend_fake.arg1_history[0], NULL);
	zassert_equal_ptr(expected_data_ptrs[1], otMessageAppend_fake.arg1_history[1], NULL);
	zassert_equal(len, otMessageAppend_fake.arg2_history[0]);
	zassert_equal(len, otMessageAppend_fake.arg2_history[1]);
	zassert_equal(1, otIp6Send_fake.call_count);
	zassert_equal_ptr(ot, otIp6Send_fake.arg0_val, NULL);
	zassert_equal_ptr(ip_msg, otIp6Send_fake.arg1_val, NULL);

	RESET_FAKE(otMessageAppend);
	RESET_FAKE(otIp6Send);
	FFF_RESET_HISTORY();

	/* fail on append */
	len = alloc_pkt(&packet, 2, 'b');
	buf = packet->buffer;

	notify_new_tx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));

	otMessageAppend_fake.return_val = OT_ERROR_NO_BUFS;
	expected_data_ptrs[0] = buf->data;

	platformRadioProcess(ot);
	zassert_equal(1, otMessageAppend_fake.call_count);
	zassert_equal_ptr(ip_msg, otMessageAppend_fake.arg0_val, NULL);
	zassert_equal_ptr(expected_data_ptrs[0], otMessageAppend_fake.arg1_val, NULL);
	zassert_equal(len, otMessageAppend_fake.arg2_val);
	zassert_equal_ptr(ip_msg, otMessageFree_fake.arg0_val, NULL);

	RESET_FAKE(otMessageAppend);
	FFF_RESET_HISTORY();

	/* fail on send */
	len = alloc_pkt(&packet, 1, 'c');
	buf = packet->buffer;

	notify_new_tx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));

	otMessageAppend_fake.return_val = OT_ERROR_NONE;
	otIp6Send_fake.return_val = OT_ERROR_BUSY;
	expected_data_ptrs[0] = buf->data;

	/* Do not expect free in case of failure in send */

	platformRadioProcess(ot);
	zassert_equal(1, otMessageAppend_fake.call_count);
	zassert_equal_ptr(ip_msg, otMessageAppend_fake.arg0_val, NULL);
	zassert_equal_ptr(expected_data_ptrs[0], otMessageAppend_fake.arg1_val, NULL);
	zassert_equal(len, otMessageAppend_fake.arg2_val);
	zassert_equal(1, otIp6Send_fake.call_count);
	zassert_equal_ptr(ot, otIp6Send_fake.arg0_val, NULL);
	zassert_equal_ptr(ip_msg, otIp6Send_fake.arg1_val, NULL);
}

static void *openthread_radio_setup(void)
{
	platformRadioInit();
	return NULL;
}

static void openthread_radio_before(void *f)
{
	ARG_UNUSED(f);
	RESET_FAKE(scan_mock);
	RESET_FAKE(cca_mock);
	RESET_FAKE(set_channel_mock);
	RESET_FAKE(filter_mock);
	RESET_FAKE(set_txpower_mock);
	RESET_FAKE(tx_mock);
	RESET_FAKE(start_mock);
	RESET_FAKE(stop_mock);
	RESET_FAKE(configure_mock);
	RESET_FAKE(configure_promiscuous_mock);
	RESET_FAKE(get_capabilities_caps_mock);
	RESET_FAKE(otPlatRadioEnergyScanDone);
	RESET_FAKE(otPlatRadioTxDone);
	RESET_FAKE(otMessageFree);
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(openthread_radio, NULL, openthread_radio_setup, openthread_radio_before, NULL, NULL);
