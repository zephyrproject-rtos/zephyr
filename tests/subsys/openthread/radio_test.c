/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include <zephyr/zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>

#include <openthread/platform/radio.h>
#include <openthread/message.h>
#include <platform-zephyr.h>

/**
 * @brief Tests for the radio.c - OpenThread radio api
 * @defgroup openthread_tests radio
 * @ingroup all_tests
 * @{
 */

#define ACK_PKT_LENGTH 3
#define FRAME_TYPE_MASK 0x07
#define FRAME_TYPE_ACK 0x02

K_SEM_DEFINE(ot_sem, 0, 1);

energy_scan_done_cb_t scan_done_cb;

/**
 * Fake pointer as it should not be accessed by the code.
 * Should not be null to be sure it was properly passed.
 */
otInstance *ot = (otInstance *)0xAAAA;
otMessage *ip_msg = (otMessage *)0xBBBB;

/* forward declarations */
static int scan_mock(const struct device *dev, uint16_t duration,
		     energy_scan_done_cb_t done_cb);
static enum ieee802154_hw_caps get_capabilities(const struct device *dev);
static int cca_mock(const struct device *dev);
static int set_channel_mock(const struct device *dev, uint16_t channel);
static int filter_mock(const struct device *dev, bool set,
		       enum ieee802154_filter_type type,
		       const struct ieee802154_filter *filter);
static int set_txpower_mock(const struct device *dev, int16_t dbm);
static int tx_mock(const struct device *dev, enum ieee802154_tx_mode mode,
		   struct net_pkt *pkt, struct net_buf *frag);
static int start_mock(const struct device *dev);
static int stop_mock(const struct device *dev);
static int configure_mock(const struct device *dev,
			  enum ieee802154_config_type type,
			  const struct ieee802154_config *config);

/* mocks */
static struct ieee802154_radio_api rapi = {
	.get_capabilities = get_capabilities,
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
	.ed_scan = scan_mock
};

static struct device radio = { .api = &rapi };

static int scan_mock(const struct device *dev, uint16_t duration,
		     energy_scan_done_cb_t done_cb)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	ztest_check_expected_value(duration);
	scan_done_cb = done_cb;
	return ztest_get_return_value();
}

static int rssi_scan_mock(const struct device *dev, uint16_t duration,
			  energy_scan_done_cb_t done_cb)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	zassert_equal(duration, 1,
		      "otPlatRadioGetRssi shall pass minimal allowed value.");

	/* use return value as callback param */
	done_cb(&radio, ztest_get_return_value());

	return 0;
}

static int set_channel_mock(const struct device *dev, uint16_t channel)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	ztest_check_expected_value(channel);
	return ztest_get_return_value();
}

void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	ztest_check_expected_value(aEnergyScanMaxRssi);
}

void otSysEventSignalPending(void)
{
	k_sem_give(&ot_sem);
}

void otTaskletsSignalPending(otInstance *aInstance)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	k_sem_give(&ot_sem);
}

static void make_sure_sem_set(k_timeout_t timeout)
{
	zassert_equal(k_sem_take(&ot_sem, timeout), 0, "Sem not released.");
}

void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame,
			    otError aError)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	ztest_check_expected_value(aFrame->mChannel);
	ztest_check_expected_value(aFrame->mLength);
	ztest_check_expected_data(aFrame->mPsdu, aFrame->mLength);
	ztest_check_expected_value(aError);
}

void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame,
		       otRadioFrame *aAckFrame, otError aError)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	ztest_check_expected_value(aError);
}

static enum ieee802154_hw_caps get_capabilities(const struct device *dev)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");

	return IEEE802154_HW_FCS | IEEE802154_HW_2_4_GHZ |
	       IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_FILTER |
	       IEEE802154_HW_ENERGY_SCAN | IEEE802154_HW_SLEEP_TO_TX;
}

static enum ieee802154_hw_caps get_capabilities_caps_mock(const struct device *dev)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");

	return ztest_get_return_value();
}

static int configure_mock(const struct device *dev,
			  enum ieee802154_config_type type,
			  const struct ieee802154_config *config)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");

	zassert_equal(type, IEEE802154_CONFIG_EVENT_HANDLER,
		      "Only event handler configuration was expected.");

	return 0;
}

