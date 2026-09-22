/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

/*
 * Notes:
 * - Two devices are instantiated per DT node, both sharing the same bc66x_data
 * and bc66x_config structs. This is necessary because NET_DEVICE_DT_INST_OFFLOAD_DEFINE
 * hard-wires dev->api to offloaded_if_api, leaving no room for cellular_driver_api.
 * The control device (DEVICE_DT_INST_DEFINE) owns PM and exposes cellular_driver_api.
 * The net device (NET_DEVICE_DT_INST_OFFLOAD_DEFINE) owns the network interface and
 * socket offloading. The control device must initialise first (lower priority value).
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for strtok_r() */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <stdlib.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/atomic.h>

#include <errno.h>
#include <string.h>

#include "bc66x_hw.h"
#include "bc66x.h"
#include "bc66x_net.h"

LOG_MODULE_REGISTER(bc66x, CONFIG_MODEM_QUECTEL_BC66X_LOG_LEVEL);

/* Data for the network offload device */
static struct bc66x_data *g_bc66x_offload_data;

/* These parsers set values in bc66x_data) */

/* Modem info handlers */
static void on_modem_error(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc == 0) {
		LOG_ERR("Modem script error: Unspecified error");
		return;
	}

	/* argv[0] is always the base match string (e.g., "+CME ERROR" or "ERROR") */
	LOG_ERR("Modem script error. Trigger: %s", argv[0]);

	if (argc > 1) {
		LOG_ERR("Error details/code: %s", argv[1]);
	}

	/* Dump any additional arguments if the modem sent a complex response */
	for (uint16_t i = 2; i < argc; i++) {
		LOG_ERR("Additional error info (arg %u): %s", i, argv[i]);
	}
}

static void on_cgsn_imei(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	/* Response format: +CGSN: 86... */
	strncpy(data->imei, argv[1], sizeof(data->imei) - 1);
	data->imei[sizeof(data->imei) - 1] = '\0';
}

static void on_cgsn_sn(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	/* Response format: MPY2... */
	strncpy(data->serial, argv[1], sizeof(data->serial) - 1);
	data->serial[sizeof(data->serial) - 1] = '\0';
}

static void on_cgsn_imeisv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	/* Response format: +CGSN: 86... */
	strncpy(data->imeisv, argv[1], sizeof(data->imeisv) - 1);
	data->imeisv[sizeof(data->imeisv) - 1] = '\0';
}

static void on_cgsn_svn(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	/* Response format: +CGSN: 3 */
	strncpy(data->svn, argv[1], sizeof(data->svn) - 1);
	data->svn[sizeof(data->svn) - 1] = '\0';
}

static void on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	/* Response format: Revision: BC660KGLAAR01A03 */
	strncpy(data->revision, argv[1], sizeof(data->revision) - 1);
	data->revision[sizeof(data->revision) - 1] = '\0';
}

static void on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	ARG_UNUSED(chat);

	if (argc != 3U) {
		return;
	}

	/* Response format: Quectel_BC660K-GL */
	strncpy(data->manufacturer, argv[0], sizeof(data->manufacturer) - 1);
	data->manufacturer[sizeof(data->manufacturer) - 1] = '\0';
	strncpy(data->model, argv[2], sizeof(data->model) - 1);
	data->model[sizeof(data->model) - 1] = '\0';
}

/* SIM info handlers */
static void on_cimi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;
	/* Response format: 460001357924680 */
	strncpy(data->connection_info.imsi, argv[1], sizeof(data->connection_info.imsi) - 1);
	data->connection_info.imsi[sizeof(data->connection_info.imsi) - 1] = '\0';
}

static void on_qccid(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;
	/* Response format: +QCCID: 89860446091891372008 */
	strncpy(data->connection_info.iccid, argv[1], sizeof(data->connection_info.iccid) - 1);
	data->connection_info.iccid[sizeof(data->connection_info.iccid) - 1] = '\0';
}

/* Signal info handlers */
static void on_cesq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct bc66x_data *data = user_data;

	ARG_UNUSED(chat);

	if (argc != 7U) {
		return;
	}

	/* Ignored data: rssi (obtained with CSQ) ber, rscp, ecno, rxlev
	 * Response format: +CESQ: <rxlev>,<ber>,<rscp>,<ecno>,<rsrq>,<rsrp>
	 * Note from specs:
	 * <relev>,  <ber>, <rscp> and <ecno>  are not applicable to NB-IoT network and should
	 * be set to "not known or not detectable" (99) for the module.
	 * Temporarly ignored.
	 */

	int raw_rsrq = atoi(argv[5]);
	int raw_rsrp = atoi(argv[6]);

	/* RSRQ */
	if (raw_rsrq == BC66X_RSRQ_UNKNOWN_VALUE) {
		data->connection_info.rsrq_x10 = BC66X_UNKNOWN_VALUE;
	} else {
		/*
		 * From specs:
		 * 0 --> <rsrq> < -19.5 dB
		 * 1 --> -19.5 dB <= <rsrq> < -19 dB
		 * ...
		 * 32 --> -4 dB <= <rsrq> < -3.5 dB
		 * ...
		 * 34 --> -3 dB <= <rsrq>
		 * Storing lower bound in .rsrq (1 --> -190 --> -19 dB)
		 */

		data->connection_info.rsrq_x10 = (raw_rsrq * BC66X_RSRQ_SCALE) + BC66X_RSRQ_OFFSET;
	}

	/* RSRP */
	if (raw_rsrp == BC66X_RSRP_UNKNOWN_VALUE) {
		data->connection_info.rsrp = BC66X_UNKNOWN_VALUE;
	} else {
		/*
		 * From specs:
		 * 0 --> <rsrp> < -140 dBm
		 * 1 --> -140 dBm <= <rsrp> < -139 dBm
		 * ...
		 * 95 --> -46 dBm <= <rsrp> < -45 dBm
		 * ...
		 * 97 --> -44 dBm <= <rsrp>
		 * Storing lower bound in .rsrp (1 --> -140 dB)
		 */
		data->connection_info.rsrp = BC66X_RSRP_OFFSET + raw_rsrp;
	}
}

