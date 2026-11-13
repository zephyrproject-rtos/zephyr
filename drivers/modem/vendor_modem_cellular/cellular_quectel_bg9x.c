/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#include <stdlib.h>
#include <string.h>

static void quectel_bg9x_on_qeng(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(quectel_bg9x_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES,
			  MODEM_CHAT_MATCH("+QENG: ", ",", quectel_bg9x_on_qeng));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_bg9x_set_baudrate_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+IPR=" STRINGIFY(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE),
						       ok_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg9x_set_baudrate_chat_script, quectel_bg9x_set_baudrate_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 1);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_bg9x_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGSN", imei_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMM", cgmm_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMI", cgmi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+QGMR", cgmr_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CIMI", cimi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+QCCID", qccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5," STRINGIFY(CONFIG_MODEM_CMUX_MTU), 300));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg9x_init_chat_script, quectel_bg9x_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg9x_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(
				"ATD*99***" STRINGIFY(CONFIG_MODEM_CELLULAR_PDP_CONTEXT_ID) "#",
				connect_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg9x_dial_chat_script, quectel_bg9x_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg9x_network_setup_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCFG=\"cmux/urcport\",1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg9x_network_chat_script, quectel_bg9x_network_setup_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 60);

/*
 * The serving cell report is informational: a firmware that rejects AT+QENG must not fail
 * the periodic script, as repeated failures restart the modem.
 */
MODEM_CHAT_MATCHES_DEFINE(quectel_bg9x_qeng_result_match, MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL),
			  MODEM_CHAT_MATCH("+CME ERROR", "", NULL));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg9x_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+QENG=\"servingcell\"",
							      quectel_bg9x_qeng_result_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg9x_periodic_chat_script, quectel_bg9x_periodic_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 4);

/*
 * Field positions in the LTE-M and NB-IoT serving cell report:
 * +QENG: "servingcell",<state>,<rat>,<is_tdd>,<mcc>,<mnc>,<cellid>,<pcid>,
 *        <earfcn>,<freq_band_ind>,<ul_bw>,<dl_bw>,<tac>,<rsrp>,<rsrq>,<rssi>,<sinr>,<srxlev>
 * <rat> is "eMTC" or "NBIoT" on the BG95, "CAT-M" or "CAT-NB" on the BG96,
 * e.g. +QENG: "servingcell","CONNECT","eMTC","FDD",724,06,2998B15,464,1700,3,5,5,C7DF,-71,-8,
 *      -48,20,
 * argv[0] is the "+QENG: " prefix, so field N is argv[N]. While searching the modem answers
 * with the state only, and GSM uses a different layout; both are ignored.
 */
enum {
	QENG_RAT = 3,
	QENG_MCC = 5,
	QENG_MNC = 6,
	QENG_CELLID = 7,
	QENG_PCID = 8,
	QENG_EARFCN = 9,
	QENG_BAND = 10,
	QENG_TAC = 13,
	QENG_RSRP = 14,
	QENG_RSRQ = 15,
	QENG_MIN_ARGC = 16,
};

static void quectel_bg9x_on_qeng(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	struct cellular_evt_network_status evt = {
		.status = data->registration_status_lte,
	};

	ARG_UNUSED(chat);

	if (argc < QENG_MIN_ARGC) {
		return;
	}

	/* The RAT token keeps its quotes in argv. */
	if ((strcmp(argv[QENG_RAT], "\"eMTC\"") == 0) ||
	    (strcmp(argv[QENG_RAT], "\"CAT-M\"") == 0)) {
		evt.access_tech = CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN;
	} else if ((strcmp(argv[QENG_RAT], "\"NBIoT\"") == 0) ||
		   (strcmp(argv[QENG_RAT], "\"CAT-NB\"") == 0)) {
		evt.access_tech = CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1;
	} else {
		return;
	}

	/* mcc/mnc are decimal, cellid/tac hexadecimal, all unquoted. */
	evt.cell.lte.mcc = (uint16_t)strtoul(argv[QENG_MCC], NULL, 10);
	evt.cell.lte.mnc = (uint16_t)strtoul(argv[QENG_MNC], NULL, 10);
	evt.cell.lte.gci = (uint32_t)strtoul(argv[QENG_CELLID], NULL, 16);
	evt.cell.lte.phys_cell_id = (uint16_t)strtoul(argv[QENG_PCID], NULL, 10);
	evt.cell.lte.earfcn = (uint32_t)strtoul(argv[QENG_EARFCN], NULL, 10);
	evt.cell.lte.band = (uint16_t)strtoul(argv[QENG_BAND], NULL, 10);
	evt.cell.lte.tac = (uint32_t)strtoul(argv[QENG_TAC], NULL, 16);
	evt.cell.lte.rsrp = (int16_t)strtol(argv[QENG_RSRP], NULL, 10);
	evt.cell.lte.rsrq = (int8_t)strtol(argv[QENG_RSRQ], NULL, 10);

	modem_cellular_emit_network_status(data, &evt);
}

MODEM_CHAT_MATCH_DEFINE(powerdown_match, "POWERED DOWN", "", NULL);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg9x_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QPOWD=1", powerdown_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg9x_shutdown_chat_script, quectel_bg9x_shutdown_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 5);

static const struct modem_cellular_vendor_config quectel_bg9x_vendor = {
	/* clang-format off */
	.scripts = {
		.set_baudrate = &quectel_bg9x_set_baudrate_chat_script,
		.init = &quectel_bg9x_init_chat_script,
		.network = &quectel_bg9x_network_chat_script,
		.dial = &quectel_bg9x_dial_chat_script,
		.periodic = &quectel_bg9x_periodic_chat_script,
		.shutdown = &quectel_bg9x_shutdown_chat_script,
	},
	.unsol_matches = {
		.matches = quectel_bg9x_unsol,
		.size = ARRAY_SIZE(quectel_bg9x_unsol),
	},
	/* clang-format on */
	.chat_delimiter = "\r",
	.chat_filter = "\n",
	.power_pulse_duration_ms = 500,
	.reset_pulse_duration_ms = 1000,
	.startup_time_ms = 5000,
	.shutdown_time_ms = 2000,
};

#define MODEM_CELLULAR_DEVICE_QUECTEL_BG9X(inst)                                                   \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 1500, 64);       \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst);                    \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3), (user_pipe_1, 4))        \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &quectel_bg9x_vendor, NULL)

#define DT_DRV_COMPAT quectel_bg95
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_BG9X)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT quectel_bg96
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_BG9X)
#undef DT_DRV_COMPAT
