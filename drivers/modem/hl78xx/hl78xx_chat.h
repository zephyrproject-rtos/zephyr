/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hl78xx_chat.h
 *
 * Wrapper accessors for MODEM_CHAT_* objects that live in a dedicated
 * translation unit (hl78xx_chat.c). Other driver TUs should only call
 * these functions instead of taking addresses or using sizeof/ARRAY_SIZE
 * on the macro-generated objects.
 */
#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CHAT_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CHAT_H_

#include <stddef.h>
#include <zephyr/modem/chat.h>

/**
 * @defgroup hl78xx_cmd_timeouts AT Command Timeout Values
 * @brief Recommended timeouts from HL78xx AT Command Reference Guide (Table A-1)
 *
 * These timeout values are grouped by command categories:
 * - FAST (2s): Basic queries, echo, flow control, simple settings
 * - SHORT (5s): SIM access (CSIM, CCHO, CCHC, CRSM), KSREP, CGDCONT, KNWSCANCFG
 * - MEDIUM (10s): KSRAT
 * - LONG (30s): CFUN, NVM writes, COPN, KCELL, SMS operations, KTCPCNX, KCARRIERCFG
 * - VERY_LONG (60s): CPIN, CGATT, CGACT, CGCMOD, socket send/receive/close
 * - EXTENDED (120s): COPS, CPOF, CPWROFF, CFUN for NB-IoT NTN
 * @{
 */

/** 2 seconds - Basic AT commands, queries, simple settings */
#define HL78XX_CMD_TIMEOUT_FAST      2
/** 5 seconds - SIM access, KSREP, CGDCONT, CEDRXRDP */
#define HL78XX_CMD_TIMEOUT_SHORT     5
/** 10 seconds - KSRAT */
#define HL78XX_CMD_TIMEOUT_MEDIUM    10
/** 30 seconds - CFUN, NVM writes, network info, SMS, TCP connect */
#define HL78XX_CMD_TIMEOUT_LONG      30
/** 60 seconds - PDP operations, CPIN, socket data transfer */
#define HL78XX_CMD_TIMEOUT_VERY_LONG 60
/** 120 seconds - COPS, power off, NB-IoT NTN operations */
#define HL78XX_CMD_TIMEOUT_EXTENDED  120

/** @} */

/**
 * @defgroup hl78xx_script_timeouts Chat Script Timeout Groups
 * @brief Composite timeouts for multi-command chat scripts
 *
 * When a script contains multiple commands with different timeouts,
 * use the maximum timeout from that command group plus margin.
 * @{
 */

/** Init script timeout - contains CFUN, NVM commands */
#define HL78XX_SCRIPT_TIMEOUT_INIT         100
/** Post-restart script timeout - waiting for +KSUP */
#define HL78XX_SCRIPT_TIMEOUT_POST_RESTART 12
/** Periodic script timeout - basic queries */
#define HL78XX_SCRIPT_TIMEOUT_PERIODIC     4
/** Network registration script timeout */
#define HL78XX_SCRIPT_TIMEOUT_NETWORK      10
/** Power off script timeout - CPWROFF */
#define HL78XX_SCRIPT_TIMEOUT_POWEROFF     HL78XX_CMD_TIMEOUT_EXTENDED
/** GNSS script timeout */
#define HL78XX_SCRIPT_TIMEOUT_GNSS         10
/** Socket operations default timeout */
#define HL78XX_SCRIPT_TIMEOUT_SOCKET       HL78XX_CMD_TIMEOUT_VERY_LONG

/** @} */

/* Forward declare driver data type to keep this header lightweight and avoid
 * circular includes. The implementation file (hl78xx_chat.c) includes
 * hl78xx.h for full driver visibility.
 */
struct hl78xx_data;

/* Chat callback bridge used by driver TUs to receive script results. */
void hl78xx_chat_callback_handler(struct modem_chat *chat, enum modem_chat_script_result result,
				  void *user_data);

/* Wrapper helpers so other translation units don't need compile-time
 * visibility of the MODEM_CHAT_* macro-generated symbols.
 */
