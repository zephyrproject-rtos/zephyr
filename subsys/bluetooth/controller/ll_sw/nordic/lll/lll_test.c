/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/toolchain.h>

#include <soc.h>

#include "hal/cpu.h"
#include "hal/cntr.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/radio_df.h"

#include "util/memq.h"
#include "util/util.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll_internal.h"

#if defined(CONFIG_BT_CTLR_DF)
#include "lll_df_types.h"
#include "lll_df.h"
#endif /* CONFIG_BT_CTLR_DF */

#include "ll_test.h"

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
#include "ull_df_types.h"
#include "ull_df_internal.h"
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

#include <zephyr/bluetooth/hci.h>

#include "hal/debug.h"

#define CNTR_MIN_DELTA 3

/* Helper definitions for repeated payload sequence */
#define PAYLOAD_11110000 0x0f
#define PAYLOAD_10101010 0x55
#define PAYLOAD_11111111 0xff
#define PAYLOAD_00000000 0x00
#define PAYLOAD_00001111 0xf0
#define PAYLOAD_01010101 0xaa

static const uint32_t test_sync_word = 0x71764129;
static uint8_t        test_phy;
static uint8_t        test_phy_flags;

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
static uint8_t        test_cte_type;
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_CTE_TX) || defined(CONFIG_BT_CTLR_DF_CTE_RX)
static uint8_t        test_cte_len;
#endif  /* CONFIG_BT_CTLR_DF_CTE_TX || CONFIG_BT_CTLR_DF_CTE_RX */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
static uint8_t        test_chan;
static uint8_t        test_slot_duration;
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

static uint32_t    tx_tifs;
static uint16_t    test_num_rx;
static bool        started;

/* NOTE: The PRBS9 sequence used as packet payload.
 * The bytes in the sequence are in the right order, but the bits of each byte
 * in the array are reverse from that found by running the PRBS9 algorithm. This
 * is done to transmit MSbit first on air.
 */

static const uint8_t prbs9[] = {
	0xFF, 0xC1, 0xFB, 0xE8, 0x4C, 0x90, 0x72, 0x8B,
	0xE7, 0xB3, 0x51, 0x89, 0x63, 0xAB, 0x23, 0x23,
	0x02, 0x84, 0x18, 0x72, 0xAA, 0x61, 0x2F, 0x3B,
	0x51, 0xA8, 0xE5, 0x37, 0x49, 0xFB, 0xC9, 0xCA,
	0x0C, 0x18, 0x53, 0x2C, 0xFD, 0x45, 0xE3, 0x9A,
	0xE6, 0xF1, 0x5D, 0xB0, 0xB6, 0x1B, 0xB4, 0xBE,
	0x2A, 0x50, 0xEA, 0xE9, 0x0E, 0x9C, 0x4B, 0x5E,
	0x57, 0x24, 0xCC, 0xA1, 0xB7, 0x59, 0xB8, 0x87,
	0xFF, 0xE0, 0x7D, 0x74, 0x26, 0x48, 0xB9, 0xC5,
	0xF3, 0xD9, 0xA8, 0xC4, 0xB1, 0xD5, 0x91, 0x11,
	0x01, 0x42, 0x0C, 0x39, 0xD5, 0xB0, 0x97, 0x9D,
	0x28, 0xD4, 0xF2, 0x9B, 0xA4, 0xFD, 0x64, 0x65,
	0x06, 0x8C, 0x29, 0x96, 0xFE, 0xA2, 0x71, 0x4D,
	0xF3, 0xF8, 0x2E, 0x58, 0xDB, 0x0D, 0x5A, 0x5F,
	0x15, 0x28, 0xF5, 0x74, 0x07, 0xCE, 0x25, 0xAF,
	0x2B, 0x12, 0xE6, 0xD0, 0xDB, 0x2C, 0xDC, 0xC3,
	0x7F, 0xF0, 0x3E, 0x3A, 0x13, 0xA4, 0xDC, 0xE2,
	0xF9, 0x6C, 0x54, 0xE2, 0xD8, 0xEA, 0xC8, 0x88,
	0x00, 0x21, 0x86, 0x9C, 0x6A, 0xD8, 0xCB, 0x4E,
	0x14, 0x6A, 0xF9, 0x4D, 0xD2, 0x7E, 0xB2, 0x32,
	0x03, 0xC6, 0x14, 0x4B, 0x7F, 0xD1, 0xB8, 0xA6,
	0x79, 0x7C, 0x17, 0xAC, 0xED, 0x06, 0xAD, 0xAF,
	0x0A, 0x94, 0x7A, 0xBA, 0x03, 0xE7, 0x92, 0xD7,
	0x15, 0x09, 0x73, 0xE8, 0x6D, 0x16, 0xEE, 0xE1,
	0x3F, 0x78, 0x1F, 0x9D, 0x09, 0x52, 0x6E, 0xF1,
	0x7C, 0x36, 0x2A, 0x71, 0x6C, 0x75, 0x64, 0x44,
	0x80, 0x10, 0x43, 0x4E, 0x35, 0xEC, 0x65, 0x27,
	0x0A, 0xB5, 0xFC, 0x26, 0x69, 0x3F, 0x59, 0x99,
	0x01, 0x63, 0x8A, 0xA5, 0xBF, 0x68, 0x5C, 0xD3,
	0x3C, 0xBE, 0x0B, 0xD6, 0x76, 0x83, 0xD6, 0x57,
	0x05, 0x4A, 0x3D, 0xDD, 0x81, 0x73, 0xC9, 0xEB,
	0x8A, 0x84, 0x39, 0xF4, 0x36, 0x0B, 0xF7};

