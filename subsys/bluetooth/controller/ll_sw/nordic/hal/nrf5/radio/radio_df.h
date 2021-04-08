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

/* Configure CTE transmission with 2us antenna switching for AoD. */
void radio_df_cte_tx_aod_2us_set(uint8_t cte_len);
/* Configure CTE transmission with 4us antenna switching for AoD. */
void radio_df_cte_tx_aod_4us_set(uint8_t cte_len);
/* Configure CTE transmission with for AoA. */
void radio_df_cte_tx_aoa_set(uint8_t cte_len);
/* Configure CTE reception with optionall AoA mode and 2us antenna switching. */
void radio_df_cte_rx_2us_switching(void);
/* Configure CTE reception with optionall AoA mode and 4us antenna switching. */
void radio_df_cte_rx_4us_switching(void);

/* Clears antenna switch pattern. */
void radio_df_ant_switch_pattern_clear(void);
/* Set antenna switch pattern. Pay attention, paterns are added to
 * Radio internal list. Before start of new patterns clear the list
 * by call to @ref radio_df_ant_switch_pattern_clear.
 */
void radio_df_ant_switch_pattern_set(uint8_t *patterns, uint8_t len);
/* Provides switch pattern of antenna used to transmit PDU that is used to
 * transmit CTE
 */
uint8_t radio_df_pdu_antenna_switch_pattern_get(void);
/* Resets Direction Finding radio configuration */
void radio_df_reset(void);

/* Completes switching and enables shortcut between PHYEND and DISABLE events */
void radio_switch_complete_and_phy_end_disable(void);

/* Enables CTE inline configuration to automatically setup sampling and
 * switching according to CTEInfo in received PDU.
 */
void radio_df_cte_inline_set_enabled(bool cte_info_in_s1);
/* Set buffer to store IQ samples collected during CTE sampling */
void radio_df_iq_data_packet_set(uint8_t *buffer, size_t len);
/* Get number of stored IQ samples during CTE receive */
uint32_t radio_df_iq_samples_amount_get(void);
/* Get CTE status (CTEInfo) parsed by Radio from received PDU */
uint8_t radio_df_cte_status_get(void);
