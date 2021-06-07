/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <bluetooth/hci.h>

#include "util/util.h"
#include "util/memq.h"

#include "hal/cpu.h"
#include "hal/radio_df.h"

#include "pdu.h"

#include "lll.h"
#include "lll_df.h"
#include "lll_df_types.h"
#include "lll_df_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
#define LOG_MODULE_NAME bt_ctlr_lll_df
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* @brief Function performs common steps for initialization and reset
 * of Direction Finding LLL module.
 *
 * @return	Zero in case of success, other value in case of failure.
 */
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

/* @brief Function initializes transmission of Constant Tone Extension.
 *
 * @param[in] type      Type of CTE: AoA, AoD 1us, AoD 2us
 * @param[in] length    Duration of CTE in 8us units
 * @param[in] ant_num   Number of antennas in switch pattern
 * @param[in] ant_ids   Antenna identifiers that create switch pattern.
 *
 * @Note Pay attention that first two antenna identifiers in a switch
 * pattern has special purpose. First one is used in guard period, second
 * in reference period. Actual switching is processed from third antenna.
 *
 * In case of AoA mode ant_num and ant_ids parameters are not used.
 */
void lll_df_conf_cte_tx_enable(uint8_t type, uint8_t length,
			       uint8_t ant_num, uint8_t *ant_ids)
{
	if (type == BT_HCI_LE_AOA_CTE) {
		radio_df_mode_set_aoa();
		radio_df_cte_tx_aoa_set(length);
	}
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) || \
	defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	else {
		radio_df_mode_set_aod();

		if (type == BT_HCI_LE_AOD_CTE_1US) {
			radio_df_cte_tx_aod_2us_set(length);
		} else {
			radio_df_cte_tx_aod_4us_set(length);
		}

		radio_df_ant_switching_pin_sel_cfg();
		radio_df_ant_switch_pattern_clear();
		radio_df_ant_switch_pattern_set(ant_ids, ant_num);
	}
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_TX || CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */
}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
struct lll_df_sync_cfg *lll_df_sync_cfg_alloc(struct lll_df_sync *df_cfg,
					      uint8_t *idx)
{
	uint8_t first, last;

	first = df_cfg->first;
	last = df_cfg->last;
	if (first == last) {
		last++;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0U;
		}
	} else {
		uint8_t first_latest;

		df_cfg->last = first;
		cpu_dmb();
		first_latest = df_cfg->first;
		if (first_latest != first) {
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

void lll_df_conf_cte_tx_disable(void)
{
	radio_df_reset();
}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
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

static int init_reset(void)
{
	return 0;
}
