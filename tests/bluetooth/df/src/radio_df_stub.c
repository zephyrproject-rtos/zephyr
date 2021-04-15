/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

/* Stub functitons that does nothing. Just to avoid liker complains. */
void radio_df_ant_configure(void)
{
}

uint8_t radio_df_ant_num_get(void)
{
	return 0;
}

void radio_df_cte_inline_set(uint8_t enable)
{
}

void radio_df_cte_length_set(uint8_t value)
{
}

void radio_df_ant_switch_pattern_clear(void)
{
}

void radio_df_ant_switch_pattern_set(uint8_t pattern)
{
}

void radio_df_reset(void)
{
}

void radio_switch_complete_and_phy_end_disable(void)
{
}

void radio_df_cte_tx_aod_2us_set(uint8_t cte_len)
{
}

void radio_df_cte_tx_aod_4us_set(uint8_t cte_len)
{
}

void radio_df_cte_tx_aoa_set(uint8_t cte_len)
{
}

void radio_df_mode_set_aoa(void)
{
}

void radio_df_mode_set_aod(void)
{
}

void radio_df_ant_switching_pin_sel_cfg(void)
{
}

void radio_df_ant_switching_gpios_cfg(void)
{
}
