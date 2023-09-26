/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <soc.h>
#include <zephyr/bluetooth/hci_types.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/dbuf.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio_df.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_df_types.h"
#include "lll_sync.h"
#include "lll_df.h"
#include "lll_df_internal.h"

#include <soc.h>
#include "hal/debug.h"

/* Minimum number of antenna switch patterns required by Direction Finding Extension to be
 * configured in SWTICHPATTER register. The value is set to 2, even though the radio peripheral
 * specification requires 3.
 * Radio always configures three antenna patterns. First pattern is set implicitly in
 * radio_df_ant_switch_pattern_set. It is provided by DTS radio.dfe_pdu_antenna property.
 * There is a need for two more patterns to be provided by an application.
 * They are aimed for: reference period and switch-sample period.
 */
#define DF_MIN_ANT_NUM_REQUIRED 2

static int init_reset(void);

/* @brief Function performs Direction Finding initialization
 *
 * @return	Zero in case of success, other value in case of failure.
 */
int lll_df_init(void)
{
#if defined(CONFIG_BT_CTLR_DF_INIT_ANT_SEL_GPIOS)
	radio_df_ant_switching_gpios_cfg();
#endif /* CONFIG_BT_CTLR_DF_INIT_ANT_SEL_GPIOS */
	return init_reset();
}

/* @brief Function performs Direction Finding reset
 *
 * @return	Zero in case of success, other value in case of failure.
 */
int lll_df_reset(void)
{
	return init_reset();
}

/* @brief Function provides number of available antennas.
 *
 * The number of antenna is hardware defined and it is provided via devicetree.
 *
 * @return      Number of available antennas.
 */
uint8_t lll_df_ant_num_get(void)
{
	return radio_df_ant_num_get();
}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* @brief Function enables transmission of Constant Tone Extension.
 *
 * @param lll_sync        Pointer to LLL sync. object associated with
 *                        periodic advertising event.
 * @param pdu             Pointer to PDU that will be transmitted.
 * @param[out] cte_len_us Pointer to store actual CTE length in [us]
 */
void lll_df_cte_tx_enable(struct lll_adv_sync *lll_sync, const struct pdu_adv *pdu,
			  uint32_t *cte_len_us)
{
	if (pdu->adv_ext_ind.ext_hdr_len) {
		const struct pdu_adv_ext_hdr *ext_hdr;

		ext_hdr = &pdu->adv_ext_ind.ext_hdr;

		/* Check if particular extended PDU has cte_info. It is possible that
		 * not all PDUs in a periodic advertising chain have CTE attached.
		 * Nevertheless whole periodic advertising chain has the same CTE
		 * configuration. Checking for existence of CTE configuration in lll_sync
		 * is not enough.
		 */
		if (ext_hdr->cte_info) {
			const struct lll_df_adv_cfg *df_cfg;

			df_cfg = lll_adv_sync_extra_data_curr_get(lll_sync);
			LL_ASSERT(df_cfg);

			lll_df_cte_tx_configure(df_cfg->cte_type, df_cfg->cte_length,
						df_cfg->ant_sw_len, df_cfg->ant_ids);

			lll_sync->cte_started = 1U;
			*cte_len_us = CTE_LEN_US(df_cfg->cte_length);
		} else {
			if (lll_sync->cte_started) {
				lll_df_cte_tx_disable();
				lll_sync->cte_started = 0U;
			}
			*cte_len_us = 0U;
		}
	} else {
		if (lll_sync->cte_started) {
			lll_df_cte_tx_disable();
			lll_sync->cte_started = 0U;
		}
		*cte_len_us = 0U;
	}
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
struct lll_df_sync_cfg *lll_df_sync_cfg_alloc(struct lll_df_sync *df_cfg,
					      uint8_t *idx)
{
	uint8_t first, last;

	/* TODO: Make this unique mechanism to update last element in double
	 *       buffer a re-usable utility function.
	 */
	first = df_cfg->first;
	last = df_cfg->last;
	if (first == last) {
		/* Return the index of next free PDU in the double buffer */
		last++;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0U;
		}
	} else {
		uint8_t first_latest;

		/* LLL has not consumed the first PDU. Revert back the `last` so
		 * that LLL still consumes the first PDU while the caller of
		 * this function updates/modifies the latest PDU.
		 *
		 * Under race condition:
		 * 1. LLL runs before `pdu->last` is reverted, then `pdu->first`
		 *    has changed, hence restore `pdu->last` and return index of
		 *    next free PDU in the double buffer.
		 * 2. LLL runs after `pdu->last` is reverted, then `pdu->first`
		 *    will not change, return the saved `last` as the index of
		 *    the next free PDU in the double buffer.
		 */
		df_cfg->last = first;
		cpu_dmb();
		first_latest = df_cfg->first;
		if (first_latest != first) {
			df_cfg->last = last;
			last++;
			if (last == DOUBLE_BUFFER_SIZE) {
				last = 0U;
			}
		}
	}

	*idx = last;

	return &df_cfg->cfg[last];
}

