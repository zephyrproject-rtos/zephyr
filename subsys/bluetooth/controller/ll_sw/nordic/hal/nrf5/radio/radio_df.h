/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Function configures Radio with information about GPIO pins that may be
 * used to drive antenna switching during CTE Tx/RX.
 */
void radio_df_ant_switching_pin_sel_cfg(void);
/* Configures GPIO pins in GPIO peripheral. The pins will be used for antenna
 * switching during CTE Rx/Tx.
 */
void radio_df_ant_switching_gpios_cfg(void);
/* Provides number of available antennas for Direction Finding. */
uint8_t radio_df_ant_num_get(void);

/* Sets Direction Finding AOA mode. */
void radio_df_mode_set_aoa(void);
/* Sets Direction Finding AOD mode. */
void radio_df_mode_set_aod(void);
/* Sets inline configuration enabled or disabled for receive of CTE. */
void radio_df_cte_inline_set(uint8_t eanble);
/* Sets length of the CTE for transmission. */
void radio_df_cte_length_set(uint8_t value);
/* Clears antenna switch pattern. */
void radio_df_ant_switch_pattern_clear(void);
/* Set antenna switch spacing to 2[us] */
void radio_df_ant_switch_spacing_set_2us(void);
/* Set antenna switch spacing to 4[us] */
void radio_df_ant_switch_spacing_set_4us(void);
/* Set antenna switch pattern. Pay attention, paterns are added to
 * Radio internal list. Before start of new patterns clear the list
 * by call to @ref radio_df_ant_switch_pattern_clear.
 */
void radio_df_ant_switch_pattern_set(uint8_t *patterns, uint8_t len);
/* Resets Direction Finding radio configuration */
void radio_df_reset(void);

/* Completes switching and enables shortcut between PHYEND and DISABLE events */
void radio_switch_complete_and_phy_end_disable(void);