/* TODO: fill correct prbs15 */
static const uint8_t prbs15[255] = { 0x00, };

static uint8_t tx_req;
static uint8_t volatile tx_ack;

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
static bool check_rx_cte(bool cte_ready);
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
static int create_iq_report(bool crc_ok)
{
	struct node_rx_iq_report *iq_report;
	struct node_rx_ftr *ftr;
	uint8_t sample_cnt;
	uint8_t cte_info;
	uint8_t ant;

	sample_cnt = radio_df_iq_samples_amount_get();

	/* If there are no samples available, the CTEInfo was
	 * not detected and sampling was not started.
	 */
	if (!sample_cnt) {
		return -EINVAL;
	}

	cte_info = radio_df_cte_status_get();
	ant = radio_df_pdu_antenna_switch_pattern_get();
	iq_report = ull_df_iq_report_alloc();
	if (!iq_report) {
		return -ENOBUFS;
	}

	iq_report->hdr.type = NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT;
	iq_report->sample_count = sample_cnt;
	iq_report->packet_status = ((crc_ok) ? BT_HCI_LE_CTE_CRC_OK :
					       BT_HCI_LE_CTE_CRC_ERR_CTE_BASED_TIME);
	iq_report->rssi_ant_id = ant;
	iq_report->cte_info = *(struct pdu_cte_info *)&cte_info;
	iq_report->local_slot_durations = test_slot_duration;
	iq_report->chan_idx = test_chan;

	ftr = &iq_report->hdr.rx_ftr;
	ftr->param = NULL;
	ftr->rssi = BT_HCI_LE_RSSI_NOT_AVAILABLE;

	ull_rx_put(iq_report->hdr.link, iq_report);

	return 0;
}
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

static void isr_tx(void *param)
{
	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_lna_disable();
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

	/* Exit if radio disabled */
	if (((tx_req - tx_ack) & 0x01) == 0U) {
		tx_ack = tx_req;

		return;
	}

	/* Setup next Tx */
	radio_tmr_tifs_set(tx_tifs);
	radio_switch_complete_and_b2b_tx(test_phy, test_phy_flags, test_phy, test_phy_flags);

	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
				 tx_tifs -
				 radio_tx_ready_delay_get(test_phy, test_phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */
}

static void isr_rx(void *param)
{
	uint8_t crc_ok = 0U;
	uint8_t trx_done;

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	struct node_rx_iq_report *node_rx;
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
	bool cte_ready;
	bool cte_ok = 0;
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
		cte_ready = radio_df_cte_ready();
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */
	}

	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();

	/* Exit if radio disabled */
	if (!trx_done) {
		return;
	}

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	if (test_cte_len > 0) {
		/* Get free iq report node for next Rx operation. */
		node_rx = ull_df_iq_report_alloc_peek(1);
		LL_ASSERT(node_rx);

		radio_df_iq_data_packet_set(node_rx->pdu, IQ_SAMPLE_TOTAL_CNT);
	}
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

	/* Setup next Rx */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_b2b_rx(test_phy, test_phy_flags, test_phy, test_phy_flags);

	/* Count Rx-ed packets */
	if (crc_ok) {

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
		if (test_cte_len > 0) {
			cte_ok = check_rx_cte(cte_ready);
			if (cte_ok) {
				test_num_rx++;
			}
		} else
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */

		{
			test_num_rx++;
		}
	}

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	if (cte_ok) {
		int err;

		err = create_iq_report(crc_ok);
		if (!err) {
			ull_rx_sched();
		}
	}
#endif
}