static int configure_match_mock(const struct device *dev,
				enum ieee802154_config_type type,
				const struct ieee802154_config *config)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	ztest_check_expected_value(type);

	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		ztest_check_expected_value(config->auto_ack_fpb.mode);
		ztest_check_expected_value(config->auto_ack_fpb.enabled);
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		ztest_check_expected_value(config->ack_fpb.extended);
		ztest_check_expected_value(config->ack_fpb.enabled);
		ztest_check_expected_data(
			config->ack_fpb.addr,
			(config->ack_fpb.extended) ? sizeof(otExtAddress) : 2);
		break;
	default:
		zassert_unreachable("Unexpected config type %d.", type);
		break;
	}

	return 0;
}

static int configure_promiscuous_mock(const struct device *dev,
				      enum ieee802154_config_type type,
				      const struct ieee802154_config *config)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	zassert_equal(type, IEEE802154_CONFIG_PROMISCUOUS,
		      "Config type incorrect.");
	ztest_check_expected_value(config->promiscuous);

	return 0;
}

static int cca_mock(const struct device *dev)
{
	/* not using assert to verify function called */
	ztest_check_expected_value(dev);
	return 0;
}

static int filter_mock(const struct device *dev, bool set,
		       enum ieee802154_filter_type type,
		       const struct ieee802154_filter *filter)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	ztest_check_expected_value(set);
	ztest_check_expected_value(type);
	switch (type) {
	case IEEE802154_FILTER_TYPE_IEEE_ADDR:
		ztest_check_expected_data(filter->ieee_addr,
					  OT_EXT_ADDRESS_SIZE);
		break;
	case IEEE802154_FILTER_TYPE_SHORT_ADDR:
		ztest_check_expected_value(filter->short_addr);
		break;
	case IEEE802154_FILTER_TYPE_PAN_ID:
		ztest_check_expected_value(filter->pan_id);
		break;
	default:
		zassert_false(true, "Type not supported in mock: %d.", type);
		break;
	}
	return 0;
}

static int set_txpower_mock(const struct device *dev, int16_t dbm)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	ztest_check_expected_value(dbm);

	return 0;
}

static int tx_mock(const struct device *dev, enum ieee802154_tx_mode mode,
		   struct net_pkt *pkt, struct net_buf *frag)
{
	zassert_equal(dev, &radio, "Device handle incorrect.");
	ztest_check_expected_value(frag->data);

	return 0;
}

static int start_mock(const struct device *dev)
{
	ztest_check_expected_value(dev);
	return 0;
}

static int stop_mock(const struct device *dev)
{
	ztest_check_expected_value(dev);
	return 0;
}

const struct device *device_get_binding_stub(const char *name)
{
	return &radio;
}

otError otIp6Send(otInstance *aInstance, otMessage *aMessage)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	ztest_check_expected_value(aMessage);
	return ztest_get_return_value();
}

otMessage *otIp6NewMessage(otInstance *aInstance,
			   const otMessageSettings *aSettings)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
	return ip_msg;
}

otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
	void *buf = (void *)aBuf;

	ztest_check_expected_value(aMessage);
	ztest_check_expected_value(aLength);
	ztest_check_expected_data(buf, aLength);
	return ztest_get_return_value();
}

void otMessageFree(otMessage *aMessage)
{
	ztest_check_expected_value(aMessage);
}

void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
	zassert_equal(aInstance, ot, "Incorrect instance.");
}

/**
 * @brief Test for immediate energy scan
 * Tests for case when radio energy scan returns success at the first call.
 *
 */
static void test_energy_scan_immediate_test(void)
{
	const uint8_t chan = 10;
	const uint8_t dur = 100;
	const int16_t energy = -94;

	scan_done_cb = NULL;

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, chan);

	ztest_returns_value(scan_mock, 0);
	ztest_expect_value(scan_mock, duration, dur);

	zassert_equal(otPlatRadioEnergyScan(ot, chan, dur), OT_ERROR_NONE,
		      "Energy scan returned error.");
	zassert_not_null(scan_done_cb, "Scan callback not specified.");

	scan_done_cb(&radio, energy);
	make_sure_sem_set(K_NO_WAIT);

	ztest_expect_value(otPlatRadioEnergyScanDone, aEnergyScanMaxRssi,
			   energy);
	platformRadioProcess(ot);
}