struct lll_df_sync_cfg *lll_df_sync_cfg_latest_get(struct lll_df_sync *df_cfg,
						   uint8_t *is_modified)
{
	uint8_t first;

	first = df_cfg->first;
	if (first != df_cfg->last) {
		uint8_t cfg_idx;

		cfg_idx = first;

		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
		df_cfg->first = first;

		if (is_modified) {
			*is_modified = 1U;
		}

		df_cfg->cfg[cfg_idx].is_enabled = 0U; /* mark as disabled - not used anymore */
	}

	return &df_cfg->cfg[first];
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
/* @brief Function initializes reception of Constant Tone Extension.
 *
 * @param slot_duration     Switching and sampling slots duration (1us or 2us).
 * @param ant_num           Number of antennas in switch pattern.
 * @param ant_ids           Antenna identifiers that create switch pattern.
 * @param chan_idx          Channel used to receive PDU with CTE
 * @param cte_info_in_s1    Inform if CTEInfo is in S1 byte for conn. PDU or in extended advertising
 *                          header of per. adv. PDU.
 * @param phy               Current PHY
 *
 * In case of AoA mode ant_num and ant_ids parameters are not used.
 */
int lll_df_conf_cte_rx_enable(uint8_t slot_duration, uint8_t ant_num, const uint8_t *ant_ids,
			      uint8_t chan_idx, bool cte_info_in_s1, uint8_t phy)
{
	struct node_rx_iq_report *node_rx;

	/* ToDo change to appropriate HCI constant */
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US)
	if (slot_duration == 0x1) {
		radio_df_cte_rx_2us_switching(cte_info_in_s1, phy);
	} else
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_1US */
	{
		radio_df_cte_rx_4us_switching(cte_info_in_s1, phy);
	}

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	radio_df_ant_switching_pin_sel_cfg();
	radio_df_ant_switch_pattern_clear();
	radio_df_ant_switch_pattern_set(ant_ids, ant_num);
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

	/* Could be moved up, if Radio setup is not needed if we are not going to report IQ data */
	node_rx = ull_df_iq_report_alloc_peek(1);
	if (!node_rx) {
		return -ENOMEM;
	}

	radio_df_iq_data_packet_set(node_rx->pdu, IQ_SAMPLE_TOTAL_CNT);
	node_rx->chan_idx = chan_idx;

	return 0;
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
/**
 * @brief Function allocates additional IQ report node for Host notification about
 * insufficient resources to sample all CTE in a periodic synchronization event.
 *
 * @param sync_lll Pointer to periodic synchronization object
 *
 * @return -ENOMEM in case there is no free node for IQ Data report
 * @return -ENOBUFS in case there are no free nodes for report of insufficient resources as well as
 * IQ data report
 * @return zero in case of success
 */
int lll_df_iq_report_no_resources_prepare(struct lll_sync *sync_lll)
{
	struct node_rx_iq_report *cte_incomplete;
	int err;

	/* Allocate additional node for a sync context only once. This is an additional node to
	 * report there is no more memory to store IQ data, hence some of CTEs are not going
	 * to be sampled.
	 */
	if (!sync_lll->node_cte_incomplete && !sync_lll->is_cte_incomplete) {
		/* Check if there are free nodes for:
		 * - storage of IQ data collcted during a PDU reception
		 * - Host notification about insufficient resources for IQ data
		 */
		cte_incomplete = ull_df_iq_report_alloc_peek(2);
		if (!cte_incomplete) {
			/* Check if there is a free node to report insufficient resources only.
			 * There will be no IQ Data collection.
			 */
			cte_incomplete = ull_df_iq_report_alloc_peek(1);
			if (!cte_incomplete) {
				/* No free nodes at all */
				return -ENOBUFS;
			}

			/* No memory for IQ data report */
			err = -ENOMEM;
		} else {
			err = 0;
		}

		/* Do actual allocation and store the node for futher processing after a PDU
		 * reception,
		 */
		ull_df_iq_report_alloc();

		/* Store the node in lll_sync object. This is a place where the node may be stored
		 * until processing afte reception of a PDU to report no IQ data or hand over
		 * to aux objects for usage in ULL. If there is not enough memory for IQ data
		 * there is no node to use for temporary storage as it is done for PDUs.
		 */
		sync_lll->node_cte_incomplete = cte_incomplete;

		/* Reset the state every time the prepare is called. IQ report node may be unchanged
		 * from former synchronization event.
		 */
		sync_lll->is_cte_incomplete = false;
	} else {
		err = 0;
	}

	return err;
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX  */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
/**
 * @brief Function initializes parsing of received PDU for CTEInfo.
 *
 * Parsing a PDU for CTEInfo is required to successfully receive a PDU that may include CTE.
 * It makes possible to correctly interpret PDU content by Radio peripheral.
 */
void lll_df_conf_cte_info_parsing_enable(void)
{
	/* Use of mandatory 2 us switching and sampling slots for CTEInfo parsing.
	 * The configuration here does not matter for actual IQ sampling.
	 * The collected data will not be reported to host.
	 * Also sampling offset does not matter so, provided PHY is legacy to setup
	 * the offset to default zero value.
	 */
	radio_df_cte_rx_4us_switching(true, PHY_LEGACY);

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	/* Use PDU_ANTENNA so no actual antenna change will be done. */
	static uint8_t ant_ids[DF_MIN_ANT_NUM_REQUIRED] = { PDU_ANTENNA, PDU_ANTENNA };

	radio_df_ant_switching_pin_sel_cfg();
	radio_df_ant_switch_pattern_clear();
	radio_df_ant_switch_pattern_set(ant_ids, DF_MIN_ANT_NUM_REQUIRED);
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

	/* Do not set storage for IQ samples, it is irrelevant for parsing of a PDU for CTEInfo. */
	radio_df_iq_data_packet_set(NULL, 0);
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

/* @brief Function performs common steps for initialization and reset
 * of Direction Finding LLL module.
 *
 * @return Zero in case of success, other value in case of failure.
 */
static int init_reset(void)
{
	return 0;
}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
/**
 * @brief Function configures transmission of Constant Tone Extension.
 *
 * @param cte_type    Type of the CTE
 * @param cte_length  Length of CTE in units of 8 us
 * @param num_ant_ids Length of @p ant_ids
 * @param ant_ids     Pointer to antenna identifiers array
 *
 * In case of AoA mode ant_sw_len and ant_ids members are not used.
 */
void lll_df_cte_tx_configure(uint8_t cte_type, uint8_t cte_length, uint8_t num_ant_ids,
			     const uint8_t *ant_ids)
{
	if (cte_type == BT_HCI_LE_AOA_CTE) {
		radio_df_mode_set_aoa();
		radio_df_cte_tx_aoa_set(cte_length);
	}
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)
	else {
		radio_df_mode_set_aod();

		if (cte_type == BT_HCI_LE_AOD_CTE_1US) {
			radio_df_cte_tx_aod_2us_set(cte_length);
		} else {
			radio_df_cte_tx_aod_4us_set(cte_length);
		}

		radio_df_ant_switching_pin_sel_cfg();
		radio_df_ant_switch_pattern_clear();
		radio_df_ant_switch_pattern_set(ant_ids, num_ant_ids);
	}
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_TX */
}

void lll_df_cte_tx_disable(void)
{
	radio_df_reset();
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX || CONFIG_BT_CTLR_DF_CONN_CTE_TX */
