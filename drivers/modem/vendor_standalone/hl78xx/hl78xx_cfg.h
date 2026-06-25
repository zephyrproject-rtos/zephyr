/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hl78xx_cfg.h
 *
 * Helper APIs for RAT, band and APN configuration extracted from hl78xx.c
 * to keep the state machine file smaller and easier to read.
 */
#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CFG_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CFG_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include "hl78xx.h"

/**
 * @brief Enable LTE coverage URC.
 *
 * @param data Pointer to the modem data structure.
 * @param timeout_s Timeout in seconds for the LTE coverage URC.
 * @note The timeout_s parameter must be between 0 and 1200 seconds.
 * Duration to attempt to acquire updated cell measurements when waking
 * from Sleep/Lite Hibernate/Hibernate in PSM dormant or eDRX (in seconds).
 * · 0 — Disabled
 * · 1−1200 — Time to attempt to acquire updated cell measurements.
 * If attempt is successful, a +KCELLMEAS URC will be produced with <revision>=1 data.
 * For more details, see Signal Quality URC on Wakeup Application Note (Doc #2174298).
 * If attempt is unsuccessful, an empty +KCELLMEAS URC will be produced.
 * @return 0 on success, negative error code on failure.
 */
int hl78xx_enable_lte_coverage_urc(struct hl78xx_data *data, bool *modem_require_restart,
				   uint16_t timeout_s);

#ifdef CONFIG_MODEM_HL78XX_AUTORAT
int hl78xx_set_prl_internal(struct hl78xx_data *data, const struct kselacq_syntax kselacq_rats);
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

int hl78xx_rat_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		   enum hl78xx_cell_rat_mode *rat_request);

int hl78xx_band_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		    enum hl78xx_cell_rat_mode rat_config_request);

int hl78xx_gsm_pdp_activate(struct hl78xx_data *data);

#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
int hl78xx_rat_ntn_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		       enum hl78xx_cell_rat_mode rat_config_request);
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
int hl78xx_set_apn_internal(struct hl78xx_data *data, const char *apn, uint16_t size);

int hl78xx_get_uart_config(struct hl78xx_data *data);

/**
 * @brief Remove one surrounding pair of double quotes from a string in place.
 *
 * @param str Mutable null-terminated string buffer.
 */
void hl78xx_trim_surrounding_quotes(char *str);

/**
 * @brief Parse a +CTZEU unsolicited result code into a typed event payload.
 *
 * @param argv Tokenized URC arguments from modem chat.
 * @param argc Number of tokens in argv.
 * @param update Output structure populated on success.
 *
 * @return 0 on success, negative errno on parse failure.
 */
int hl78xx_ctzeu_parse_urc(char **argv, uint16_t argc, struct hl78xx_ctzeu_update *update);

/**
 * @brief Parse a +CEREG/+CREG response into cached registration details.
 *
 * @param data HL78XX data structure.
 * @param argv Tokenized URC arguments from modem chat.
 * @param argc Number of tokens in argv.
 * @param is_urc Indicates if the message is a URC.
 */
void hl78xx_parse_cereg_info(struct hl78xx_data *data, char **argv, uint16_t argc, bool is_urc);

/**
 * @brief Set network operator format.
 *
 * @param data Pointer to HL78xx data structure.
 * @param format Network operator format.
 * @return 0 on success, negative error code on failure.
 */
int hl78xx_set_network_operator_format(struct hl78xx_data *data,
				       enum hl78xx_operator_format format);

/**
 * @brief Parse a PLMN string into MCC and MNC.
 *
 * @param operator Null-terminated string containing the PLMN.
 * @param mcc Pointer to store the Mobile Country Code.
 * @param mnc Pointer to store the Mobile Network Code.
 * @return true if parsing was successful, false otherwise.
 */
bool hl78xx_parse_plmn(const char *operator, uint16_t *mcc, uint16_t *mnc);

/**
 * @brief Convert an active band hex bitmap string to a band number.
 *
 * Scans the hex bitmap string from least-significant nibble to most-significant,
 * finds the first set bit, and returns the corresponding band number (1-based).
 * Assumes only a single band bit is set, as reported by AT+KBND?.
 *
 * @param bmp Null-terminated hex bitmap string (e.g. "00020000000000000000").
 * @return Band number >= 1 on success, -ENODATA if bitmap is empty or all zeros.
 */