/**
 * @brief Test for delayed energy scan
 * Tests for case when radio returns not being able to start energy scan and
 * the scan should be scheduled for later.
 *
 */
static void test_energy_scan_delayed_test(void)
{
	const uint8_t chan = 10;
	const uint8_t dur = 100;
	const int16_t energy = -94;

	scan_done_cb = NULL;

	/* request scan */
	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, chan);

	ztest_returns_value(scan_mock, -EBUSY);
	ztest_expect_value(scan_mock, duration, dur);

	zassert_equal(otPlatRadioEnergyScan(ot, chan, dur), OT_ERROR_NONE,
		      "Energy scan returned error.");
	zassert_not_null(scan_done_cb, "Scan callback not specified.");
	make_sure_sem_set(K_NO_WAIT);

	/* process reported event */
	ztest_returns_value(scan_mock, 0);
	ztest_expect_value(scan_mock, duration, dur);

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, chan);
	platformRadioProcess(ot);
	zassert_not_null(scan_done_cb, "Scan callback not specified.");

	/* invoke scan done */
	scan_done_cb(&radio, energy);
	make_sure_sem_set(K_NO_WAIT);

	ztest_expect_value(otPlatRadioEnergyScanDone, aEnergyScanMaxRssi,
			   energy);
	platformRadioProcess(ot);
}

static void create_ack_frame(void)
{
	struct net_pkt *packet;
	struct net_buf *buf;
	const uint8_t lqi = 230;
	const int8_t rssi = -80;

	packet = net_pkt_alloc(K_NO_WAIT);
	buf = net_pkt_get_reserve_tx_data(K_NO_WAIT);
	net_pkt_append_buffer(packet, buf);

	buf->len = ACK_PKT_LENGTH;
	buf->data[0] = FRAME_TYPE_ACK;

	net_pkt_set_ieee802154_rssi(packet, rssi);
	net_pkt_set_ieee802154_lqi(packet, lqi);
	zassert_equal(ieee802154_radio_handle_ack(NULL, packet), NET_OK,
		      "Handling ack failed.");
	net_pkt_unref(packet);
}

/**
 * @brief Test for tx data handling
 * Tests if OT frame is correctly passed to the radio driver.
 * Additionally verifies ACK frame passing back to the OT.
 *
 */
static void test_tx_test(void)
{
	const uint8_t chan = 20;
	uint8_t chan2 = chan - 1;
	const int8_t power = -3;

	otRadioFrame *frm = otPlatRadioGetTransmitBuffer(ot);

	zassert_not_null(frm, "Transmit buffer is null.");

	zassert_equal(otPlatRadioSetTransmitPower(ot, power), OT_ERROR_NONE,
		      "Failed to set TX power.");

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, chan);
	ztest_expect_value(set_txpower_mock, dbm, power);
	ztest_expect_value(start_mock, dev, &radio);
	zassert_equal(otPlatRadioReceive(ot, chan), OT_ERROR_NONE, "Failed to receive.");

	/* ACKed frame */
	frm->mChannel = chan2;
	frm->mInfo.mTxInfo.mCsmaCaEnabled = true;
	frm->mPsdu[0] = IEEE802154_AR_FLAG_SET;
	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, chan2);
	ztest_expect_value(cca_mock, dev, &radio);
	ztest_expect_value(tx_mock, frag->data, frm->mPsdu);
	ztest_expect_value(set_txpower_mock, dbm, power);
	zassert_equal(otPlatRadioTransmit(ot, frm), OT_ERROR_NONE, "Transmit failed.");

	create_ack_frame();
	make_sure_sem_set(Z_TIMEOUT_MS(100));

	ztest_expect_value(otPlatRadioTxDone, aError, OT_ERROR_NONE);
	platformRadioProcess(ot);

	/* Non-ACKed frame */
	frm->mChannel = --chan2;
	frm->mInfo.mTxInfo.mCsmaCaEnabled = false;
	frm->mPsdu[0] = 0;

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, chan2);
	ztest_expect_value(tx_mock, frag->data, frm->mPsdu);
	ztest_expect_value(set_txpower_mock, dbm, power);
	zassert_equal(otPlatRadioTransmit(ot, frm), OT_ERROR_NONE, "Transmit failed.");
	make_sure_sem_set(Z_TIMEOUT_MS(100));
	ztest_expect_value(otPlatRadioTxDone, aError, OT_ERROR_NONE);
	platformRadioProcess(ot);
}

