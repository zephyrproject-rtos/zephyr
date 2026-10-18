/*
 * Copyright (c) 2026, Trasna.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT trasna_lexi_r7

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(trasna_lexi_r7_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES);
MODEM_CHAT_MATCH_DEFINE(trasna_lexi_r7_sim_ready_match, "+UUSIMSTAT: 6", "", NULL);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	trasna_lexi_r7_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+USIMSTAT=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", trasna_lexi_r7_sim_ready_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGEREP=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGSN", imei_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMM", cgmm_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMI", cgmi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMR", cgmr_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CIMI", cimi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CCID", ccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMUX=0,0,5," STRINGIFY(CONFIG_MODEM_CMUX_MTU), ok_match));

MODEM_CHAT_SCRIPT_DEFINE(trasna_lexi_r7_init_chat_script, trasna_lexi_r7_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(trasna_lexi_r7_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE(
				"ATD*99***" STRINGIFY(CONFIG_MODEM_CELLULAR_PDP_CONTEXT_ID) "#",
				1000));

MODEM_CHAT_SCRIPT_DEFINE(trasna_lexi_r7_dial_chat_script, trasna_lexi_r7_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(trasna_lexi_r7_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(trasna_lexi_r7_periodic_chat_script,
			 trasna_lexi_r7_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(trasna_lexi_r7_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPWROFF", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(trasna_lexi_r7_shutdown_chat_script,
			 trasna_lexi_r7_shutdown_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 20);

static const struct modem_cellular_vendor_config_common trasna_lexi_r7_vendor = {
	/* clang-format off */
	.scripts = {
		.init = &trasna_lexi_r7_init_chat_script,
		.dial = &trasna_lexi_r7_dial_chat_script,
		.periodic = &trasna_lexi_r7_periodic_chat_script,
		.shutdown = &trasna_lexi_r7_shutdown_chat_script,
	},
	.unsol_matches = {
		.matches = trasna_lexi_r7_unsol,
		.size = ARRAY_SIZE(trasna_lexi_r7_unsol),
	},
	/* clang-format on */
	.chat_delimiter = "\r",
	.chat_filter = "\n",
	.baudrate_update_delay_ms = 300,
	.reset_power_on_delay_ms = 500,
	.power_pulse_duration_ms = 100,
	.reset_pulse_duration_ms = 600,
	.startup_time_ms = 10000,
	.shutdown_time_ms = 5000,
	.force_autostart = true,
};

#define MODEM_CELLULAR_DEVICE_TRASNA_LEXI_R7(inst)                                                 \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 1500, 64);       \
                                                                                                   \
	MODEM_CELLULAR_COMMON_BAUDRATE_CHAT_SCRIPT(inst);                                          \
                                                                                                   \
	static const struct modem_cellular_vendor_config_inst MODEM_CELLULAR_INST_NAME(            \
		vendor_config_inst, inst) = {                                                      \
		MODEM_CELLULAR_VENDOR_CONFIG_INST_TARGET_BAUDRATE(inst),                           \
	};                                                                                         \
                                                                                                   \
	static const struct modem_cellular_vendor_config MODEM_CELLULAR_INST_NAME(vendor_config,   \
										  inst) = {        \
		.common = &trasna_lexi_r7_vendor,                                                  \
		.instance = &MODEM_CELLULAR_INST_NAME(vendor_config_inst, inst),                   \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst);                    \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3))                          \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &MODEM_CELLULAR_INST_NAME(vendor_config, inst))

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_TRASNA_LEXI_R7)
