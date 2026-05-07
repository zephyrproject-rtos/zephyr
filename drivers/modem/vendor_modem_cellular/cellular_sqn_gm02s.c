/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT sqn_gm02s

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	sqn_gm02s_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMUX=0,0,5,127", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(sqn_gm02s_init_chat_script, sqn_gm02s_init_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(sqn_gm02s_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CFUN=1", 10000),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATD*99***1#", connect_match));

MODEM_CHAT_SCRIPT_DEFINE(sqn_gm02s_dial_chat_script, sqn_gm02s_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 15);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(sqn_gm02s_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(sqn_gm02s_periodic_chat_script, sqn_gm02s_periodic_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 4);

static const struct modem_cellular_config_scripts sqn_gm02s_scripts = {
	.init = &sqn_gm02s_init_chat_script,
	.dial = &sqn_gm02s_dial_chat_script,
	.periodic = &sqn_gm02s_periodic_chat_script,
};

#define MODEM_CELLULAR_DEVICE_SQN_GM02S(inst)                                                      \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);   \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3), (user_pipe_1, 4))        \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, 1500, 100, 2000, 5000, true, &sqn_gm02s_scripts)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_SQN_GM02S)