/**
 * @brief Test for tx power setting
 * Tests if tx power requested by the OT is correctly passed to the radio.
 *
 */
static void test_tx_power_test(void)
{
	int8_t out_power = 0;

	zassert_equal(otPlatRadioSetTransmitPower(ot, -3), OT_ERROR_NONE,
		      "Failed to set TX power.");
	zassert_equal(otPlatRadioGetTransmitPower(ot, &out_power),
		      OT_ERROR_NONE, "Failed to obtain TX power.");
	zassert_equal(out_power, -3, "Got different power than set.");
	zassert_equal(otPlatRadioSetTransmitPower(ot, -6), OT_ERROR_NONE,
		      "Failed to set TX power.");
	zassert_equal(otPlatRadioGetTransmitPower(ot, &out_power),
		      OT_ERROR_NONE, "Failed to obtain TX power.");
	zassert_equal(out_power, -6,
		      "Second call to otPlatRadioSetTransmitPower failed.");
}

/**
 * @brief Test for getting radio sensitivity
 * There is no api to get radio sensitivity from the radio so the value is
 * hardcoded in radio.c. Test only verifies if the value returned makes any
 * sense.
 *
 */
static void test_sensitivity_test(void)
{
	/*
	 * Nothing to test actually as this is constant 100.
	 * When radio interface will be extended to get sensitivity this test
	 * can be extended with the radio api call. For now just verify if the
	 * value is reasonable.
	 */
	zassert_true(-80 > otPlatRadioGetReceiveSensitivity(ot),
		     "Radio sensitivity not in range.");
}

static void set_expected_match_values(enum ieee802154_config_type type,
				      uint8_t *addr, bool extended, bool enabled)
{
	ztest_expect_value(configure_match_mock, type, type);
	switch (type) {
	case IEEE802154_CONFIG_AUTO_ACK_FPB:
		ztest_expect_value(configure_match_mock,
				   config->auto_ack_fpb.enabled, enabled);
		ztest_expect_value(configure_match_mock,
				   config->auto_ack_fpb.mode,
				   IEEE802154_FPB_ADDR_MATCH_THREAD);
		break;
	case IEEE802154_CONFIG_ACK_FPB:
		ztest_expect_value(configure_match_mock,
				   config->ack_fpb.extended, extended);
		ztest_expect_value(configure_match_mock,
				   config->ack_fpb.enabled, enabled);
		ztest_expect_data(configure_match_mock, config->ack_fpb.addr,
				  addr);
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
static void test_source_match_test(void)
{
	otExtAddress ext_addr;

	rapi.configure = configure_match_mock;
	/* Enable/Disable */
	set_expected_match_values(IEEE802154_CONFIG_AUTO_ACK_FPB, NULL, false,
				  true);
	otPlatRadioEnableSrcMatch(ot, true);
	set_expected_match_values(IEEE802154_CONFIG_AUTO_ACK_FPB, NULL, false,
				  false);
	otPlatRadioEnableSrcMatch(ot, false);

	set_expected_match_values(IEEE802154_CONFIG_AUTO_ACK_FPB, NULL, false,
				  true);
	otPlatRadioEnableSrcMatch(ot, true);

	/* Add */
	sys_put_le16(12345, ext_addr.m8);
	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, false,
				  true);
	zassert_equal(otPlatRadioAddSrcMatchShortEntry(ot, 12345),
		      OT_ERROR_NONE, "Failed to add short src entry.");


	for (int i = 0; i < sizeof(ext_addr.m8); i++) {
		ext_addr.m8[i] = i;
	}
	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, true,
				  true);
	zassert_equal(otPlatRadioAddSrcMatchExtEntry(ot, &ext_addr),
		      OT_ERROR_NONE, "Failed to add ext src entry.");

	/* Clear */
	sys_put_le16(12345, ext_addr.m8);
	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, false,
				  false);
	zassert_equal(otPlatRadioClearSrcMatchShortEntry(ot, 12345),
		      OT_ERROR_NONE, "Failed to clear short src entry.");

	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, ext_addr.m8, true,
				  false);
	zassert_equal(otPlatRadioClearSrcMatchExtEntry(ot, &ext_addr),
		      OT_ERROR_NONE, "Failed to clear ext src entry.");

	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, NULL, false,
				  false);
	otPlatRadioClearSrcMatchShortEntries(ot);

	set_expected_match_values(IEEE802154_CONFIG_ACK_FPB, NULL, true, false);
	otPlatRadioClearSrcMatchExtEntries(ot);

	rapi.configure = configure_mock;
}

