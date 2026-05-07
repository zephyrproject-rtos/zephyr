/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT u_blox_lara_r6

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	u_blox_lara_r6_set_baudrate_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+IPR=" STRINGIFY(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE),
						       ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_lara_r6_set_baudrate_chat_script,
			 u_blox_lara_r6_set_baudrate_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);

/* NOTE: For some reason, a CMUX max frame size of 127 causes FCS errors in
 * this modem; larger or smaller doesn't. The modem's default value is 31,
 * which works well
 */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	u_blox_lara_r6_init_chat_script_cmds,

	/* U-blox LARA-R6 LWM2M client is enabled by default. Not only causes
	 * this the modem to connect to U-blox's server on its own, it also
	 * for some reason causes the modem to reply "Destination
	 * unreachable" to DNS answers from DNS requests that we send
	 */
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+ULWM2M=1", allow_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
#if CONFIG_MODEM_CELLULAR_RAT_4G
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+URAT=3", ok_match),
#elif CONFIG_MODEM_CELLULAR_RAT_4G_3G
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+URAT=3,2", ok_match),
#elif CONFIG_MODEM_CELLULAR_RAT_4G_3G_2G
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+URAT=3,2,0", ok_match),
#endif
#if CONFIG_MODEM_CELLULAR_CLEAR_FORBIDDEN
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CRSM=214,28539,0,0,12,"
				   "\"FFFFFFFFFFFFFFFFFFFFFFFF\"",
				   ok_match),
#endif
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CCID", ccid_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMUX=0,0,5,31", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_lara_r6_init_chat_script, u_blox_lara_r6_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_lara_r6_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_lara_r6_dial_chat_script, u_blox_lara_r6_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_lara_r6_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_lara_r6_periodic_chat_script,
			 u_blox_lara_r6_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

static const struct modem_cellular_config_scripts u_blox_lara_r6_scripts = {
	.init = &u_blox_lara_r6_init_chat_script,
	.dial = &u_blox_lara_r6_dial_chat_script,
	.periodic = &u_blox_lara_r6_periodic_chat_script,
	.set_baudrate = &u_blox_lara_r6_set_baudrate_chat_script,
};

#define MODEM_CELLULAR_DEVICE_U_BLOX_LARA_R6(inst)                                                 \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);   \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (gnss_pipe, 3), (user_pipe_0, 4))          \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, 1500, 100, 9000, 5000, false, &u_blox_lara_r6_scripts)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_U_BLOX_LARA_R6)