static void on_csq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	ARG_UNUSED(chat);

	if (argc != 3U) {
		return;
	}

	struct bc66x_data *data = user_data;
	int raw_rssi;

	/* Response format: +CSQ: <rssi>,<ber>
	 * <ber> ignored
	 */
	raw_rssi = atoi(argv[1]);

	if (raw_rssi == BC66X_RSSI_UNKNOWN_VALUE) {
		data->connection_info.rssi = BC66X_RSSI_UNKNOWN_VALUE;
	} else {
		/*
		 * From specs:
		 * 0 --> -113 dBm or less
		 * 1 --> -111 dBm
		 * 2–30 --> -109 to -53 dBm
		 * 31 --> -51 dBm or greater
		 */
		data->connection_info.rssi = BC66X_RSSI_SCALE * raw_rssi + BC66X_RSSI_OFFSET;
	}
}

static void cereg_core(struct bc66x_data *data, int stat, int act)
{
	bool change_state = true;
	bool connected_state = false;

	if (atomic_get(&data->modem_state) == BC66X_SLEEPING_STATE) {
		/* This can happen since there is a delay between
		 * enabling and entering sleep modes.
		 */
		LOG_DBG("CEREG received while being in sleep state. The state will not be "
			"touched.");
		change_state = false;
	}

	switch (stat) {
	/* Modem is considered connected in these two cases */
	case BC66X_EPS_REG_STATUS_REGISTERED:
		data->connection_info.registration_status = CELLULAR_REGISTRATION_REGISTERED_HOME;
		connected_state = true;
		break;
	case BC66X_EPS_REG_STATUS_REGISTERED_ROAMING:
		data->connection_info.registration_status =
			CELLULAR_REGISTRATION_REGISTERED_ROAMING;
		connected_state = true;
		break;

	/* In either of these cases modem is considered not connected */
	case BC66X_EPS_REG_STATUS_NOT_REGISTERED:
		data->connection_info.registration_status = CELLULAR_REGISTRATION_NOT_REGISTERED;
		connected_state = false;
		break;
	case BC66X_EPS_REG_STATUS_NOT_REGISTERED_SEARCHING:
		data->connection_info.registration_status = CELLULAR_REGISTRATION_SEARCHING;
		connected_state = false;
		break;
	case BC66X_EPS_REG_STATUS_REGISTRATION_DENIED:
		data->connection_info.registration_status = CELLULAR_REGISTRATION_DENIED;
		connected_state = false;
		break;

	case BC66X_EPS_REG_STATUS_UNKNOWN:
	default:
		data->connection_info.registration_status = CELLULAR_REGISTRATION_UNKNOWN;
		connected_state = false;
		break;
	}

	if (connected_state) {
		if (change_state) {
			atomic_set(&data->modem_state, BC66X_READY_STATE);
		}

		/* Both calls together make net_if_is_up() return true */
		net_if_carrier_on(data->net.iface);
		net_if_up(data->net.iface);

		LOG_INF("Modem registered to network");
	} else {
		if (change_state) {
			atomic_set(&data->modem_state, BC66X_CONNECTING_STATE);
		}

		net_if_carrier_off(data->net.iface);
		net_if_down(data->net.iface);

		LOG_INF("Modem not connected (stat=%d)", stat);
	}

	switch (act) {
	case BC66X_ACCESS_TECHNOLOGY_E_UTRAN:
		LOG_DBG("Access technology: E-UTRAN");
		data->connection_info.access_technology = CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN;
		break;
	case BC66X_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1:
		LOG_DBG("Access technology: E-UTRAN (NB-S1 mode)");
		data->connection_info.access_technology = CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1;
		break;
	default:
		/* BC660K supports only E-UTRAN-NB-S1, BC66 E-UTRAN too. */
		LOG_WRN("Unknown AcT received from CEREG: %d", act);
		data->connection_info.access_technology = CELLULAR_ACCESS_TECHNOLOGY_UNKNOWN;
	}
}

static void on_cereg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	LOG_DBG("CEREG response");

	struct bc66x_data *data = user_data;
	int stat = -1;
	int act = -1;

	/* Response format: +CEREG: <n>, <stat> [,[<tac>],[<ci>],[<AcT>]] */
	stat = atoi(argv[2]);

	/* "<AcT>, <tac> and <ci> are provided only if available." */
	if (argc >= 6) {
		act = atoi(argv[5]);
	}

	cereg_core(data, stat, act);
}

static void on_urc_cereg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	LOG_DBG("URC CEREG received");
	struct bc66x_data *data = user_data;
	int stat = -1;
	int act = -1;

	/* Response format: +CEREG: <stat> [,[<tac>],[<ci>],[<AcT>]] */
	stat = atoi(argv[1]);

	if (argc >= 5) {
		act = atoi(argv[4]);
	}

	cereg_core(data, stat, act);
}

static void on_urc_ip(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	LOG_DBG("IP URC received");

	struct bc66x_data *data = user_data;

	/* Response format: +IP: xxx.xxx.xxx.xxx */

	strncpy(data->connection_info.ip_addr, argv[1], sizeof(data->connection_info.ip_addr) - 1);
	data->connection_info.ip_addr[sizeof(data->connection_info.ip_addr) - 1] = '\0';

	/* Remove old addr (if any) */
	net_if_ipv4_addr_rm(data->net.iface, &data->net.ipv4_addr);

	struct net_in_addr new_addr;

	net_addr_pton(NET_AF_INET, argv[1], &new_addr);

	if (!net_if_ipv4_addr_add(data->net.iface, &new_addr, NET_ADDR_MANUAL, 0)) {
		LOG_WRN("Failed to set IPv4 address");
		return;
	}

	net_ipaddr_copy(&data->net.ipv4_addr, &new_addr);
}

/* Sleep handlers */
static void on_urc_enter_deepsleep(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct bc66x_data *data = user_data;

	LOG_DBG("Received ENTER DEEPSLEEP");

	atomic_set(&data->is_in_deep_sleep, 1);
}

