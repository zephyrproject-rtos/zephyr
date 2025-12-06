/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *****************************************************************************
 * hl78xx_chat.c
 *
 * Centralized translation unit for MODEM_CHAT_* macro-generated objects and
 * chat scripts for the HL78xx driver. This file contains the MODEM_CHAT
 * matches and script definitions and exposes runtime wrapper functions
 * declared in hl78xx_chat.h.
 *
 * Contract:
 *  - Other translation units MUST NOT take addresses of the MODEM_CHAT_*
 *    symbols or use ARRAY_SIZE() on them at file scope. Use the getters
 *    (hl78xx_get_*) and runners (hl78xx_ run_*_script[_async]) instead.
 *****************************************************************************
 */

#include "hl78xx.h"
#include "hl78xx_chat.h"
#include <zephyr/modem/chat.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(hl78xx_dev);

/* Forward declarations of handlers implemented in hl78xx.c (extern linkage) */
void hl78xx_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
/* +CGCONTRDP handler implemented in hl78xx_sockets.c - declared here so the
 * chat match may reference it. This handler parses PDP context response and
 * updates DNS / interface state for the driver instance.
 */
void hl78xx_on_cgdcontrdp(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
#if defined(CONFIG_MODEM_HL78XX_12)
void hl78xx_on_kstatev(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
void hl78xx_on_cgact(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */
#endif /* CONFIG_MODEM_HL78XX_12 */
void hl78xx_on_socknotifydata(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_ktcpnotif(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
/* Handler implemented to assign modem-provided udp socket ids */
void hl78xx_on_kudpsocket_create(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void hl78xx_on_ktcpsocket_create(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
/* Handler implemented to assign modem-provided tcp socket ids */
void hl78xx_on_ktcpind(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
/*
 * Chat script and URC match definitions - extracted from hl78xx.c
 */
void hl78xx_on_udprcv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_kbndcfg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_csq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_cfun(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_cops(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_ksup(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_imei(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_ksrep(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_ksrat(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_kselacq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void hl78xx_on_serial_number(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
void hl78xx_on_wdsi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
MODEM_CHAT_MATCH_DEFINE(hl78xx_ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(hl78xx_allow_match, MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH(CME_ERROR_STRING, "", NULL),
			  MODEM_CHAT_MATCH(ERROR_STRING, "", NULL));
/* clang-format off */
MODEM_CHAT_MATCHES_DEFINE(hl78xx_unsol_matches,
			  MODEM_CHAT_MATCH("+KSUP: ", "", hl78xx_on_ksup),
			  MODEM_CHAT_MATCH("+CREG: ", ",", hl78xx_on_cxreg),
			  MODEM_CHAT_MATCH("+CEREG: ", ",", hl78xx_on_cxreg),
#if defined(CONFIG_MODEM_HL78XX_12)
			  MODEM_CHAT_MATCH("+KSTATEV: ", ",", hl78xx_on_kstatev),
#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
			  MODEM_CHAT_MATCH("+CGACT: ", ",", hl78xx_on_cgact),
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */
#endif /* CONFIG_MODEM_HL78XX_12 */
			  MODEM_CHAT_MATCH("+KUDP_DATA: ", ",", hl78xx_on_socknotifydata),
			  MODEM_CHAT_MATCH("+KTCP_DATA: ", ",", hl78xx_on_socknotifydata),
			  MODEM_CHAT_MATCH("+KTCP_NOTIF: ", ",", hl78xx_on_ktcpnotif),
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
			  MODEM_CHAT_MATCH("+KUDP_RCV: ", ",", hl78xx_on_udprcv),
#endif
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
			  MODEM_CHAT_MATCH("+WDSI: ", ",", hl78xx_on_wdsi),
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
			  MODEM_CHAT_MATCH("+KBNDCFG: ", ",", hl78xx_on_kbndcfg),
			  MODEM_CHAT_MATCH("+CSQ: ", ",", hl78xx_on_csq),
			  MODEM_CHAT_MATCH("+CESQ: ", ",", hl78xx_on_cesq),
			  MODEM_CHAT_MATCH("+CFUN: ", "", hl78xx_on_cfun),
			  MODEM_CHAT_MATCH("+COPS: ", ",", hl78xx_on_cops));
/* clang-format on */
MODEM_CHAT_MATCHES_DEFINE(hl78xx_abort_matches, MODEM_CHAT_MATCH(CME_ERROR_STRING, "", NULL));
MODEM_CHAT_MATCH_DEFINE(hl78xx_at_ready_match, "+KSUP: ", "", hl78xx_on_ksup);
MODEM_CHAT_MATCH_DEFINE(hl78xx_imei_match, "", "", hl78xx_on_imei);
MODEM_CHAT_MATCH_DEFINE(hl78xx_cgmm_match, "", "", hl78xx_on_cgmm);
MODEM_CHAT_MATCH_DEFINE(hl78xx_cimi_match, "", "", hl78xx_on_imsi);
MODEM_CHAT_MATCH_DEFINE(hl78xx_cgmi_match, "", "", hl78xx_on_cgmi);
MODEM_CHAT_MATCH_DEFINE(hl78xx_cgmr_match, "", "", hl78xx_on_cgmr);
MODEM_CHAT_MATCH_DEFINE(hl78xx_serial_number_match, "+KGSN: ", "", hl78xx_on_serial_number);
MODEM_CHAT_MATCH_DEFINE(hl78xx_iccid_match, "+CCID: ", "", hl78xx_on_iccid);
MODEM_CHAT_MATCH_DEFINE(hl78xx_ksrep_match, "+KSREP: ", ",", hl78xx_on_ksrep);
MODEM_CHAT_MATCH_DEFINE(hl78xx_ksrat_match, "+KSRAT: ", "", hl78xx_on_ksrat);
MODEM_CHAT_MATCH_DEFINE(hl78xx_kselacq_match, "+KSELACQ: ", ",", hl78xx_on_kselacq);

/* Chat script matches / definitions */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", hl78xx_ok_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_periodic_chat_script, hl78xx_periodic_chat_script_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 4);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KHWIOCFG=3,1,6", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4,0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSLEEP=2", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS=0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEDRXS=0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KPATTERN=\"--EOF--Pattern--\"",
							 hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KGSN=3", hl78xx_serial_number_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CCID", hl78xx_iccid_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", hl78xx_imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", hl78xx_cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", hl78xx_cgmi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", hl78xx_cgmr_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", hl78xx_cimi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_ok_match),
#if defined(CONFIG_MODEM_HL78XX_12)
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSTATEV=1", hl78xx_ok_match),
#endif
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGEREP=2", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSELACQ?", hl78xx_kselacq_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSRAT?", hl78xx_ksrat_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KBNDCFG?", hl78xx_ok_match),
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
#ifdef CONFIG_MODEM_HL78XX_12
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSI=8191", hl78xx_ok_match),
#elif defined(CONFIG_MODEM_HL78XX_00)
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSI=4479", hl78xx_ok_match),
#else
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSI=?", hl78xx_ok_match),
#endif /* CONFIG_MODEM_HL78XX_12 */
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_CONNECT_AIRVANTAGE
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSC=0,1", hl78xx_ok_match),
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_CONNECT_AIRVANTAGE */
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_DOWNLOAD_FIRMWARE
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSC=1,1", hl78xx_ok_match),
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_DOWNLOAD_FIRMWARE */
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_INSTALL_FIRMWARE
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSC=2,1", hl78xx_ok_match),
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_INSTALL_FIRMWARE */
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGACT?", hl78xx_ok_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_init_chat_script, hl78xx_init_chat_script_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 100);

/* Post-restart script (moved from hl78xx.c) */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_post_restart_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("", hl78xx_at_ready_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSRAT?", hl78xx_ksrat_match),
#if defined(CONFIG_MODEM_HL78XX_12)
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSTATEV=1", hl78xx_ok_match)
#endif
);

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_post_restart_chat_script, hl78xx_post_restart_chat_script_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 12);

/* init_fail_script moved from hl78xx.c */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(init_fail_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", hl78xx_ksrep_match));

MODEM_CHAT_SCRIPT_DEFINE(init_fail_script, init_fail_script_cmds, hl78xx_abort_matches,
			 hl78xx_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_enable_ksup_urc_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP=1", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", hl78xx_ksrep_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_enable_ksup_urc_script, hl78xx_enable_ksup_urc_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 4);

/* power-off script moved from hl78xx.c */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_pwroff_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPWROFF", hl78xx_ok_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_pwroff_script, hl78xx_pwroff_cmds, hl78xx_abort_matches,
			 hl78xx_chat_callback_handler, 4);

/* GSM registration status disable / LTE registration status enable script */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_gsm_dis_lte_en_reg_status_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=5", hl78xx_ok_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_gsm_dis_lte_en_reg_status_script,
			 hl78xx_gsm_dis_lte_en_reg_status_script_cmds, hl78xx_abort_matches, NULL,
			 4);

#if defined(CONFIG_MODEM_HL78XX_12) &&                                                             \
	(defined(CONFIG_MODEM_HL78XX_RAT_GSM) || defined(CONFIG_MODEM_HL78XX_AUTORAT))
/* LTE registration status disable / GSM registration status enable script */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_lte_dis_gsm_en_reg_status_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=0", hl78xx_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=3", hl78xx_ok_match));
MODEM_CHAT_SCRIPT_DEFINE(hl78xx_lte_dis_gsm_en_reg_status_script,
			 hl78xx_lte_dis_gsm_en_reg_status_script_cmds, hl78xx_abort_matches, NULL,
			 4);
#endif /* CONFIG_MODEM_HL78XX_12 */
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
/* AirVantage script connect accept  */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_av_connect_accept_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSR=1", hl78xx_ok_match));
MODEM_CHAT_SCRIPT_DEFINE(hl78xx_av_connect_accept_script, hl78xx_av_connect_accept_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 10);
/* FOTA script download accept */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_fota_download_accept_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSR=3", hl78xx_ok_match));
MODEM_CHAT_SCRIPT_DEFINE(hl78xx_fota_download_accept_script, hl78xx_fota_download_accept_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 10);
/* FOTA script install */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_fota_install_accept_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+WDSR=4", hl78xx_ok_match));
MODEM_CHAT_SCRIPT_DEFINE(hl78xx_fota_install_accept_script, hl78xx_fota_install_accept_cmds,
			 hl78xx_abort_matches, hl78xx_chat_callback_handler, 10);
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
/* Socket-specific matches and wrappers exposed for the sockets translation
 * unit. These were extracted from hl78xx_sockets.c to centralize chat
 * definitions.
 */
MODEM_CHAT_MATCHES_DEFINE(connect_matches, MODEM_CHAT_MATCH(CONNECT_STRING, "", NULL),
			  MODEM_CHAT_MATCH(CME_ERROR_STRING, "", NULL),
			  MODEM_CHAT_MATCH(ERROR_STRING, "", NULL));
MODEM_CHAT_MATCHES_DEFINE(kudpind_allow_match,
			  MODEM_CHAT_MATCH("+KUDP_IND: ", ",", hl78xx_on_kudpsocket_create),
			  MODEM_CHAT_MATCH(CME_ERROR_STRING, "", NULL),
			  MODEM_CHAT_MATCH(ERROR_STRING, "", NULL));
MODEM_CHAT_MATCH_DEFINE(ktcpind_match, "+KTCP_IND: ", ",", hl78xx_on_ktcpind);
MODEM_CHAT_MATCH_DEFINE(ktcpcfg_match, "+KTCPCFG: ", "", hl78xx_on_ktcpsocket_create);
MODEM_CHAT_MATCH_DEFINE(cgdcontrdp_match, "+CGCONTRDP: ", ",", hl78xx_on_cgdcontrdp);
MODEM_CHAT_MATCH_DEFINE(ktcp_state_match, "+KTCPSTAT: ", ",", NULL);

const struct modem_chat_match *hl78xx_get_connect_matches(void)
{
	return connect_matches;
}

size_t hl78xx_get_connect_matches_size(void)
{
	return (size_t)ARRAY_SIZE(connect_matches);
}

const struct modem_chat_match *hl78xx_get_sockets_allow_matches(void)
{
	return hl78xx_allow_match;
}

size_t hl78xx_get_sockets_allow_matches_size(void)
{
	return (size_t)ARRAY_SIZE(hl78xx_allow_match);
}

const struct modem_chat_match *hl78xx_get_kudpind_match(void)
{
	return kudpind_allow_match;
}

size_t hl78xx_get_kudpind_allow_matches_size(void)
{
	return (size_t)ARRAY_SIZE(kudpind_allow_match);
}

const struct modem_chat_match *hl78xx_get_ktcpind_match(void)
{
	return &ktcpind_match;
}

const struct modem_chat_match *hl78xx_get_ktcpcfg_match(void)
{
	return &ktcpcfg_match;
}

const struct modem_chat_match *hl78xx_get_cgdcontrdp_match(void)
{
	return &cgdcontrdp_match;
}

const struct modem_chat_match *hl78xx_get_ktcp_state_match(void)
{
	return &ktcp_state_match;
}

/* modem_init_chat is implemented in hl78xx.c so it can construct the
 * modem_chat_config with device-local buffer sizes (argv_size) without
 * relying on ARRAY_SIZE at file scope inside this translation unit.
 */

/* Bridge function - modem_chat callback */
void hl78xx_chat_callback_handler(struct modem_chat *chat, enum modem_chat_script_result result,
				  void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
	} else if (result == MODEM_CHAT_SCRIPT_RESULT_TIMEOUT) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_AT_CMD_TIMEOUT);
	} else {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_FAILED);
	}
}

/* --- Wrapper helpers -------------------------------------------------- */
const struct modem_chat_match *hl78xx_get_ok_match(void)
{
	return &hl78xx_ok_match;
}

size_t hl78xx_get_ok_match_size(void)
{
	return 1;
}

const struct modem_chat_match *hl78xx_get_abort_matches(void)
{
	return hl78xx_abort_matches;
}

const struct modem_chat_match *hl78xx_get_unsol_matches(void)
{
	return hl78xx_unsol_matches;
}

size_t hl78xx_get_unsol_matches_size(void)
{
	/* Return size as a runtime value to avoid constant-expression errors
	 * in translation units that include this header.
	 */
	return (size_t)(ARRAY_SIZE(hl78xx_unsol_matches));
}

size_t hl78xx_get_abort_matches_size(void)
{
	return (size_t)(ARRAY_SIZE(hl78xx_abort_matches));
}

const struct modem_chat_match *hl78xx_get_allow_match(void)
{
	return hl78xx_allow_match;
}

size_t hl78xx_get_allow_match_size(void)
{
	return (size_t)(ARRAY_SIZE(hl78xx_allow_match));
}

/* Run the predefined init script for the given device */
int hl78xx_run_init_script(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script(&data->chat, &hl78xx_init_chat_script);
}

/* Run the periodic script */
int hl78xx_run_periodic_script(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script(&data->chat, &hl78xx_periodic_chat_script);
}

int hl78xx_run_init_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_init_chat_script);
}

int hl78xx_run_periodic_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_periodic_chat_script);
}

const struct modem_chat_match *hl78xx_get_ksrat_match(void)
{
	return &hl78xx_ksrat_match;
}

int hl78xx_run_post_restart_script(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script(&data->chat, &hl78xx_post_restart_chat_script);
}

int hl78xx_run_post_restart_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_post_restart_chat_script);
}

int hl78xx_run_init_fail_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &init_fail_script);
}

