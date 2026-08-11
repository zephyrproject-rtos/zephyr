/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Julien Vermillard <julien@clunkymachines.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT fibocom_le250

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(fibocom_le250_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES);

/*
 * Configure reporting, collect the standard modem identifiers, and then put
 * the UART into 3GPP TS 27.010 basic-mode multiplexing at 115200 baud with a
 * 127-byte frame.
 */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	fibocom_le250_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100), MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
	MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=2", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CCID", ccid_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127,10,3,30,10,2", 1000));

MODEM_CHAT_SCRIPT_DEFINE(fibocom_le250_init_chat_script, fibocom_le250_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(fibocom_le250_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATD*99#", connect_match));

MODEM_CHAT_SCRIPT_DEFINE(fibocom_le250_dial_chat_script, fibocom_le250_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(fibocom_le250_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match));

MODEM_CHAT_SCRIPT_DEFINE(fibocom_le250_periodic_chat_script,
			 fibocom_le250_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

/*
 * The command returns OK before shutdown has necessarily finished. Keep the
 * script active for five seconds so that a following resume cannot pulse
 * PWRKEY while the modem is still completing its shutdown sequence.
 */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(fibocom_le250_shutdown_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CPWROFF", 5000));

MODEM_CHAT_SCRIPT_DEFINE(fibocom_le250_shutdown_chat_script,
			 fibocom_le250_shutdown_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 10);

static const struct modem_cellular_vendor_config fibocom_le250_vendor = {
	/* clang-format off */
	.scripts = {
		.init = &fibocom_le250_init_chat_script,
		.dial = &fibocom_le250_dial_chat_script,
		.periodic = &fibocom_le250_periodic_chat_script,
		.shutdown = &fibocom_le250_shutdown_chat_script,
	},
	.unsol_matches = {
		.matches = fibocom_le250_unsol,
		.size = ARRAY_SIZE(fibocom_le250_unsol),
	},
	/* clang-format on */
	.chat_delimiter = "\r",
	.chat_filter = "\n",
	/*
	 * PWRKEY must be asserted for more than 650 ms to power on and more
	 * than 3100 ms for hardware power-off. Normal shutdown uses the
	 * command above, so the long pulse is only a fallback.
	 */
	.power_pulse_duration_ms = 3200,
	.reset_pulse_duration_ms = 500,
	.startup_time_ms = 5000,
	.shutdown_time_ms = 5000,
};

#define MODEM_CELLULAR_DEVICE_FIBOCOM_LE250(inst)                                                  \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 1500, 64);       \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst);                    \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3))                          \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &fibocom_le250_vendor)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_FIBOCOM_LE250)
