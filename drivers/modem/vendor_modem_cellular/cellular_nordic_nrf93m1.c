/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#include <stdlib.h>

#define DT_DRV_COMPAT nordic_nrf93m1

static void nrf93m1_on_bcinfosc(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data);

MODEM_CELLULAR_COMMON_CHAT_MATCHES();
MODEM_CHAT_MATCH_DEFINE(pwd_match, "POWERED DOWN", "", NULL);

MODEM_CHAT_MATCHES_DEFINE(nordic_nrf93m1_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES,
			  MODEM_CHAT_MATCH("RDY", "", modem_cellular_chat_on_modem_ready),
			  MODEM_CHAT_MATCH("%BCINFOSC:", ",", nrf93m1_on_bcinfosc));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	nordic_nrf93m1_init_chat_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+IFC=2,2", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMUX=0,0,5,127", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf93m1_init_chat_script, nordic_nrf93m1_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

/* Differs from common `iccid_match` only in the % vs + prefix */
MODEM_CHAT_MATCH_DEFINE(nrf93m1_iccid_match, "%ICCID: ", "", modem_cellular_chat_on_iccid);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf93m1_network_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=2", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("", 500),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT%ICCID", nrf93m1_iccid_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf93m1_network_chat_script,
			 nordic_nrf93m1_network_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf93m1_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATD*99***1#", connect_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf93m1_dial_chat_script, nordic_nrf93m1_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 60);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf93m1_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT%POWD=1", pwd_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf93m1_shutdown_chat_script,
			 nordic_nrf93m1_shutdown_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 10);

/* Basic cell information for only the serving cell */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf93m1_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT%BCINFO=0,4,0,1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf93m1_periodic_chat_script,
			 nordic_nrf93m1_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

static void nrf93m1_on_bcinfosc(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	struct cellular_evt_network_status evt = {
		.status = data->registration_status_lte,
		.access_tech = data->access_tech,
	};

	if (argc >= 9) {
		/* MCC, MNC, GCI and TAC are quoted with " characters */
		evt.cell.lte.earfcn = strtol(argv[1], NULL, 10);
		evt.cell.lte.phys_cell_id = strtol(argv[2], NULL, 10);
		evt.cell.lte.rsrp = strtol(argv[3], NULL, 10);
		evt.cell.lte.rsrq = strtol(argv[4], NULL, 10);
		evt.cell.lte.mcc = strtol(argv[5] + 1, NULL, 10);
		evt.cell.lte.mnc = strtol(argv[6] + 1, NULL, 10);
		evt.cell.lte.gci = strtol(argv[7] + 1, NULL, 16);
		evt.cell.lte.tac = strtol(argv[8] + 1, NULL, 16);
		/* LTE band not explicitly provided (by any command)
		 * but can be derived from EARFCN offline. Set to 0 as
		 * an invalid marker.
		 */
		evt.cell.lte.band = 0;
	}

	modem_cellular_emit_event(data, CELLULAR_EVENT_NETWORK_STATUS_CHANGED, &evt);
}

static const struct modem_cellular_vendor_config nrf93m1_vendor = {
	/* clang-format off */
	.scripts = {
		.init = &nordic_nrf93m1_init_chat_script,
		.network = &nordic_nrf93m1_network_chat_script,
		.dial = &nordic_nrf93m1_dial_chat_script,
		.periodic = &nordic_nrf93m1_periodic_chat_script,
		.shutdown = &nordic_nrf93m1_shutdown_chat_script,
	},
	.unsol_matches = {
		.matches = nordic_nrf93m1_unsol,
		.size = ARRAY_SIZE(nordic_nrf93m1_unsol),
	},
	/* clang-format on */
	.power_pulse_duration_ms = 100,
	.reset_pulse_duration_ms = 500,
	/* In practice RDY event short circuits this timeout */
	.cmux_disconnect_timeout_ms = 3000,
	.startup_time_ms = 5000,
	.shutdown_time_ms = 1000,
};

#define MODEM_CELLULAR_DEVICE_NORDIC_NRF93M1(inst)                                                 \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 1500); \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r\n",                                                          \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3), (user_pipe_1, 4))        \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &nrf93m1_vendor)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_NORDIC_NRF93M1)