int hl78xx_run_enable_ksup_urc_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_enable_ksup_urc_script);
}

int hl78xx_run_pwroff_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_pwroff_script);
}
#if defined(CONFIG_MODEM_HL78XX_12) &&                                                             \
	(defined(CONFIG_MODEM_HL78XX_RAT_GSM) || defined(CONFIG_MODEM_HL78XX_AUTORAT))
/* Run the LTE disable GSM enable registration status script */
int hl78xx_run_lte_dis_gsm_en_reg_status_script(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script(&data->chat, &hl78xx_lte_dis_gsm_en_reg_status_script);
}
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */
/* Run the GSM disable LTE enable registration status script */
int hl78xx_run_gsm_dis_lte_en_reg_status_script(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script(&data->chat, &hl78xx_gsm_dis_lte_en_reg_status_script);
}
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
/* AirVantage script connect accept  */
int hl78xx_run_av_connect_accept_script_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_av_connect_accept_script);
}
/* FOTA script download accept */
int hl78xx_run_fota_script_download_accept_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_fota_download_accept_script);
}
/* FOTA script install accept */
int hl78xx_run_fota_script_install_accept_async(struct hl78xx_data *data)
{
	if (!data) {
		return -EINVAL;
	}
	return modem_chat_run_script_async(&data->chat, &hl78xx_fota_install_accept_script);
}
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
