/*
 * Copyright (c) 2026 Mark Geiger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT quectel_bg770

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(quectel_bg770_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_bg770_set_baudrate_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+IPR=" STRINGIFY(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE),
						       ok_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg770_set_baudrate_chat_script, quectel_bg770_set_baudrate_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 1);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_bg770_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+QGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCCID", qccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127", 300));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg770_init_chat_script, quectel_bg770_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg770_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCFGEXT=\"FTP_SERVER\",1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGATT?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCFGEXT=\"pppmapping/subprofile\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGPADDR", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATD*99#", connect_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg770_dial_chat_script, quectel_bg770_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg770_network_setup_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QCFG=\"cmux/urcport\",1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg770_network_chat_script, quectel_bg770_network_setup_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 60);

MODEM_CHAT_MATCH_DEFINE(powerdown_match, "POWERED DOWN", "", NULL);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg770_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+QPOWD=1", powerdown_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg770_shutdown_chat_script,
			 quectel_bg770_shutdown_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 5);

static const struct modem_cellular_vendor_config quectel_bg770_vendor = {
	/* clang-format off */
	.scripts = {
		.set_baudrate = &quectel_bg770_set_baudrate_chat_script,
		.init = &quectel_bg770_init_chat_script,
		.network = &quectel_bg770_network_chat_script,
		.dial = &quectel_bg770_dial_chat_script,
		.shutdown = &quectel_bg770_shutdown_chat_script,
	},
	.unsol_matches = {
		.matches = quectel_bg770_unsol,
		.size = ARRAY_SIZE(quectel_bg770_unsol),
	},
	/* clang-format on */
	.chat_delimiter = "\r",
	.chat_filter = "\n",
	.power_pulse_duration_ms = 750,
	.reset_pulse_duration_ms = 850,
	.startup_time_ms = 8000,
	.shutdown_time_ms = 2000,
};

#define MODEM_CELLULAR_DEVICE_QUECTEL_BG770(inst)                                                  \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 1500, 64);       \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst);                    \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3), (user_pipe_1, 4))        \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &quectel_bg770_vendor)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_BG770)
