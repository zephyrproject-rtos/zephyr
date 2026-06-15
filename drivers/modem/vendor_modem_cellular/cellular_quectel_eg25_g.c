/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#define DT_DRV_COMPAT quectel_eg25_g

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(quectel_eg25_g_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES);

/*
 * AT+CMUX <port_speed> is specified in 3GPP TS 27.007 defining values 1..6 (up to 230400);
 * the EG25-G accepts vendor values for higher rates. The modem moves its multiplexer UART
 * to the named rate on CMUX entry, so it must track the host rate selected by
 * CONFIG_MODEM_CELLULAR_NEW_BAUDRATE.
 */
#if CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 921600
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "8"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 460800
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "7"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 230400
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "6"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 115200
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "5"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 57600
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "4"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 38400
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "3"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 19200
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "2"
#elif CONFIG_MODEM_CELLULAR_NEW_BAUDRATE == 9600
#define QUECTEL_EG25_G_CMUX_PORT_SPEED "1"
#else
#error "CONFIG_MODEM_CELLULAR_NEW_BAUDRATE unsupported for EG25-G CMUX port speed"
#endif

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_eg25_g_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	IF_ENABLED(DT_PROP(DT_BUS(DT_COMPAT_GET_ANY_STATUS_OKAY(quectel_eg25_g)), hw_flow_control),
		   (MODEM_CHAT_SCRIPT_CMD_RESP("AT+IFC=2,2", ok_match),))
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
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0," QUECTEL_EG25_G_CMUX_PORT_SPEED
					",127,10,3,30,10,2", 100));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_init_chat_script, quectel_eg25_g_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

#if CONFIG_MODEM_CELLULAR_NEW_BAUDRATE != 115200
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_eg25_g_set_baudrate_chat_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+IPR=" STRINGIFY(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE),
				   ok_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_set_baudrate_chat_script,
			 quectel_eg25_g_set_baudrate_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 10);
#endif

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_eg25_g_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATD*99***1#", connect_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_dial_chat_script, quectel_eg25_g_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_eg25_g_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_periodic_chat_script,
			 quectel_eg25_g_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

static const struct modem_cellular_vendor_config quectel_eg25_g_vendor = {
	/* clang-format off */
	.scripts = {
#if CONFIG_MODEM_CELLULAR_NEW_BAUDRATE != 115200
		.set_baudrate = &quectel_eg25_g_set_baudrate_chat_script,
#endif
		.init = &quectel_eg25_g_init_chat_script,
		.dial = &quectel_eg25_g_dial_chat_script,
		.periodic = &quectel_eg25_g_periodic_chat_script,
	},
	.unsol_matches = {
		.matches = quectel_eg25_g_unsol,
		.size = ARRAY_SIZE(quectel_eg25_g_unsol),
	},
	/* clang-format on */
	.power_pulse_duration_ms = 1500,
	.reset_pulse_duration_ms = 500,
	.startup_time_ms = 15000,
	.shutdown_time_ms = 5000,
};

#define MODEM_CELLULAR_DEVICE_QUECTEL_EG25_G(inst)                                                 \
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
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &quectel_eg25_g_vendor)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_EG25_G)