int hl78xx_band_bitmap_to_number(const char *bmp);

#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
/* Baud rate detection and switching */
int configure_uart_for_auto_baudrate(struct hl78xx_data *data, uint32_t baudrate);
int hl78xx_try_baudrate(struct hl78xx_data *data, uint32_t baudrate);
int hl78xx_detect_current_baudrate(struct hl78xx_data *data);
int hl78xx_switch_baudrate(struct hl78xx_data *data, uint32_t target_baudrate);
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */

/**
 * @brief Convert a binary bitmap to a trimmed hexadecimal string.
 *
 * Converts a bitmap into a hex string, removing leading zeros for a
 * compact representation. Useful for modem configuration commands.
 *
 * @param bitmap Pointer to the input binary bitmap.
 * @param hex_str Output buffer for the resulting hex string.
 * @param hex_str_len Size of the output buffer in bytes.
 */
void hl78xx_bitmap_to_hex_string_trimmed(const uint8_t *bitmap, char *hex_str, size_t hex_str_len);

/**
 * @brief Trim leading zeros from a hexadecimal string.
 *
 * Removes any '0' characters from the beginning of the provided hex string,
 * returning a pointer to the first non-zero character.
 *
 * @param hex_str Null-terminated hexadecimal string.
 *
 * @return Pointer to the first non-zero digit in the string,
 *         or the last zero if the string is all zeros.
 */
const char *hl78xx_trim_leading_zeros(const char *hex_str);

/**
 * @brief hl78xx_extract_essential_part_apn - Extract the essential part of the APN.
 * @param full_apn Full APN string.
 * @param essential_apn Buffer to store the essential part of the APN.
 * @param max_len Maximum length of the essential APN buffer.
 */
void hl78xx_extract_essential_part_apn(const char *full_apn, char *essential_apn, size_t max_len);

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
int hl78xx_enable_pmc(struct hl78xx_data *data);
int hl78xx_psm_settings(struct hl78xx_data *data);
int hl78xx_edrx_settings(struct hl78xx_data *data);
int hl78xx_power_down_settings(struct hl78xx_data *data);
void hl78xx_dynamic_cmd_feed_lpm_timers(struct hl78xx_data *data, uint16_t response_timeout);
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN

int hl78xx_init_power_down(struct hl78xx_data *data);
void hl78xx_power_down_ignore_feeding(struct hl78xx_data *data);
void hl78xx_power_down_allow_feeding(struct hl78xx_data *data);
bool hl78xx_power_down_is_ignoring_feeding(struct hl78xx_data *data);
int hl78xx_cancel_power_down(struct hl78xx_data *data);
int hl78xx_is_power_down_scheduled(struct hl78xx_data *data);
int hl78xx_power_down_feed_timer(struct hl78xx_data *data, uint32_t cmd_timeout_s);

#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX

void hl78xx_edrx_idle_init(struct hl78xx_data *data);
void hl78xx_edrx_idle_ignore_feeding(struct hl78xx_data *data);
void hl78xx_edrx_idle_allow_feeding(struct hl78xx_data *data);
bool hl78xx_edrx_idle_is_ignoring_feeding(struct hl78xx_data *data);
int hl78xx_edrx_idle_feed_timer(struct hl78xx_data *data, uint32_t cmd_timeout_s);
int hl78xx_is_edrx_idle_scheduled(struct hl78xx_data *data);
int hl78xx_edrx_idle_cancel(struct hl78xx_data *data);
bool hl78xx_edrx_idle_is_sleeping(struct hl78xx_data *data);
uint32_t hl78xx_edrx_idle_get_remaining_timetosleep(struct hl78xx_data *data);

#endif /* CONFIG_MODEM_HL78XX_EDRX */

#ifdef CONFIG_MODEM_HL78XX_PSM
void hl78xx_psmev_init(struct hl78xx_data *data);
#endif /* CONFIG_MODEM_HL78XX_PSM */

int binary_str_to_byte(const char *bin_str);
void byte_to_binary_str(uint8_t byte, char *output);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

bool hl78xx_is_rsrp_value_valid(int16_t rsrp);
bool hl78xx_is_rsrq_value_valid(int16_t rsrq);
bool hl78xx_is_sinr_value_valid(int16_t sinr);
bool hl78xx_is_rsrp_valid(struct hl78xx_data *data);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CFG_H_ */
