/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	telit_mex10g1_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100), MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100), MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+ICCID", iccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match), MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match), MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127,10,3,30,10,2", 300));

MODEM_CHAT_SCRIPT_DEFINE(telit_mex10g1_init_chat_script, telit_mex10g1_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(telit_mex10g1_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0));

MODEM_CHAT_SCRIPT_DEFINE(telit_mex10g1_dial_chat_script, telit_mex10g1_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(telit_mex10g1_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(telit_mex10g1_periodic_chat_script,
			 telit_mex10g1_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

#if DT_HAS_COMPAT_STATUS_OKAY(telit_me310g1)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(telit_me310g1_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#SHDN", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(telit_me310g1_shutdown_chat_script,
			 telit_me310g1_shutdown_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 15);

#endif

__maybe_unused static const struct modem_cellular_config_scripts telit_me910g1_scripts = {
	.init = &telit_mex10g1_init_chat_script,
	.dial = &telit_mex10g1_dial_chat_script,
	.periodic = &telit_mex10g1_periodic_chat_script,
};

#define MODEM_CELLULAR_DEVICE_TELIT_ME910G1(inst)                                                  \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);   \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3))                          \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, 5050, 250, 15000, 5000, false, &telit_me910g1_scripts)

#if DT_HAS_COMPAT_STATUS_OKAY(telit_me310g1)
static const struct modem_cellular_config_scripts telit_me310g1_scripts = {
	.init = &telit_mex10g1_init_chat_script,
	.dial = &telit_mex10g1_dial_chat_script,
	.periodic = &telit_mex10g1_periodic_chat_script,
	.shutdown = &telit_me310g1_shutdown_chat_script,
};
#endif

#define MODEM_CELLULAR_DEVICE_TELIT_ME310G1(inst)                                                  \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);   \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3))                          \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, 5050, 0 /* unused */, 1000, 15000, false,             \
				       &telit_me310g1_scripts)

#define DT_DRV_COMPAT telit_me910g1
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_TELIT_ME910G1)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT telit_me310g1
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_TELIT_ME310G1)
#undef DT_DRV_COMPAT