static void on_urc_exit_deepsleep(struct modem_chat *chat, char **argv, uint16_t argc,
				  void *user_data)
{
	struct bc66x_data *data = user_data;

	LOG_DBG("Received EXIT DEEPSLEEP");

	atomic_set(&data->is_in_deep_sleep, 0);
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCH_DEFINE(discard_match, "", "", NULL);

MODEM_CHAT_MATCHES_DEFINE(ok_and_discard, ok_match, discard_match);

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", on_modem_error),
			  MODEM_CHAT_MATCH("+CME ERROR", "", on_modem_error),
			  MODEM_CHAT_MATCH("+CMS ERROR", "", on_modem_error));

/* URCs */
MODEM_CHAT_MATCHES_DEFINE(
	unsol_matches, MODEM_CHAT_MATCH("SEND OK", "", on_send_ok),
	MODEM_CHAT_MATCH("SEND FAIL", "", on_send_fail),
	MODEM_CHAT_MATCH("CLOSE OK", "", on_close_ok),
	MODEM_CHAT_MATCH("+QIOPEN: ", ",", on_urc_qiopen),
	MODEM_CHAT_MATCH("+QIURC: \"recv\",", ",", on_urc_recv),
	MODEM_CHAT_MATCH("+QIURC: \"closed\",", ",", on_urc_closed),
	MODEM_CHAT_MATCH("+CEREG: ", ",", on_urc_cereg), MODEM_CHAT_MATCH("+IP: ", "", on_urc_ip),
	MODEM_CHAT_MATCH("+QNBIOTEVENT: \"ENTER DEEPSLEEP\"", "", on_urc_enter_deepsleep),
	MODEM_CHAT_MATCH("+QNBIOTEVENT: \"EXIT DEEPSLEEP\"", "", on_urc_exit_deepsleep),
	/* NOTE: +QIDNSGIP: is treated as URC (even if it should be a response) */
	MODEM_CHAT_MATCH("+QIDNSGIP: ", ",", on_qidnsgip),
	/* BC66 Only*/
	MODEM_CHAT_MATCH("+QIURC: \"dnsgip\",", ",", on_urc_dnsgip),
	MODEM_CHAT_MATCH("+QPING: ", ",", on_urc_qiping));

/* Modem Info */
MODEM_CHAT_MATCH_DEFINE(cgsn_sn_match, "", "", on_cgsn_sn);
MODEM_CHAT_MATCH_DEFINE(cgmr_match, "Revision: ", "", on_cgmr);
MODEM_CHAT_MATCH_DEFINE(cgmm_match, "Quectel", "_", on_cgmm);

/* SIM info */
MODEM_CHAT_MATCH_DEFINE(cgsn_imei_match, "+CGSN: ", "", on_cgsn_imei);
MODEM_CHAT_MATCH_DEFINE(cgsn_imeisv_match, "+CGSN: ", "", on_cgsn_imeisv);
MODEM_CHAT_MATCH_DEFINE(cgsn_svn_match, "+CGSN: ", "", on_cgsn_svn);
MODEM_CHAT_MATCH_DEFINE(cimi_match, "", "", on_cimi);
MODEM_CHAT_MATCH_DEFINE(qccid_match, "+QCCID: ", "", on_qccid);

/* Signal info */
MODEM_CHAT_MATCH_DEFINE(cesq_match, "+CESQ: ", ",", on_cesq);
MODEM_CHAT_MATCH_DEFINE(csq_match, "+CSQ: ", ",", on_csq);
MODEM_CHAT_MATCH_DEFINE(cereg_match, "+CEREG: ", ",", on_cereg);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	bc66x_init_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP(
		"AT", discard_match), /* Ignore everything before this (discard_match) */

	MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("ATE0", ok_and_discard),
	/* Disable sleep modes (handled by this driver) */
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+QSCLK=0", ok_match),

	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", ok_match),
	/* "requires" cfun=0 before and =1 after */
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCSEARFCN", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
	/* TODO: Make this customizable */
	/* Radio on for 2 seconds, then go to sleep. Check in with the network at least once every
	 * 320 hours.
	 */
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS=1,,,\"11000001\",\"00000001\"", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_init_script, bc66x_init_script_cmds, abort_matches, NULL,
			 BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* Script to check if the modem is ready */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_sync_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATI", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_sync_script, bc66x_sync_cmds, abort_matches, NULL,
			 BC66X_CHAT_SCRIPTS_SHORT_TIMEOUT_S);

/* Additional SETUP commands  */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc660k_setup_cmds,
			      /* Enable URC for network registration */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=2", ok_match),
			      /* Set TCP/IP dataformat to hex */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QICFG=\"dataformat\",1,1", ok_match));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66_setup_cmds,
			      /* Enable URC for network registration */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=2", ok_match),
			      /* Set TCP/IP dataformat to hex */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QICFG=\"dataformat\",1,1", ok_match),
			      /* Data output format: data header,data */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QICFG=\"viewmode\",1", ok_match));

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(bc660k_setup_script, bc660k_setup_cmds, NULL,
				  BC66X_CHAT_SCRIPTS_TIMEOUT_S);
MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(bc66_setup_script, bc66_setup_cmds, NULL,
				  BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* MODEM INFO */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_get_modem_info_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN=0", cgsn_sn_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_get_modem_info_script, bc66x_get_modem_info_cmds, abort_matches,
			 NULL, BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* SIM INFO */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_get_sim_info_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN=1", cgsn_imei_match),

			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN=2", cgsn_imeisv_match),

			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN=3", cgsn_svn_match),

			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),

			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),

			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCCID", qccid_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_get_sim_info_script, bc66x_get_sim_info_cmds, abort_matches, NULL,
			 BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* GET SIGNAL */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_get_signal_info_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CESQ", cesq_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_get_signal_info_script, bc66x_get_signal_info_cmds, abort_matches,
			 NULL, BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* PM */
