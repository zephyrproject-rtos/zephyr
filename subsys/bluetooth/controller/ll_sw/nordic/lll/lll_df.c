/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <bluetooth/hci.h>

#include "hal/radio_df.h"
#include "lll_df.h"

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
	}
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) || \
	defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
	else {
		radio_df_mode_set_aod();

		if (type == BT_HCI_LE_AOD_CTE_1US) {
			radio_df_ant_switch_spacing_set_2us();
		} else {
			radio_df_ant_switch_spacing_set_4us();
		}

		radio_df_ant_switching_pin_sel_cfg();
		radio_df_ant_switch_pattern_clear();
		radio_df_ant_switch_pattern_set(ant_ids, ant_num);
	}
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_TX || CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

	radio_df_cte_length_set(length);
}

void lll_df_conf_cte_tx_disable(void)
{
	radio_df_reset();
}

static int init_reset(void)
{
	return 0;
}