/**
 * @brief Test for enabling or disabling promiscuous mode
 * Tests if OT can successfully enable or disable promiscuous mode.
 *
 */
static void test_promiscuous_mode_set_test(void)
{
	rapi.configure = configure_promiscuous_mock;

	zassert_false(otPlatRadioGetPromiscuous(ot),
		      "By default promiscuous mode shall be disabled.");

	ztest_expect_value(configure_promiscuous_mock, config->promiscuous,
			   true);
	otPlatRadioSetPromiscuous(ot, true);
	zassert_true(otPlatRadioGetPromiscuous(ot), "Mode not enabled.");

	ztest_expect_value(configure_promiscuous_mock, config->promiscuous,
			   false);
	otPlatRadioSetPromiscuous(ot, false);
	zassert_false(otPlatRadioGetPromiscuous(ot), "Mode still enabled.");

	rapi.configure = configure_mock;
}

/**
 * @brief Test of proper radio to OT capabilities mapping
 * Tests if different radio capabilities map for their corresponding OpenThread
 * capability
 *
 */
static void test_get_caps_test(void)
{
	rapi.get_capabilities = get_capabilities_caps_mock;

	/* no caps */
	ztest_returns_value(get_capabilities_caps_mock, 0);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	/* not used by OT */
	ztest_returns_value(get_capabilities_caps_mock, IEEE802154_HW_FCS);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");
	ztest_returns_value(get_capabilities_caps_mock, IEEE802154_HW_2_4_GHZ);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");
	ztest_returns_value(get_capabilities_caps_mock, IEEE802154_HW_SUB_GHZ);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	/* not implemented or not fully supported */
	ztest_returns_value(get_capabilities_caps_mock, IEEE802154_HW_TXTIME);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	ztest_returns_value(get_capabilities_caps_mock, IEEE802154_HW_PROMISC);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_NONE,
		      "Incorrect capabilities returned.");

	/* proper mapping */
	ztest_returns_value(get_capabilities_caps_mock, IEEE802154_HW_CSMA);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_CSMA_BACKOFF,
		      "Incorrect capabilities returned.");

	ztest_returns_value(get_capabilities_caps_mock,
			    IEEE802154_HW_ENERGY_SCAN);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_ENERGY_SCAN,
		      "Incorrect capabilities returned.");

	ztest_returns_value(get_capabilities_caps_mock,
			    IEEE802154_HW_TX_RX_ACK);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_ACK_TIMEOUT,
		      "Incorrect capabilities returned.");

	ztest_returns_value(get_capabilities_caps_mock,
			    IEEE802154_HW_SLEEP_TO_TX);
	zassert_equal(otPlatRadioGetCaps(ot), OT_RADIO_CAPS_SLEEP_TO_TX,
		      "Incorrect capabilities returned.");

	/* all at once */
	ztest_returns_value(
		get_capabilities_caps_mock,
		IEEE802154_HW_FCS | IEEE802154_HW_PROMISC |
			IEEE802154_HW_FILTER | IEEE802154_HW_CSMA |
			IEEE802154_HW_2_4_GHZ | IEEE802154_HW_TX_RX_ACK |
			IEEE802154_HW_SUB_GHZ | IEEE802154_HW_ENERGY_SCAN |
			IEEE802154_HW_TXTIME | IEEE802154_HW_SLEEP_TO_TX);
	zassert_equal(
		otPlatRadioGetCaps(ot),
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
static void test_get_rssi_test(void)
{
	const int8_t rssi = -103;

	rapi.ed_scan = rssi_scan_mock;

	ztest_returns_value(rssi_scan_mock, rssi);
	zassert_equal(otPlatRadioGetRssi(ot), rssi,
		      "Invalid RSSI value received.");

	rapi.ed_scan = scan_mock;
}

/**
 * @brief Test switching between radio states
 * Tests if radio is correctly switched between states.
 *
 */
static void test_radio_state_test(void)
{
	const uint8_t channel = 12;
	const uint8_t power = 10;

	zassert_equal(otPlatRadioSetTransmitPower(ot, power), OT_ERROR_NONE,
		      "Failed to set TX power.");
	zassert_equal(otPlatRadioDisable(ot), OT_ERROR_NONE,
		      "Failed to disable radio.");

	zassert_false(otPlatRadioIsEnabled(ot), "Radio reports as enabled.");

	zassert_equal(otPlatRadioSleep(ot), OT_ERROR_INVALID_STATE,
		      "Changed to sleep regardless being disabled.");

	zassert_equal(otPlatRadioEnable(ot), OT_ERROR_NONE,
		      "Enabling radio failed.");

	zassert_true(otPlatRadioIsEnabled(ot), "Radio reports disabled.");

	ztest_expect_value(stop_mock, dev, &radio);
	zassert_equal(otPlatRadioSleep(ot), OT_ERROR_NONE,
		      "Failed to switch to sleep mode.");

	zassert_true(otPlatRadioIsEnabled(ot), "Radio reports as disabled.");

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, channel);
	ztest_expect_value(set_txpower_mock, dbm, power);
	ztest_expect_value(start_mock, dev, &radio);
	zassert_equal(otPlatRadioReceive(ot, channel), OT_ERROR_NONE,
		      "Failed to receive.");
	zassert_equal(platformRadioChannelGet(ot), channel,
		      "Channel number not remembered.");

	zassert_true(otPlatRadioIsEnabled(ot), "Radio reports as disabled.");
}