const struct modem_chat_match *hl78xx_get_at_ready_match(void);
const struct modem_chat_match *hl78xx_get_ok_match(void);
size_t hl78xx_get_ok_match_size(void);
const struct modem_chat_match *hl78xx_get_abort_matches(void);
const struct modem_chat_match *hl78xx_get_unsol_matches(void);
size_t hl78xx_get_unsol_matches_size(void);
size_t hl78xx_get_abort_matches_size(void);
const struct modem_chat_match *hl78xx_get_allow_match(void);
size_t hl78xx_get_allow_match_size(void);

/* Run predefined scripts from other units */
int hl78xx_run_init_script(struct hl78xx_data *data);
int hl78xx_run_periodic_script(struct hl78xx_data *data);
int hl78xx_run_post_restart_script(struct hl78xx_data *data);
int hl78xx_run_init_fail_script_async(struct hl78xx_data *data);
int hl78xx_run_enable_ksup_urc_script_async(struct hl78xx_data *data);
int hl78xx_run_pwroff_script_async(struct hl78xx_data *data);
int hl78xx_run_post_restart_script_async(struct hl78xx_data *data);
#ifdef CONFIG_MODEM_HL78XX_12
#if defined(CONFIG_MODEM_HL78XX_RAT_GSM) ||                     \
	defined(CONFIG_MODEM_HL78XX_AUTORAT)
/* Run the LTE disable GSM enable registration status script */
int hl78xx_run_lte_dis_gsm_en_reg_status_script(struct hl78xx_data *data);
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */
#ifdef CONFIG_NTN_POSITION_SOURCE_MANUAL
/* Run the NTN position setting script */
int hl78xx_run_ntn_pos_script_async(struct hl78xx_data *data);
#endif /* CONFIG_NTN_POSITION_SOURCE_MANUAL */
#endif /* CONFIG_MODEM_HL78XX_12 */
/* Run the GSM disable LTE enable registration status script */
int hl78xx_run_gsm_dis_lte_en_reg_status_script(struct hl78xx_data *data);
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
/* FOTA-related script runners */
int hl78xx_run_av_connect_accept_script_async(struct hl78xx_data *data);
int hl78xx_run_fota_script_download_accept_async(struct hl78xx_data *data);
int hl78xx_run_fota_script_install_accept_async(struct hl78xx_data *data);
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
/* Async runners for init/periodic scripts */
int hl78xx_run_init_script_async(struct hl78xx_data *data);
int hl78xx_run_periodic_script_async(struct hl78xx_data *data);
int hl78xx_run_cfun_query_script_async(struct hl78xx_data *data);
#ifdef CONFIG_HL78XX_GNSS
int hl78xx_run_gnss_init_chat_script_async(struct hl78xx_data *data);
int hl78xx_run_gnss_stop_search_chat_script(struct hl78xx_data *data);
int hl78xx_run_gnss_terminate_nmea_chat_script(struct hl78xx_data *data);
int hl78xx_run_gnss_gnssloc_script(struct hl78xx_data *data);
#endif /* CONFIG_HL78XX_GNSS */
/* Getter for ksrat match (moved into chat TU) */
const struct modem_chat_match *hl78xx_get_ksrat_match(void);

/* Socket-related chat matches used by the sockets TU */
const struct modem_chat_match *hl78xx_get_connect_matches(void);
size_t hl78xx_get_connect_matches_size(void);
const struct modem_chat_match *hl78xx_get_sockets_allow_matches(void);
size_t hl78xx_get_sockets_allow_matches_size(void);
const struct modem_chat_match *hl78xx_get_kudpind_match(void);
size_t hl78xx_get_kudpind_allow_matches_size(void);
const struct modem_chat_match *hl78xx_get_ktcpind_match(void);
const struct modem_chat_match *hl78xx_get_ktcpcfg_match(void);
const struct modem_chat_match *hl78xx_get_cgdcontrdp_match(void);
const struct modem_chat_match *hl78xx_get_ktcp_state_match(void);
#ifdef CONFIG_HL78XX_GNSS
/* GNSS-related chat matches used by the GNSS TU */
const struct modem_chat_match *hl78xx_get_gnssnmea_match(void);
const struct modem_chat_match *hl78xx_get_gnssconf_enabledsys_match(void);
const struct modem_chat_match *hl78xx_get_gnssconf_enabledfilter_match(void);
#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
/* A-GNSS assistance data match */
const struct modem_chat_match *hl78xx_get_gnssad_match(void);
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */

#endif /* CONFIG_HL78XX_GNSS */

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CHAT_H_ */
