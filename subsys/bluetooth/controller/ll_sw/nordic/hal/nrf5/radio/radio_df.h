/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* PDU_ANTENNA is defined outside of the #if block below because
 * radio_df_pdu_antenna_switch_pattern_get() can get called even when
 * the preprocessor condition being tested is 0. In this case, we use
 * the default value of 0.
 */
#define PDU_ANTENNA DT_PROP_OR(RADIO_NODE, dfe_pdu_antenna, 0)

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

/* Configure CTE transmission with 2us antenna switching for AoD. */
void radio_df_cte_tx_aod_2us_set(uint8_t cte_len);
/* Configure CTE transmission with 4us antenna switching for AoD. */
void radio_df_cte_tx_aod_4us_set(uint8_t cte_len);
/* Configure CTE transmission with for AoA. */
void radio_df_cte_tx_aoa_set(uint8_t cte_len);
/* Configure CTE reception with optional AoA mode and 2us antenna switching. */
void radio_df_cte_rx_2us_switching(bool cte_info_in_s1, uint8_t phy);
/* Configure CTE reception with optional AoA mode and 4us antenna switching. */
void radio_df_cte_rx_4us_switching(bool cte_info_in_s1, uint8_t phy);

/* Clears antenna switch pattern. */
void radio_df_ant_switch_pattern_clear(void);
/* Set antenna switch pattern. Pay attention, patterns are added to
 * Radio internal list. Before start of new patterns clear the list
 * by call to @ref radio_df_ant_switch_pattern_clear.
 */
void radio_df_ant_switch_pattern_set(const uint8_t *patterns, uint8_t len);
/* Provides switch pattern of antenna used to transmit PDU that is used to
 * transmit CTE
 */
uint8_t radio_df_pdu_antenna_switch_pattern_get(void);
/* Resets Direction Finding radio configuration */
void radio_df_reset(void);

/* Completes switching and enables shortcut between PHYEND and TXEN events */
void radio_switch_complete_and_phy_end_b2b_tx(uint8_t phy_curr, uint8_t flags_curr,
					      uint8_t phy_next, uint8_t flags_next);

/* Set buffer to store IQ samples collected during CTE sampling */
void radio_df_iq_data_packet_set(uint8_t *buffer, size_t len);
/* Get number of stored IQ samples during CTE receive */
uint32_t radio_df_iq_samples_amount_get(void);
/* Get CTE status (CTEInfo) parsed by Radio from received PDU */
uint8_t radio_df_cte_status_get(void);
/* Get information if CTE was present in a received packet */
bool radio_df_cte_ready(void);