/**
 * @brief Test address filtering
 * Tests if short, extended address and PanID are correctly passed to the radio
 * driver.
 *
 */
static void test_address_test(void)
{
	const uint16_t pan_id = 0xDEAD;
	const uint16_t short_add = 0xCAFE;
	otExtAddress ieee_addr;

	for (int i = 0; i < sizeof(ieee_addr.m8); i++) {
		ieee_addr.m8[i] = 'a' + i;
	}

	ztest_expect_value(filter_mock, set, true);
	ztest_expect_value(filter_mock, type, IEEE802154_FILTER_TYPE_PAN_ID);
	ztest_expect_value(filter_mock, filter->pan_id, pan_id);
	otPlatRadioSetPanId(ot, pan_id);

	ztest_expect_value(filter_mock, set, true);
	ztest_expect_value(filter_mock, type,
			   IEEE802154_FILTER_TYPE_SHORT_ADDR);
	ztest_expect_value(filter_mock, filter->short_addr, short_add);
	otPlatRadioSetShortAddress(ot, short_add);

	ztest_expect_value(filter_mock, set, true);
	ztest_expect_value(filter_mock, type, IEEE802154_FILTER_TYPE_IEEE_ADDR);
	ztest_expect_data(filter_mock, filter->ieee_addr, ieee_addr.m8);
	otPlatRadioSetExtendedAddress(ot, &ieee_addr);
}


uint8_t alloc_pkt(struct net_pkt **out_packet, uint8_t buf_ct, uint8_t offset)
{
	struct net_pkt *packet;
	struct net_buf *buf;
	uint8_t len = 0;
	uint8_t buf_num;

	packet = net_pkt_alloc(K_NO_WAIT);
	for (buf_num = 0; buf_num < buf_ct; buf_num++) {
		buf = net_pkt_get_reserve_tx_data(K_NO_WAIT);
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
static void test_receive_test(void)
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

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, channel);
	ztest_expect_value(set_txpower_mock, dbm, power);
	ztest_expect_value(start_mock, dev, &radio);
	zassert_equal(otPlatRadioReceive(ot, channel), OT_ERROR_NONE,
		      "Failed to receive.");

	/*
	 * Not setting any expect values as nothing shall be called from
	 * notify_new_rx_frame calling thread. OT functions can be called only
	 * after semaphore for main thread is released.
	 */
	notify_new_rx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));
	ztest_expect_value(otPlatRadioReceiveDone, aError, OT_ERROR_NONE);
	ztest_expect_value(otPlatRadioReceiveDone, aFrame->mChannel, channel);
	ztest_expect_value(otPlatRadioReceiveDone, aFrame->mLength, len);
	ztest_expect_data(otPlatRadioReceiveDone, aFrame->mPsdu, buf->data);
	platformRadioProcess(ot);
}