/* Note AT+QCFG="slplocktimes" is untouched (default value: 10s) */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc660k_enable_light_sleep_mode_cmds,
			      /* Enable only light sleep mode */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QSCLK=2", ok_match),
			      /* No application is expected to exchange data. */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CNMPSD", ok_match));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66_enable_light_sleep_mode_cmds,
			      /* Enable only light sleep mode */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QSCLK=2", ok_match));

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(bc660k_enable_light_sleep_script,
				  bc660k_enable_light_sleep_mode_cmds, NULL,
				  BC66X_CHAT_SCRIPTS_TIMEOUT_S);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(bc66_enable_light_sleep_script, bc66_enable_light_sleep_mode_cmds,
				  NULL, BC66X_CHAT_SCRIPTS_TIMEOUT_S);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc660k_enable_deep_sleep_mode_cmds,
			      /* No application is expected to exchange data. */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CNMPSD", ok_match),
			      /* Enable both light and deep sleep mode */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QSCLK=1", ok_match));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66_enable_deep_sleep_mode_cmds,
			      /* Enable both light and deep sleep mode */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QSCLK=1", ok_match));

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(bc660k_enable_deep_sleep_script,
				  bc660k_enable_deep_sleep_mode_cmds, NULL,
				  BC66X_CHAT_SCRIPTS_TIMEOUT_S);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(bc66_enable_deep_sleep_script, bc66_enable_deep_sleep_mode_cmds,
				  NULL, BC66X_CHAT_SCRIPTS_TIMEOUT_S);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_wake_up_cmds,
			      /* This should wake up the modem (for BC660K only), no response
			       * expected. After this, wait timeout before the next command(s)
			       */
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT",
							      BC66X_PM_WAKE_UP_COMMAND_TIMEOUT_MS),
			      /* Check if the moded is awake */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
			      /* Disable sleep modes */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QSCLK=0", ok_match),
			      /* Update connection status */
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", cereg_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_wake_up_script, bc66x_wake_up_cmds, abort_matches, NULL,
			 BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* REGISTRATION (Assumption: CEREG with n=2, otherwise AT+COPS can be used for AcT) */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_get_registration_status_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", cereg_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_get_registration_status_script, bc66x_get_registration_status_cmds,
			 abort_matches, NULL, BC66X_CHAT_SCRIPTS_TIMEOUT_S);

