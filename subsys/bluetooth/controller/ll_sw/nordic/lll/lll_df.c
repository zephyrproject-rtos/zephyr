/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <soc.h>
#include <bluetooth/hci.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"

#include "hal/cpu.h"
#include "hal/radio_df.h"

#include "pdu.h"

#include "lll.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_df.h"
#include "lll_df_types.h"
#include "lll_df_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
#define LOG_MODULE_NAME bt_ctlr_lll_df
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
static void df_cte_tx_configure(const struct lll_df_adv_cfg *df_cfg);
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
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

			df_cte_tx_configure(df_cfg);

			lll_sync->cte_started = 1U;
			*cte_len_us = CTE_LEN_US(df_cfg->cte_length);
		} else {
			if (lll_sync->cte_started) {
				lll_df_conf_cte_tx_disable();
				lll_sync->cte_started = 0U;
			}
			*cte_len_us = 0U;
		}
	} else {
		if (lll_sync->cte_started) {
			lll_df_conf_cte_tx_disable();
			lll_sync->cte_started = 0U;
		}
		*cte_len_us = 0U;
	}
}

void lll_df_conf_cte_tx_disable(void)
{
	radio_df_reset();
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

/* @brief Function initializes reception of Constant Tone Extension.
 *
 * @param slot_duration     Switching and sampling slots duration (1us or 2us).
 * @param ant_num           Number of antennas in switch pattern.
 * @param ant_ids           Antenna identifiers that create switch pattern.
 * @param chan_idx          Channel used to receive PDU with CTE
 *
 * In case of AoA mode ant_num and ant_ids parameters are not used.
 */
void lll_df_conf_cte_rx_enable(uint8_t slot_duration, uint8_t ant_num, uint8_t *ant_ids,
			       uint8_t chan_idx)
{
	struct node_rx_iq_report *node_rx;

	/* ToDo change to appropriate HCI constant */
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US)
	if (slot_duration == 0x1) {
		radio_df_cte_rx_2us_switching();
	} else
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_1US */
	{
		radio_df_cte_rx_4us_switching();
	}

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	radio_df_ant_switching_pin_sel_cfg();
	radio_df_ant_switch_pattern_clear();
	radio_df_ant_switch_pattern_set(ant_ids, ant_num);
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

	node_rx = ull_df_iq_report_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_df_iq_data_packet_set(node_rx->pdu, IQ_SAMPLE_TOTAL_CNT);
	node_rx->chan_idx = chan_idx;
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

/* @brief Function performs common steps for initialization and reset
 * of Direction Finding LLL module.
 *
 * @return Zero in case of success, other value in case of failure.
 */
static int init_reset(void)
{
	return 0;
}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* @brief Function initializes transmission of Constant Tone Extension.
 *
 * @param df_cfg    Pointer to direction finding configuration
 *
 * @Note Pay attention that first two antenna identifiers in a switch
 * pattern df_cfg->ant_ids has special purpose. First one is used in guard period,
 * second in reference period. Actual switching is processed from third antenna.
 *
 * In case of AoA mode df_ant_sw_len and df->ant_ids members are not used.
 */
static void df_cte_tx_configure(const struct lll_df_adv_cfg *df_cfg)
{
	if (df_cfg->cte_type == BT_HCI_LE_AOA_CTE) {
		radio_df_mode_set_aoa();
		radio_df_cte_tx_aoa_set(df_cfg->cte_length);
	}
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)
	else {
		radio_df_mode_set_aod();

		if (df_cfg->cte_type == BT_HCI_LE_AOD_CTE_1US) {
			radio_df_cte_tx_aod_2us_set(df_cfg->cte_length);
		} else {
			radio_df_cte_tx_aod_4us_set(df_cfg->cte_length);
		}

		radio_df_ant_switching_pin_sel_cfg();
		radio_df_ant_switch_pattern_clear();
		radio_df_ant_switch_pattern_set(df_cfg->ant_ids, df_cfg->ant_sw_len);
	}
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_TX */
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