/**
 * @brief Test received messages handling.
 * Tests if received frames are properly passed to the OpenThread
 *
 */
static void test_net_pkt_transmit(void)
{
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

	ztest_returns_value(set_channel_mock, 0);
	ztest_expect_value(set_channel_mock, channel, channel);
	ztest_expect_value(set_txpower_mock, dbm, power);
	ztest_expect_value(start_mock, dev, &radio);
	zassert_equal(otPlatRadioReceive(ot, channel), OT_ERROR_NONE,
		      "Failed to receive.");

	notify_new_tx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));

	ztest_expect_value(otMessageAppend, aMessage, ip_msg);
	ztest_expect_value(otMessageAppend, aLength, len);
	ztest_expect_data(otMessageAppend, buf, buf->data);
	ztest_returns_value(otMessageAppend, OT_ERROR_NONE);

	buf = buf->frags;
	ztest_expect_value(otMessageAppend, aMessage, ip_msg);
	ztest_expect_value(otMessageAppend, aLength, len);
	ztest_expect_data(otMessageAppend, buf, buf->data);
	ztest_returns_value(otMessageAppend, OT_ERROR_NONE);

	ztest_expect_value(otIp6Send, aMessage, ip_msg);
	ztest_returns_value(otIp6Send, OT_ERROR_NONE);

	/* Do not expect free in case of success */

	platformRadioProcess(ot);

	/* fail on append */
	len = alloc_pkt(&packet, 2, 'b');
	buf = packet->buffer;

	notify_new_tx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));

	ztest_expect_value(otMessageAppend, aMessage, ip_msg);
	ztest_expect_value(otMessageAppend, aLength, len);
	ztest_expect_data(otMessageAppend, buf, buf->data);
	ztest_returns_value(otMessageAppend, OT_ERROR_NO_BUFS);

	/* Expect free in case of failure in appending buffer*/
	ztest_expect_value(otMessageFree, aMessage, ip_msg);

	platformRadioProcess(ot);

	/* fail on send */
	len = alloc_pkt(&packet, 1, 'c');
	buf = packet->buffer;

	notify_new_tx_frame(packet);

	make_sure_sem_set(Z_TIMEOUT_MS(100));

	ztest_expect_value(otMessageAppend, aMessage, ip_msg);
	ztest_expect_value(otMessageAppend, aLength, len);
	ztest_expect_data(otMessageAppend, buf, buf->data);
	ztest_returns_value(otMessageAppend, OT_ERROR_NONE);

	ztest_expect_value(otIp6Send, aMessage, ip_msg);
	ztest_returns_value(otIp6Send, OT_ERROR_BUSY);

	/* Do not expect free in case of failure in send */

	platformRadioProcess(ot);

}

void test_main(void)
{
	platformRadioInit();

	ztest_test_suite(openthread_radio,
		ztest_unit_test(test_energy_scan_immediate_test),
		ztest_unit_test(test_energy_scan_delayed_test),
		ztest_unit_test(test_tx_test),
		ztest_unit_test(test_tx_power_test),
		ztest_unit_test(test_sensitivity_test),
		ztest_unit_test(test_source_match_test),
		ztest_unit_test(test_promiscuous_mode_set_test),
		ztest_unit_test(test_get_caps_test),
		ztest_unit_test(test_get_rssi_test),
		ztest_unit_test(test_radio_state_test),
		ztest_unit_test(test_address_test),
		ztest_unit_test(test_receive_test),
		ztest_unit_test(test_net_pkt_transmit));


	ztest_run_test_suite(openthread_radio);
}