static uint8_t tx_power_check(int8_t tx_power)
{
	if ((tx_power != BT_HCI_TX_TEST_POWER_MIN_SET) &&
	    (tx_power != BT_HCI_TX_TEST_POWER_MAX_SET) &&
	    ((tx_power < BT_HCI_TX_TEST_POWER_MIN) || (tx_power > BT_HCI_TX_TEST_POWER_MAX))) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	return BT_HCI_ERR_SUCCESS;
}

static uint8_t tx_power_set(int8_t tx_power)
{
	uint8_t err;

	err = tx_power_check(tx_power);
	if (err) {
		return err;
	}

	if (tx_power == BT_HCI_TX_TEST_POWER_MAX_SET) {
		tx_power = radio_tx_power_max_get();
	} else if (tx_power == BT_HCI_TX_TEST_POWER_MIN_SET) {
		tx_power = radio_tx_power_min_get();
	} else {
		tx_power = radio_tx_power_floor(tx_power);
	}

	radio_tx_power_set(tx_power);

	return err;
}

#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
static uint8_t cte_tx_parameters_check(uint8_t cte_len, uint8_t cte_type,
					uint8_t switch_pattern_len)
{
	if ((cte_type > BT_HCI_LE_AOD_CTE_2US) ||
	    (cte_len < BT_HCI_LE_CTE_LEN_MIN) ||
	    (cte_len > BT_HCI_LE_CTE_LEN_MAX)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (cte_type != BT_HCI_LE_AOA_CTE) {
		if ((switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN) ||
		    (switch_pattern_len > BT_HCI_LE_SWITCH_PATTERN_LEN_MAX)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
	}

	if ((cte_type == BT_HCI_LE_AOD_CTE_1US) &&
	    !IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	return BT_HCI_ERR_SUCCESS;
}

static uint8_t cte_tx_init(uint8_t cte_len, uint8_t cte_type, uint8_t switch_pattern_len,
			   const uint8_t *ant_id)
{
	uint8_t err;

	err = cte_tx_parameters_check(cte_len, cte_type, switch_pattern_len);
	if (err) {
		return err;
	}

	if (cte_type == BT_HCI_LE_AOA_CTE) {
		radio_df_mode_set_aoa();
		radio_df_cte_tx_aoa_set(cte_len);
	} else {
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)
		radio_df_mode_set_aod();

		if (cte_type == BT_HCI_LE_AOD_CTE_1US) {
			radio_df_cte_tx_aod_2us_set(cte_len);
		} else {
			radio_df_cte_tx_aod_4us_set(cte_len);
		}

		radio_df_ant_switching_pin_sel_cfg();
		radio_df_ant_switch_pattern_clear();
		radio_df_ant_switch_pattern_set(ant_id, switch_pattern_len);
#else
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_TX */
	}

	return err;
}
#endif /* CONFIG_BT_CTLR_DF_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
static uint8_t cte_rx_parameters_check(uint8_t expected_cte_len, uint8_t expected_cte_type,
					uint8_t slot_duration, uint8_t switch_pattern_len)
{
	if ((expected_cte_type > BT_HCI_LE_AOD_CTE_2US) ||
	    (expected_cte_len < BT_HCI_LE_CTE_LEN_MIN) ||
	    (expected_cte_len > BT_HCI_LE_CTE_LEN_MAX)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (expected_cte_type == BT_HCI_LE_AOA_CTE) {
		if ((switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN) ||
		    (switch_pattern_len > BT_HCI_LE_SWITCH_PATTERN_LEN_MAX)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if ((slot_duration == 0) ||
		    (slot_duration > BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
	}

	if ((slot_duration == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US) &&
	    (!IS_ENABLED(CONFIG_BT_CTLR_DF_CTE_RX_SAMPLE_1US))) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	return BT_HCI_ERR_SUCCESS;
}

static uint8_t cte_rx_init(uint8_t expected_cte_len, uint8_t expected_cte_type,
			   uint8_t slot_duration, uint8_t switch_pattern_len,
			   const uint8_t *ant_ids, uint8_t phy)
{
	uint8_t err;
	uint8_t cte_phy = (phy == BT_HCI_LE_RX_PHY_1M) ? PHY_1M : PHY_2M;

	err = cte_rx_parameters_check(expected_cte_len, expected_cte_type,
				      slot_duration, switch_pattern_len);
	if (err) {
		return err;
	}

	if (slot_duration == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US) {
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US)
		radio_df_cte_rx_2us_switching(true, cte_phy);
#else
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_1US */
	} else {
		radio_df_cte_rx_4us_switching(true, cte_phy);
	}

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	radio_df_ant_switching_pin_sel_cfg();
	radio_df_ant_switch_pattern_clear();
	radio_df_ant_switch_pattern_set(ant_ids, switch_pattern_len);
#else
	return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	struct node_rx_iq_report *node_rx;

	node_rx = ull_df_iq_report_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_df_iq_data_packet_set(node_rx->pdu, IQ_SAMPLE_TOTAL_CNT);
#else
	radio_df_iq_data_packet_set(NULL, 0);
#endif

	return err;
}

static bool check_rx_cte(bool cte_ready)
{
	uint8_t cte_status;
	struct pdu_cte_info *cte_info;

	cte_status = radio_df_cte_status_get();
	cte_info = (struct pdu_cte_info *)&cte_status;

	if ((cte_info->type != test_cte_type) || (cte_info->time != test_cte_len)) {
		return false;
	}

	return true;
}
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */

static uint32_t calculate_tifs(uint8_t len)
{
	uint32_t interval;
	uint32_t transmit_time;

#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
	/* Include additional byte for the CTEInfo field and CTE length in microseconds. */
	transmit_time = PDU_US((test_cte_len > 0) ? (len + 1) : len, 0, test_phy, test_phy_flags) +
			CTE_LEN_US(test_cte_len);
#else
	transmit_time = PDU_US(len, 0, test_phy, test_phy_flags);
#endif /* CONFIG_BT_CTLR_DF_CTE_TX */

	/* Ble Core Specification Vol 6 Part F 4.1.6
	 * LE Test packet interval: I(L) = ceil((L + 249) / 625) * 625 us
	 * where L is an LE Test packet length in microseconds unit.
	 */
	interval = ceiling_fraction((transmit_time + 249), SCAN_INT_UNIT_US) * SCAN_INT_UNIT_US;

	return interval - transmit_time;
}

static uint8_t init(uint8_t chan, uint8_t phy, int8_t tx_power,
		    bool cte, void (*isr)(void *))
{
	int err;
	uint8_t ret;

	if (started) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* start coarse timer */
	cntr_start();

	/* Setup resources required by Radio */
	err = lll_hfclock_on_wait();
	LL_ASSERT(err >= 0);

	/* Reset Radio h/w */
	radio_reset();
	radio_isr_set(isr, NULL);

#if defined(CONFIG_BT_CTLR_DF)
	/* Reset  Radio DF */
	radio_df_reset();
#endif

	/* Store value needed in Tx/Rx ISR */
	if (phy < BT_HCI_LE_TX_PHY_CODED_S2) {
		test_phy = BIT(phy - 1);
		test_phy_flags = 1U;
	} else {
		test_phy = BIT(2);
		test_phy_flags = 0U;
	}

	/* Setup Radio in Tx/Rx */
	/* NOTE: No whitening in test mode. */
	radio_phy_set(test_phy, test_phy_flags);

	ret = tx_power_set(tx_power);

	radio_freq_chan_set((chan << 1) + 2);
	radio_aa_set((uint8_t *)&test_sync_word);
	radio_crc_configure(0x65b, PDU_AC_CRC_IV);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_DTM_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(test_phy) |
			    RADIO_PKT_CONF_PDU_TYPE(IS_ENABLED(CONFIG_BT_CTLR_DF_CTE_TX) ?
								RADIO_PKT_CONF_PDU_TYPE_DC :
								RADIO_PKT_CONF_PDU_TYPE_AC) |
			    RADIO_PKT_CONF_CTE(cte ? RADIO_PKT_CONF_CTE_ENABLED :
						     RADIO_PKT_CONF_CTE_DISABLED));

	return ret;
}

static void payload_set(uint8_t type, uint8_t len, uint8_t cte_len, uint8_t cte_type)
{
	struct pdu_dtm *pdu = radio_pkt_scratch_get();

	pdu->type = type;
	pdu->len = len;

#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
	pdu->cp = cte_len ? 1U : 0U;
	pdu->octet3.cte_info.time = cte_len;
	pdu->octet3.cte_info.type = cte_type;
#else
	ARG_UNUSED(cte_len);
	ARG_UNUSED(cte_type);
#endif /* CONFIG_BT_CTLR_DF_CTE_TX */

	switch (type) {
	case BT_HCI_TEST_PKT_PAYLOAD_PRBS9:
		(void)memcpy(pdu->payload, prbs9, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_11110000:
		(void)memset(pdu->payload, PAYLOAD_11110000, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_10101010:
		(void)memset(pdu->payload, PAYLOAD_10101010, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_PRBS15:
		(void)memcpy(pdu->payload, prbs15, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_11111111:
		(void)memset(pdu->payload, PAYLOAD_11111111, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_00000000:
		(void)memset(pdu->payload, PAYLOAD_00000000, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_00001111:
		(void)memset(pdu->payload, PAYLOAD_00001111, len);
		break;

	case BT_HCI_TEST_PKT_PAYLOAD_01010101:
		(void)memset(pdu->payload, PAYLOAD_01010101, len);
		break;
	}

	radio_pkt_tx_set(pdu);
}

uint8_t ll_test_tx(uint8_t chan, uint8_t len, uint8_t type, uint8_t phy,
		   uint8_t cte_len, uint8_t cte_type, uint8_t switch_pattern_len,
		   const uint8_t *ant_id, int8_t tx_power)
{
	uint32_t start_us;
	uint8_t err;
	const bool cte_request = (cte_len > 0) ? true : false;

	if ((type > BT_HCI_TEST_PKT_PAYLOAD_01010101) || !phy ||
	    (phy > BT_HCI_LE_TX_PHY_CODED_S2)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	err = init(chan, phy, tx_power, cte_request, isr_tx);
	if (err) {
		return err;
	}

	/* Configure Constant Tone Extension */
	if (cte_request) {
#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
		if (phy > BT_HCI_LE_TX_PHY_2M) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		err = cte_tx_init(cte_len, cte_type, switch_pattern_len, ant_id);
		if (err) {
			return err;
		}
#else
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* CONFIG_BT_CTLR_DF_CTE_TX */
	}

	tx_req++;

	payload_set(type, len, cte_len, cte_type);

#if defined(CONFIG_BT_CTLR_DF_CTE_TX)
	/* Store CTE parameters needed in Tx ISR */
	test_cte_len = cte_len;
#endif /* CONFIG_BT_CTLR_DF_CTE_TX */

	tx_tifs = calculate_tifs(len);

	radio_tmr_tifs_set(tx_tifs);
	radio_switch_complete_and_b2b_tx(test_phy, test_phy_flags, test_phy, test_phy_flags);

	start_us = radio_tmr_start(1, cntr_cnt_get() + CNTR_MIN_DELTA, 0);
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_tx_ready_delay_get(test_phy,
							  test_phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

	started = true;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_test_rx(uint8_t chan, uint8_t phy, uint8_t mod_idx, uint8_t expected_cte_len,
		   uint8_t expected_cte_type, uint8_t slot_duration, uint8_t switch_pattern_len,
		   const uint8_t *ant_ids)
{
	uint8_t err;
	const bool cte_expected = (expected_cte_len > 0) ? true : false;

	if (!phy || (phy > BT_HCI_LE_RX_PHY_CODED)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	err = init(chan, phy, BT_HCI_TX_TEST_POWER_MAX_SET, cte_expected, isr_rx);
	if (err) {
		return err;
	}

	if (cte_expected) {
#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
		if (phy == BT_HCI_LE_RX_PHY_CODED) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		err = cte_rx_init(expected_cte_len, expected_cte_type, slot_duration,
				  switch_pattern_len, ant_ids, phy);
		if (err) {
			return err;
		}
#else
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */
	}

#if defined(CONFIG_BT_CTLR_DF_CTE_RX)
	/* Store CTE parameters needed in Rx ISR */
	test_cte_type = expected_cte_type;
	test_cte_len = expected_cte_len;
#endif /* CONFIG_BT_CTLR_DF_CTE_RX */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	test_chan = chan;
	test_slot_duration = slot_duration;
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

	radio_pkt_rx_set(radio_pkt_scratch_get());
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_b2b_rx(test_phy, test_phy_flags, test_phy, test_phy_flags);
	radio_tmr_start(0, cntr_cnt_get() + CNTR_MIN_DELTA, 0);

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_on();
#endif /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */

	started = true;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_test_end(uint16_t *num_rx)
{
	int err;
	uint8_t ack;

	if (!started) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Return packets Rx-ed/Completed */
	*num_rx = test_num_rx;
	test_num_rx = 0U;

	/* Disable Radio, if in Rx test */
	ack = tx_ack;
	if (tx_req == ack) {
		radio_disable();
	} else {
		/* Wait for Tx to complete */
		tx_req = ack + 2;
		while (tx_req != tx_ack) {
			cpu_sleep();
		}
	}

	/* Stop packet timer */
	radio_tmr_stop();

	/* Release resources acquired for Radio */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	/* Stop coarse timer */
	cntr_stop();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_off();
#endif /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */

	started = false;

	return BT_HCI_ERR_SUCCESS;
}