/* (power) RESET MODEM */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(bc66x_reset_cmd, MODEM_CHAT_SCRIPT_CMD_RESP("AT+QRST=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(bc66x_reset_script, bc66x_reset_cmd, abort_matches, NULL,
			 BC66X_CHAT_SCRIPTS_TIMEOUT_S);

static inline bool bc66x_pm_is_active(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	if (pm_device_state_get(dev, &state) == 0) {
		return state == PM_DEVICE_STATE_ACTIVE;
	}
#endif

	return true; /* PM disabled or state fetch failed: assume active */
}

static void bc66x_identify_apn_profile(struct bc66x_data *data)
{
	if (data->connection_info.iccid[0] == '\0') {
		LOG_WRN("Cannot identify operator, void iccid");
		return;
	}

	/* Add some space to be sure */
	char buffer[sizeof(CONFIG_MODEM_QUECTEL_BC66X_APN_PROFILES) + 8];
	char *saveptr;

	strncpy(buffer, CONFIG_MODEM_QUECTEL_BC66X_APN_PROFILES, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';

	char *token = strtok_r(buffer, ",", &saveptr);

	while (token != NULL) {
		char *equal_sign = strchr(token, '=');

		if (equal_sign != NULL) {
			*equal_sign = '\0';
			char *p_apn = token;
			char *associated_number_prefix = equal_sign + 1;

			/* Trim leading whitespace */
			while (*p_apn == ' ') {
				p_apn++;
			}
			while (*associated_number_prefix == ' ') {
				associated_number_prefix++;
			}

			size_t prefix_len = strlen(associated_number_prefix);

			if (strlen(data->connection_info.iccid) >= prefix_len &&
			    strncmp(data->connection_info.iccid, associated_number_prefix,
				    prefix_len) == 0) {
				/* Success */
				strncpy(data->connection_info.operator.name, p_apn,
					sizeof(data->connection_info.operator.name) - 1);
				data->connection_info.operator
					.name[sizeof(data->connection_info.operator.name) - 1] =
					'\0';
				return;
			}
		}
		token = strtok_r(NULL, ",", &saveptr);
	}

	/* Not found */
	data->connection_info.operator.name[0] = '\0';
}

/* Used during INIT, SUSPENDING and RESUMING (without PM get/put) */
static int bc66x_run_script_core(struct bc66x_data *data, const struct modem_chat_script *script)
{
	int ret;

	k_sem_take(&data->script_lock, K_FOREVER);

	ret = modem_chat_run_script(&data->chat, script);
	if (ret < 0) {
		LOG_WRN("Script %s error: %d", script->name, ret);
	}

	k_sem_give(&data->script_lock);
	return ret;
}

static int bc66x_run_script(struct bc66x_data *data, const struct modem_chat_script *script)
{
	int ret;

	if (!atomic_get(&data->is_initialized)) {
		LOG_WRN("Trying to running script before modem initialized");
		return -EAGAIN;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = pm_device_runtime_get(data->dev);
	if (ret < 0) {
		return -ENODEV;
	}
#else
	pm_device_busy_set(data->dev);
	if (!bc66x_pm_is_active(data->dev)) {
		pm_device_busy_clear(data->dev);
		return -ENODEV;
	}
#endif

	ret = bc66x_run_script_core(data, script);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_put_async(data->dev, BC66X_PM_SUSPEND_GRACE_PERIOD);
#else
	pm_device_busy_clear(data->dev);
#endif

	return ret;
}

int bc66x_run_cmd_matches(struct bc66x_data *data, const char *cmd, uint32_t script_timeout_s,
			  const struct modem_chat_match *response_matches,
			  uint16_t response_matches_size,
			  const struct modem_chat_match *s_abort_matches,
			  uint16_t abort_matches_size)
{
	const struct modem_chat_script_chat script_chats[] = {
		{
			.request = (const uint8_t *)cmd,
			.request_size = (uint16_t)strlen(cmd),
			.response_matches = response_matches,
			.response_matches_size = response_matches_size,
			.timeout = script_timeout_s * MSEC_PER_SEC,
		},
	};

	const struct modem_chat_script script = {
		.name = "bc660k-cmd",
		.script_chats = script_chats,
		.script_chats_size = 1,
		.abort_matches = s_abort_matches,
		.abort_matches_size = abort_matches_size,
		.callback = NULL,
		.timeout = script_timeout_s,
	};

	int ret = bc66x_run_script(data, &script);

	return ret;
}

int bc66x_run_cmd(struct bc66x_data *data, const char *cmd, uint32_t script_timeout_s)
{
	return bc66x_run_cmd_matches(data, cmd, script_timeout_s, &ok_match, 1, abort_matches,
				     ARRAY_SIZE(abort_matches));
}

static int bc66x_copy_cached_info(const char *src, char *dst, size_t size)
{
	if ((dst == NULL) || (size == 0U)) {
		return -EINVAL;
	}

	if ((src == NULL) || (src[0] == '\0')) {
		dst[0] = '\0';
		return -ENODATA;
	}

	strncpy(dst, src, size - 1);
	dst[size - 1] = '\0';

	return 0;
}

static int bc66x_update_modem_info(struct bc66x_data *data)
{
	LOG_DBG("Updating modem info...");
	int ret;

	ret = bc66x_run_script(data, &bc66x_get_modem_info_script);
	if (ret < 0) {
		LOG_WRN("Cannot update modem info: %d", ret);
	}
	return ret;
}

static int bc66x_update_sim_info(struct bc66x_data *data)
{
	LOG_DBG("Updating SIM info...");
	int ret;

	ret = bc66x_run_script(data, &bc66x_get_sim_info_script);
	if (ret < 0) {
		LOG_WRN("Cannot update SIM info: %d", ret);
	}
	return ret;
}

static int bc66x_update_signal_info(struct bc66x_data *data)
{
	LOG_DBG("Updating signal info...");
	int ret;

	ret = bc66x_run_script(data, &bc66x_get_signal_info_script);
	if (ret < 0) {
		LOG_WRN("Cannot update signal info: %d", ret);
	}
	return ret;
}

static int bc66x_update_registration_status(struct bc66x_data *data)
{
	LOG_DBG("Updating registration status...");
	int ret;

	ret = bc66x_run_script(data, &bc66x_get_registration_status_script);
	if (ret < 0) {
		LOG_WRN("Cannot update registration status: %d", ret);
	}
	return ret;
}

static void bc66x_set_apn_work_handler(struct k_work *work)
{
	struct bc66x_data *data = CONTAINER_OF(work, struct bc66x_data, set_apn_work.work);

	size_t new_apn_len = strlen(data->new_apn_str);
	/* Add extra space to contain AT+QCGDEFCONT...*/
	char apn_cmd_buf[new_apn_len + 32];
	int ret;

	ret = snprintk(apn_cmd_buf, sizeof(apn_cmd_buf), "AT+QCGDEFCONT=\"IP\",\"%s\"",
		       data->new_apn_str);
	if (ret < 0 || ret >= sizeof(apn_cmd_buf)) {
		LOG_ERR("Command string error");
		return;
	}

	LOG_DBG("Set default apn command: %s", apn_cmd_buf);

	ret = bc66x_run_cmd(data, apn_cmd_buf, BC66X_SCRIPT_TIMEOUT_S);
	if (ret < 0) {
		LOG_ERR("Cannot update APN, error: %d", ret);
	} else {
		LOG_INF("New APN set correctly. Restart needed.");
		if (BC66X_FORCE_RESTART_AFTER_APN_SET) {
			LOG_INF("Restarting modem");
			bc66x_run_script(data, &bc66x_reset_script);
		}
	}

	atomic_set(&data->is_setting_apn, 0);
}

static int bc66x_transport_init(const struct device *dev)
{
	struct bc66x_data *data = dev->data;
	const struct bc66x_config *cfg = dev->config;

	const struct modem_backend_uart_config uart_cfg = {
		.uart = cfg->uart_dev,
		.receive_buf = data->uart_rx_buf,
		.receive_buf_size = sizeof(data->uart_rx_buf),
		.transmit_buf = data->uart_tx_buf,
		.transmit_buf_size = sizeof(data->uart_tx_buf),
	};

	const struct modem_chat_config chat_cfg = {
		.user_data = data,
		.receive_buf = data->chat_recv_buf,
		.receive_buf_size = sizeof(data->chat_recv_buf),
		.delimiter = "\r",
		.delimiter_size = 1,
		.filter = "\n",
		.filter_size = 1,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	int ret;

	if (atomic_get(&data->is_backend_ready)) {
		return 0;
	}

	data->pipe = modem_backend_uart_init(&data->uart_backend, &uart_cfg);
	if (data->pipe == NULL) {
		LOG_ERR("modem_backend_uart_init failed");
		return -ENODEV;
	}

	ret = modem_chat_init(&data->chat, &chat_cfg);
	if (ret < 0) {
		LOG_ERR("modem_chat_init failed: %d", ret);
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->pipe);
	if (ret < 0) {
		LOG_ERR("modem_chat_attach failed: %d", ret);
		return ret;
	}

	ret = modem_pipe_open(data->pipe, BC66X_INIT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("modem_pipe_open failed: %d", ret);
		return ret;
	}

	atomic_set(&data->is_backend_ready, 1);
	return 0;
}

static int bc66x_do_init(const struct device *dev)
{
	struct bc66x_data *data = dev->data;
	const struct bc66x_config *cfg = dev->config;
	int ret;

	ret = bc66x_transport_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* Is the modem, somehow, already ON? */

	ret = bc66x_run_script_core(data, &bc66x_sync_script);
	if (ret < 0) {
		/* No, execute power on sequence */
		LOG_DBG("Modem was OFF");

		if (cfg->is_bc66) {
			ret = bc66_hw_start(cfg);
		} else {
			ret = bc660k_hw_start(cfg);
		}
		if (ret < 0) {
			LOG_ERR("Error while starting modem: %d", ret);
			return ret;
		}

		k_sleep(BC66X_WAIT_FOR_HW_DELAY);

		ret = bc66x_run_script_core(data, &bc66x_sync_script);
		if (ret < 0) {
			LOG_ERR("Modem did not boot, error: %d", ret);
			return ret;
		}

	} else {
		LOG_DBG("Modem already ON");
	}

	LOG_DBG("Running init script");
	ret = bc66x_run_script_core(data, &bc66x_init_script);
	if (ret < 0) {
		LOG_ERR("Init script failed: %d", ret);
		return ret;
	}

	return 0;
}

static void bc66x_init_work_handler(struct k_work *work)
{
	/* NOTE PM runtime get and put not used here! (intended) */
	struct bc66x_data *data = CONTAINER_OF(work, struct bc66x_data, init_work.work);
	const struct bc66x_config *cfg = data->dev->config;
	int ret;

	if (atomic_get(&data->is_initialized)) {
		return;
	}

	LOG_DBG("Init work started");

	k_sem_take(&data->init_lock, K_FOREVER);
	atomic_set(&data->modem_state, BC66X_INIT_STATE);
	memset(&data->connection_info, 0, sizeof(data->connection_info));

	ret = bc66x_do_init(data->dev);
	if (ret == 0) {
		atomic_set(&data->is_initialized, 1);
		atomic_set(&data->modem_state, BC66X_CONNECTING_STATE);

		LOG_DBG("BC660K driver initialized");

		/* ADDITIONAL SETUP COMMANDS */
		/* TODO: add retry on fail */
		ret = bc66x_run_script_core(data, cfg->setup_script);
		if (ret < 0) {
			LOG_ERR("Error while running additional setup commands: %d", ret);
		} else {
			LOG_DBG("Additional setup commands executed");
		}

		bc66x_update_modem_info(data);
		bc66x_update_sim_info(data);
		bc66x_update_signal_info(data);
	} else {
		if (data->init_retries < CONFIG_BC66X_INIT_RETRIES) {
			LOG_WRN("BC66X deferred init failed: %d , retrying... (%d / %d)", ret,
				data->init_retries + 1, CONFIG_BC66X_INIT_RETRIES);
			data->init_retries++;
			k_work_schedule_for_queue(&data->bc66x_workq, &data->init_work,
						  BC66X_INIT_DELAY);
		} else {
			LOG_ERR("BC66X deferred init failed: %d . Max retries (%d) reached", ret,
				data->init_retries);
		}
	}
	k_sem_give(&data->init_lock);
	LOG_DBG("End of init work");
}

static int bc66x_get_modem_info(const struct device *dev, const enum cellular_modem_info_type type,
				char *info, size_t size)
{
	struct bc66x_data *data = dev->data;
	int ret;

	/* Init check inside run_script */

	ret = bc66x_update_modem_info(data);
	if (ret < 0) {
		LOG_WRN("Cannot update modem info");
	}

	ret = bc66x_update_sim_info(data);
	if (ret < 0) {
		LOG_WRN("Cannot update SIM info");
	}

	bc66x_identify_apn_profile(data);

	switch (type) {
	case CELLULAR_MODEM_INFO_IMEI:
		ret = bc66x_copy_cached_info(data->imei, info, size);
		break;
	case CELLULAR_MODEM_INFO_MODEL_ID:
		ret = bc66x_copy_cached_info(data->model, info, size);
		break;
	case CELLULAR_MODEM_INFO_MANUFACTURER:
		ret = bc66x_copy_cached_info(data->manufacturer, info, size);
		break;
	case CELLULAR_MODEM_INFO_FW_VERSION:
		ret = bc66x_copy_cached_info(data->revision, info, size);
		break;
	case CELLULAR_MODEM_INFO_SIM_IMSI:
		ret = bc66x_copy_cached_info(data->connection_info.imsi, info, size);
		break;
	case CELLULAR_MODEM_INFO_SIM_ICCID:
		ret = bc66x_copy_cached_info(data->connection_info.iccid, info, size);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int bc66x_get_signal(const struct device *dev, const enum cellular_signal_type type,
			    int16_t *value)
{

	struct bc66x_data *data = dev->data;

	/* init check (also) inside run_script */
	if (!atomic_get(&data->is_initialized)) {
		return -ENODATA;
	}

	int ret;

	ret = bc66x_update_signal_info(data);
	if (ret < 0) {
		LOG_WRN("Cannot update signal info");
	}

	switch (type) {
	case CELLULAR_SIGNAL_RSSI:
		*value = data->connection_info.rssi;
		ret = 0;
		break;

	case CELLULAR_SIGNAL_RSRP:
		*value = data->connection_info.rsrp;
		ret = 0;
		break;

	case CELLULAR_SIGNAL_RSRQ:
		*value = data->connection_info.rsrq_x10 / 10;
		ret = 0;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

static int bc66x_get_registration_status(const struct device *dev,
					 enum cellular_access_technology tech,
					 enum cellular_registration_status *status)
{

	const struct bc66x_config *cfg = dev->config;
	struct bc66x_data *data = dev->data;
	int ret;

	ret = bc66x_update_registration_status(data);
	if (ret < 0) {
		LOG_WRN("Cannot update registration info");
	}

	if (tech == CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1 ||
	    (cfg->is_bc66 && tech == CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN)) {

		if (data->connection_info.access_technology == tech) {
			*status = data->connection_info.registration_status;
		} else {
			*status = CELLULAR_REGISTRATION_NOT_REGISTERED;
		}
		return 0;

	} else {
		return -ENOTSUP;
	}
}

static int bc66x_set_apn(const struct device *dev, const char *new_apn)
{
	struct bc66x_data *data = dev->data;

	LOG_DBG("Set APN requested...");

	if (!atomic_get(&data->is_initialized)) {
		return -EAGAIN;
	}

	size_t new_apn_len = strnlen(new_apn, BC66X_MAX_APN_LENGTH);

	if (new_apn_len >= BC66X_MAX_APN_LENGTH) {
		LOG_ERR("APN string must be less than %d bytes", BC66X_MAX_APN_LENGTH);
		return -EINVAL;
	}

	if (new_apn_len == 0) {
		LOG_ERR("Invalid APN string (length 0)");
		return -EINVAL;
	}

	if (atomic_cas(&data->is_setting_apn, 0, 1)) {
		memcpy(data->new_apn_str, new_apn, new_apn_len);
		data->new_apn_str[new_apn_len] = '\0';
		k_work_schedule_for_queue(&data->bc66x_workq, &data->set_apn_work, K_NO_WAIT);
		LOG_DBG("Set APN work scheduled");
	} else {
		LOG_WRN("Another APN is still to be set");
		return -EAGAIN;
	}

	return 0;
}

/* TODO: Implement every function */
DEVICE_API(cellular, bc66x_cellular_api) = {
	.get_modem_info = bc66x_get_modem_info,
	.get_signal = bc66x_get_signal,
	.get_registration_status = bc66x_get_registration_status,
	.set_apn = bc66x_set_apn,
};

static struct offloaded_if_api bc66x_net_api_funcs = {
	.iface_api.init = bc66x_iface_init,
};

static int bc66x_wake_up(struct bc66x_data *data)
{
	LOG_DBG("Waking up...");

	atomic_cas(&data->modem_state, BC66X_SLEEPING_STATE, BC66X_AWAKENING_STATE);

	int ret = -1;
	int retries = 0;

	k_work_cancel_delayable(&data->deep_sleep_work);

	do {
		retries++;

		/* Wake up with PSM EINT (free to set +QCFG: "wakeupRXD",0) */
		/* To exit if light or deep sleep */
		ret = bc66x_hw_psm_pulse_ms(data->dev->config, 100); /* TODO make this a define? */
		if (ret < 0) {
			LOG_ERR("Error while pulsing psm pin: %d", ret);
		}

		ret = bc66x_run_script_core(data, &bc66x_wake_up_script);
		if (ret < 0) {
			LOG_WRN("Error while waking up with commands, try %d / %d", retries,
				BC66X_PM_WAKE_UP_MAX_RETRIES);
		}

	} while (ret < 0 && retries < BC66X_PM_WAKE_UP_MAX_RETRIES);

	if (ret < 0) {
		LOG_ERR("Cannot wake up modem");
		return ret;
	}

	/* NOTE: modem state updated with script handling (CEREG response) */

	atomic_set(&data->is_in_deep_sleep, 0); /* Also inside the "EXIT DEEPSLEEP" URC handling */
	LOG_DBG("Modem is now awake");
	return 0;
}

static int bc66x_enter_light_sleep(struct bc66x_data *data)
{
	LOG_DBG("Enabling light sleep...");

	const struct bc66x_config *cfg = data->dev->config;
	int ret;

	/* TODO: Add another grace period? */
	ret = bc66x_run_script_core(data, cfg->enable_light_sleep_script);
	if (ret < 0) {
		LOG_WRN("Error while enabling light sleep mode");
		/* Still assuming sleeping state to avoid losing commands... */
	}

	atomic_set(&data->modem_state, BC66X_SLEEPING_STATE);

	k_work_schedule_for_queue(&data->bc66x_workq, &data->deep_sleep_work,
				  BC66X_PM_DEEP_SLEEP_MODE_DELAY);

	LOG_DBG("Light sleep mode enabled, deep sleep work scheduled");

	return ret;
}

static void bc66x_enter_deep_sleep_work_handler(struct k_work *work)
{
	LOG_DBG("Enabling deep sleep...");

	struct bc66x_data *data = CONTAINER_OF(work, struct bc66x_data, deep_sleep_work.work);
	const struct bc66x_config *cfg = data->dev->config;
	int ret;

	if (atomic_get(&data->is_in_deep_sleep)) {
		LOG_WRN("Asked to enter deep sleep while already in deep sleep...");
		return;
	}
	/* Wake up and enable deep sleep  */

	bc66x_wake_up(data); /* Ignoring result ... */

	ret = bc66x_run_script_core(data, cfg->enable_deep_sleep_script);
	if (ret < 0) {
		LOG_WRN("Error while enabling deep sleep mode");
		/* Still assuming sleeping state to avoid losing commands... */
	}

	atomic_set(&data->modem_state, BC66X_SLEEPING_STATE);
	LOG_DBG("Deep sleep mode enabled");

	/* is_in_deep_sleep is set when receiveing the URC */
}

/* "device drivers must not perform any blocking operations during suspend" */
static int bc66x_pm_action(const struct device *dev, enum pm_device_action action)
{
	/* NOTE: Blocking operations are OK since CONFIG_PM_DEVICE_RUNTIME_USE_DEDICATED_WQ=y */
	struct bc66x_data *data = dev->data;

	if (!atomic_get(&data->is_initialized)) {
		return 0;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("Suspending modem...");

		bc66x_enter_light_sleep(data);

		return 0;
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("Resuming...");

		if (atomic_get(&data->modem_state) == BC66X_SLEEPING_STATE) {
			bc66x_wake_up(data);
		} else {
			LOG_DBG("No need to wake up modem (not sleeping)");
		}
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int bc66x_driver_init(const struct device *dev)
{
	struct bc66x_data *data = dev->data;
	const struct bc66x_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	if (cfg->is_offload_node) {
		g_bc66x_offload_data = data;
	}

	data->is_backend_ready = ATOMIC_INIT(0);
	data->is_initialized = ATOMIC_INIT(0);
	data->modem_state = ATOMIC_INIT(BC66X_INIT_STATE);
	data->is_in_deep_sleep = ATOMIC_INIT(0);
	data->is_setting_apn = ATOMIC_INIT(0);

	k_sem_init(&data->init_lock, 1, 1);
	k_sem_init(&data->script_lock, 1, 1);

	ret = bc66x_hw_init(dev->config);
	if (ret < 0) {
		LOG_ERR("bc66x_hw_init failed: %d", ret);
		return ret;
	}

	/* NETWORK DATA initialized in the "other" initializer */

	/* Init worker */
	k_work_init_delayable(&data->init_work, bc66x_init_work_handler);
	k_work_init_delayable(&data->deep_sleep_work, bc66x_enter_deep_sleep_work_handler);
	k_work_init_delayable(&data->set_apn_work, bc66x_set_apn_work_handler);

	k_work_queue_start(&data->bc66x_workq, cfg->workq_stack, cfg->workq_stack_size,
			   cfg->workq_priority, NULL);

	k_work_schedule_for_queue(&data->bc66x_workq, &data->init_work, BC66X_INIT_DELAY);

	LOG_DBG("BC66X init scheduled");
	return pm_device_driver_init(dev, bc66x_pm_action);
}

static int bc66x_offload_socket(int family, int type, int proto)
{
	if (g_bc66x_offload_data == NULL) {
		errno = ENODEV;
		return -1;
	}

	return bc66x_create_socket(g_bc66x_offload_data, family, type, proto);
}

/*
 * Only a single BC66x modem can implement socket offloading.
 * Select it via:
 *   / { chosen { quectel,bc66x-offload-modem = &my_node; }; };
 * If not chosen, default to the first bc660k instance, else the first bc66.
 */
#if DT_HAS_CHOSEN(quectel_bc66x_offload_modem)
#define BC66X_OFFLOAD_NODE DT_CHOSEN(quectel_bc66x_offload_modem)
#elif DT_HAS_COMPAT_STATUS_OKAY(quectel_bc660k)
#define BC66X_OFFLOAD_NODE DT_INST(0, quectel_bc660k)
#else
#define BC66X_OFFLOAD_NODE DT_INST(0, quectel_bc66)
#endif

BUILD_ASSERT(CONFIG_MODEM_QUECTEL_BC66X_NET_INIT_PRIORITY >
		     CONFIG_MODEM_QUECTEL_BC66X_INIT_PRIORITY,
	     "BC66X_NET_INIT_PRIORITY must be greater than BC66X_INIT_PRIORITY");

BUILD_ASSERT(DT_NODE_HAS_COMPAT(BC66X_OFFLOAD_NODE, quectel_bc66) ||
		     DT_NODE_HAS_COMPAT(BC66X_OFFLOAD_NODE, quectel_bc660k),
	     "quectel,bc66x-offload-modem must point to a BC66 or BC660K node");

#define BC66X_IS_OFFLOAD_NODE(node_id) DT_SAME_NODE(node_id, BC66X_OFFLOAD_NODE)

#define BC66X_SYM(prefix, node_id) UTIL_CAT(prefix, DT_DEP_ORD(node_id))

#define BC66X_DEFINE(node_id)                                            \
	/* Work queue stack for the "control device" */                      \
	K_KERNEL_STACK_DEFINE(BC66X_SYM(bc66x_init_stack_, node_id),         \
			      CONFIG_MODEM_QUECTEL_BC66X_WORKQ_STACK_SIZE);          \
	static struct bc66x_data BC66X_SYM(bc66x_data_, node_id) = {         \
		IF_ENABLED(BC66X_IS_OFFLOAD_NODE(node_id),                       \
			   (.net.socket_creator = bc66x_offload_socket,)) };         \
	static const struct bc66x_config BC66X_SYM(bc66x_cfg_, node_id) = {  \
		.boot_gpio = GPIO_DT_SPEC_GET(node_id, mdm_boot_gpios),          \
		.rst_gpio = GPIO_DT_SPEC_GET(node_id, mdm_rst_gpios),            \
		.psm_gpio = GPIO_DT_SPEC_GET(node_id, mdm_psm_gpios),            \
		.uart_dev = DEVICE_DT_GET(DT_BUS(node_id)),                      \
		.workq_stack = BC66X_SYM(bc66x_init_stack_, node_id),            \
		.workq_stack_size = K_KERNEL_STACK_SIZEOF(BC66X_SYM(bc66x_init_stack_, node_id)),\
		.workq_priority = CONFIG_MODEM_QUECTEL_BC66X_WORKQ_PRIORITY,     \
		.is_bc66 = DT_NODE_HAS_COMPAT(node_id, quectel_bc66),            \
		.is_offload_node = BC66X_IS_OFFLOAD_NODE(node_id),               \
		.send_cmd = DT_NODE_HAS_COMPAT(node_id, quectel_bc66) ?          \
					 BC66_SEND_HEX_AT_COMMAND : BC660K_SEND_HEX_AT_COMMAND,\
		.supported_ctx_id = DT_NODE_HAS_COMPAT(node_id, quectel_bc66) ?    \
						 BC66_ONLY_SUPPORTED_CONTEXT_ID :                  \
						 BC660K_ONLY_SUPPORTED_CONTEXT_ID,                 \
		.setup_script = DT_NODE_HAS_COMPAT(node_id, quectel_bc66) ?        \
					     &bc66_setup_script :  &bc660k_setup_script,       \
		.enable_light_sleep_script = DT_NODE_HAS_COMPAT(node_id, quectel_bc66) ? \
			&bc66_enable_light_sleep_script : &bc660k_enable_light_sleep_script, \
		.enable_deep_sleep_script = DT_NODE_HAS_COMPAT(node_id, quectel_bc66) ?  \
			&bc66_enable_deep_sleep_script : &bc660k_enable_deep_sleep_script,   \
	};                                                                           \
	PM_DEVICE_DEFINE(BC66X_SYM(bc66x_ctrl_pm_, node_id), bc66x_pm_action);       \
	IF_ENABLED(BC66X_IS_OFFLOAD_NODE(node_id),                                   \
	(NET_DEVICE_DT_OFFLOAD_DEFINE(node_id, bc66x_network_init,                   \
				     NULL, /* pm_device intentionally NULL */                      \
				     &BC66X_SYM(bc66x_data_, node_id), /* shared merged struct */  \
				     &BC66X_SYM(bc66x_cfg_, node_id),  /* config */                \
				     CONFIG_MODEM_QUECTEL_BC66X_NET_INIT_PRIORITY,                 \
				     &bc66x_net_api_funcs, /* offloaded_if_api defined outside */  \
				     BC66X_MTU);))                                                 \
	DEVICE_DEFINE(BC66X_SYM(bc66x_ctrl_, node_id),                                 \
		      "BC66X_CTRL_" STRINGIFY(DT_DEP_ORD(node_id)), bc66x_driver_init,     \
			      PM_DEVICE_GET(BC66X_SYM(bc66x_ctrl_pm_, node_id)), /* pm_device */  \
			      &BC66X_SYM(bc66x_data_, node_id), /* same shared merged struct */   \
			      &BC66X_SYM(bc66x_cfg_, node_id),  /* config */                      \
			      POST_KERNEL, CONFIG_MODEM_QUECTEL_BC66X_INIT_PRIORITY,              \
			      &bc66x_cellular_api);

NET_SOCKET_OFFLOAD_REGISTER(bc66x, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, NET_AF_UNSPEC,
			    bc66x_offload_is_supported, bc66x_offload_socket);

DT_FOREACH_STATUS_OKAY(quectel_bc660k, BC66X_DEFINE)
DT_FOREACH_STATUS_OKAY(quectel_bc66, BC66X_DEFINE)
