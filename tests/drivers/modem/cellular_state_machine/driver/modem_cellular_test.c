/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/drivers/uart_emul.h>

#include <stdlib.h>

#include "modem_cellular_test.h"

#define DT_DRV_COMPAT zephyr_modem_cellular_test

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(test_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES);

/* Optional scripts are only defined when an instance uses them */
#if !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(no_set_baudrate_script)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(set_baudrate_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD,
							 ok_match));
MODEM_CHAT_SCRIPT_DEFINE(set_baudrate_chat_script, set_baudrate_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

#if !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(modem_info_script)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(init_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_INIT_CMD, ok_match));
MODEM_CHAT_SCRIPT_DEFINE(init_chat_script, init_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(modem_info_script)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	info_init_chat_cmds, MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_INIT_CMD, ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_IMEI_SN_CMD, imei_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_IMEI_CMD, cgsn_imei_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_SERIAL_NUMBER_CMD, cgsn_sn_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_MODEL_ID_CMD, cgmm_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_MANUFACTURER_CMD, cgmi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_FW_VERSION_CMD, cgmr_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_IMSI_CMD, cimi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_ICCID_CMD, iccid_match));
MODEM_CHAT_SCRIPT_DEFINE(init_info_chat_script, info_init_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

#if !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(no_network_script)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(network_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_NETWORK_CMD,
							 ok_match));
MODEM_CHAT_SCRIPT_DEFINE(network_chat_script, network_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

MODEM_CHAT_SCRIPT_CMDS_DEFINE(dial_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_DIAL_CMD, ok_match));
MODEM_CHAT_SCRIPT_DEFINE(dial_chat_script, dial_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);

#if !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(no_periodic_script)
enum {
	SERVING_CELL_MCC = 1,
	SERVING_CELL_MNC,
	SERVING_CELL_GCI,
	SERVING_CELL_TAC,
	SERVING_CELL_EARFCN,
	SERVING_CELL_BAND,
	SERVING_CELL_PHYS_CELL_ID,
	SERVING_CELL_RSRP,
	SERVING_CELL_RSRQ,
	SERVING_CELL_ARGC,
};

/* Report the serving cell as vendor drivers do from their periodic scripts */
static void test_on_serving_cell(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = user_data;
	struct cellular_evt_network_status evt = {
		.status = data->registration_status_lte,
		.access_tech = data->access_tech,
	};

	if (argc != SERVING_CELL_ARGC) {
		return;
	}

	evt.cell.lte.mcc = (uint16_t)strtoul(argv[SERVING_CELL_MCC], NULL, 10);
	evt.cell.lte.mnc = (uint16_t)strtoul(argv[SERVING_CELL_MNC], NULL, 10);
	evt.cell.lte.gci = (uint32_t)strtoul(argv[SERVING_CELL_GCI], NULL, 10);
	evt.cell.lte.tac = (uint32_t)strtoul(argv[SERVING_CELL_TAC], NULL, 10);
	evt.cell.lte.earfcn = (uint32_t)strtoul(argv[SERVING_CELL_EARFCN], NULL, 10);
	evt.cell.lte.band = (uint16_t)strtoul(argv[SERVING_CELL_BAND], NULL, 10);
	evt.cell.lte.phys_cell_id = (uint16_t)strtoul(argv[SERVING_CELL_PHYS_CELL_ID], NULL, 10);
	evt.cell.lte.rsrp = (int16_t)strtol(argv[SERVING_CELL_RSRP], NULL, 10);
	evt.cell.lte.rsrq = (int8_t)strtol(argv[SERVING_CELL_RSRQ], NULL, 10);

	modem_cellular_emit_network_status(data, &evt);
}

MODEM_CELLULAR_OK_CHAT_MATCH_DEFINE(serving_cell_match, MODEM_CELLULAR_TEST_SERVING_CELL, ",",
				    test_on_serving_cell);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(periodic_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT(MODEM_CELLULAR_TEST_PERIODIC_CMD,
							      serving_cell_match));
MODEM_CHAT_SCRIPT_DEFINE(periodic_chat_script, periodic_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

#if !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(no_shutdown_script)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(shutdown_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_SHUTDOWN_CMD,
							 ok_match));
MODEM_CHAT_SCRIPT_DEFINE(shutdown_chat_script, shutdown_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

#if !DT_ALL_INST_HAS_BOOL_STATUS_OKAY(no_dlci_setup_script)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(dlci_setup_chat_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(MODEM_CELLULAR_TEST_DLCI_SETUP_CMD,
							 ok_match));
MODEM_CHAT_SCRIPT_DEFINE(dlci_setup_chat_script, dlci_setup_chat_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 1);
#endif

#ifdef CONFIG_EMUL
static int modem_cellular_test_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

static const struct uart_emul_device_api test_emul_api;
#endif

#define TEST_SCRIPT(inst, property, script)                                                        \
	COND_CODE_1(DT_INST_PROP(inst, property), (NULL), (&script))

#define INIT_SCRIPT(inst)                                                                          \
	COND_CODE_1(DT_INST_PROP(inst, modem_info_script),                                         \
		    (&init_info_chat_script),                                                      \
		    (&init_chat_script))

#define MODEM_CELLULAR_DEVICE_TEST(inst)                                                           \
	static const struct modem_cellular_vendor_config MODEM_CELLULAR_INST_NAME(vendor,          \
										  inst) = {        \
		.scripts =                                                                         \
			{                                                                          \
				.set_baudrate = TEST_SCRIPT(inst, no_set_baudrate_script,          \
							    set_baudrate_chat_script),             \
				.init = INIT_SCRIPT(inst),                                         \
				.network =                                                         \
					TEST_SCRIPT(inst, no_network_script, network_chat_script), \
				.dial = &dial_chat_script,                                         \
				.periodic = TEST_SCRIPT(inst, no_periodic_script,                  \
							periodic_chat_script),                     \
				.shutdown = TEST_SCRIPT(inst, no_shutdown_script,                  \
							shutdown_chat_script),                     \
				.dlci_setup = TEST_SCRIPT(inst, no_dlci_setup_script,              \
							  dlci_setup_chat_script),                 \
			},                                                                         \
		.unsol_matches = {.matches = test_unsol, .size = ARRAY_SIZE(test_unsol)},          \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.power_pulse_duration_ms = DT_INST_PROP(inst, power_pulse_duration_ms),            \
		.reset_pulse_duration_ms = DT_INST_PROP(inst, reset_pulse_duration_ms),            \
		.cmux_disconnect_timeout_ms = DT_INST_PROP(inst, cmux_disconnect_timeout_ms),      \
		.startup_time_ms = DT_INST_PROP(inst, startup_time_ms),                            \
		.shutdown_time_ms = DT_INST_PROP(inst, shutdown_time_ms),                          \
	};

#define MODEM_CELLULAR_DEVICE_TEST_DEFINE(inst)                                                    \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 1500, 1500);     \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst);                    \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3), (user_pipe_1, 4))        \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &MODEM_CELLULAR_INST_NAME(vendor, inst), NULL)        \
	IF_ENABLED(CONFIG_EMUL, (EMUL_DT_INST_DEFINE(inst, modem_cellular_test_emul_init, NULL,    \
						    NULL, &test_emul_api, NULL)))

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_TEST)
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_TEST_DEFINE)
