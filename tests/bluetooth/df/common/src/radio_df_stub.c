/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stddef.h>

/* global variables that may be used to imitate real radio behaviour */
static uint32_t g_iq_samples_amount;
static uint8_t g_cte_status;
static uint8_t g_pdu_switch_pattern;

/* Stub functions that does nothing. Just to avoid liker complains. */

uint8_t radio_df_ant_num_get(void)
{
	return 0;
}

void radio_df_cte_inline_set(uint8_t enable)
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

void radio_df_ant_switching_gpios_cfg(void)
{
}

void radio_df_cte_tx_aoa_set(uint8_t cte_len)
{
}

void radio_df_ant_switching_pin_sel_cfg(void)
{
}

void radio_df_mode_set_aoa(void)
{
}

void radio_df_mode_set_aod(void)
{
}

/* CTE RX related functions */
void radio_df_cte_rx_2us_switching(void)
{
}

void radio_df_cte_rx_4us_switching(void)
{
}

void radio_df_iq_data_packet_set(uint8_t *buffer, size_t len)
{
}

void ut_radio_df_iq_samples_amount_set(uint32_t amount)
{
	g_iq_samples_amount = amount;
}

uint32_t radio_df_iq_samples_amount_get(void)
{
	return g_iq_samples_amount;
}

void ut_radio_df_cte_status_set(uint8_t status)
{
	g_cte_status = status;
}

uint8_t radio_df_cte_status_get(void)
{
	return g_cte_status;
}

void ut_radio_df_pdu_antenna_switch_pattern_set(uint8_t pattern)
{
	g_pdu_switch_pattern = pattern;
}

uint8_t radio_df_pdu_antenna_switch_pattern_get(void)
{
	return g_pdu_switch_pattern;
}
