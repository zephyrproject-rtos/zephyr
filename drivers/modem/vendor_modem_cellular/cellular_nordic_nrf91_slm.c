/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT nordic_nrf91_slm

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCH_DEFINE(xiccid_match, "%XICCID: ", "", modem_cellular_chat_on_iccid);
MODEM_CHAT_MATCH_DEFINE(uicc_initialized, "%XSIM: 1", "", NULL);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_init_chat_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT%XSIM=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=41", uicc_initialized),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT%XICCID", xiccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT%XSIM=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT#XCMUX=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_init_chat_script, nordic_nrf91_slm_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_network_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XPPP=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_network_chat_script, nordic_nrf91_slm_network_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

/* In nRF91 Serial Modem AT#XPPP=1 triggers PPP automatically on a secondary DLC,
 * so empty dial script is used
 */
MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(nordic_nrf91_slm_dial_chat_script, modem_chat_empty_script_chats,
				  modem_cellular_chat_callback_handler, 0);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_shutdown_chat_script,
			 nordic_nrf91_slm_shutdown_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 5);

static const struct modem_cellular_config_scripts nrf91_slm_scripts = {
	.init = &nordic_nrf91_slm_init_chat_script,
	.network = &nordic_nrf91_slm_network_chat_script,
	.dial = &nordic_nrf91_slm_dial_chat_script,
	.shutdown = &nordic_nrf91_slm_shutdown_chat_script,
};

#define MODEM_CELLULAR_DEVICE_NORDIC_NRF91_SLM(inst)                                               \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 1500); \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r\n",                                                          \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (gnss_pipe, 3))                            \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, 0, 500, 5000, 0, false, &nrf91_slm_scripts)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_NORDIC_NRF91_SLM)
