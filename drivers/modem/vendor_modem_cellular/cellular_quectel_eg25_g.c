/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/modem/modem_cellular.h>

#include <stdlib.h>
#include <string.h>

#define DT_DRV_COMPAT quectel_eg25_g

static void quectel_eg25_g_on_qeng(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data);
static void quectel_eg25_g_on_qeng_scan_commit(struct modem_chat *chat, char **argv, uint16_t argc,
					       void *user_data);

MODEM_CELLULAR_COMMON_CHAT_MATCHES();

MODEM_CHAT_MATCHES_DEFINE(quectel_eg25_g_unsol, MODEM_CELLULAR_COMMON_UNSOL_MATCHES,
			  MODEM_CHAT_MATCH("+QENG: ", ",", quectel_eg25_g_on_qeng));

/*
 * The neighbourcell report spans several lines terminated by OK, or a bare
 * ERROR while the modem is not camped (searching / limited service). Both
 * terminate the step and commit, so the report never aborts the periodic script
 * and an ERROR while searching publishes an empty list rather than a stale one.
 * The staging buffer self-clears on commit, so the query is independent of any
 * other command in the periodic script.
 */
MODEM_CHAT_MATCHES_DEFINE(qeng_neighbourcell_match,
			  MODEM_CHAT_MATCH("OK", "", quectel_eg25_g_on_qeng_scan_commit),
			  MODEM_CHAT_MATCH("ERROR", "", quectel_eg25_g_on_qeng_scan_commit));

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

/*
 * argv[0] is the "+QENG: " prefix; the comma-separated fields follow, so field N
 * of the response is argv[N]. The cell-type token at argv[1] selects the report
 * kind. Non-LTE RATs use a different field layout and are ignored.
 */
#define QENG_CELLTYPE_ARGV 1

/*
 * LTE neighbours are reported as "neighbourcell intra"/"neighbourcell inter"; a
 * prefix match covers both, plus the bare GSM/WCDMA "neighbourcell" token that
 * the RAT guard then filters out.
 */
#define QENG_NEIGHBOURCELL_PREFIX "\"neighbourcell"

/* RAT token as it appears in argv, i.e. with the QENG surrounding quotes. */
#define QENG_LTE_RAT_TOKEN "\"LTE\""

/* Placeholder the modem emits for a field it has not measured. */
#define QENG_UNMEASURED_FIELD "-"

/*
 * Serving cell (LTE):
 * +QENG: "servingcell",<state>,"LTE",<is_tdd>,<mcc>,<mnc>,<cellid>,<pcid>,
 *        <earfcn>,<freq_band_ind>,<ul_bw>,<dl_bw>,<tac>,<rsrp>,<rsrq>,...
 */
enum {
	QENG_LTE_RAT = 3,
	QENG_LTE_MCC = 5,
	QENG_LTE_MNC = 6,
	QENG_LTE_CELLID = 7,
	QENG_LTE_PCID = 8,
	QENG_LTE_EARFCN = 9,
	QENG_LTE_BAND = 10,
	QENG_LTE_TAC = 13,
	QENG_LTE_RSRP = 14,
	QENG_LTE_RSRQ = 15,
	QENG_LTE_MIN_ARGC = 16,
};

/*
 * Neighbour cell (LTE intra/inter):
 * +QENG: "neighbourcell intra","LTE",<earfcn>,<pcid>,<rsrq>,<rsrp>,<rssi>,...
 * +QENG: "neighbourcell inter","LTE",<earfcn>,<pcid>,<rsrq>,<rsrp>,<rssi>,...
 * Both layouts share the leading fields used here (rsrq precedes rsrp, unlike
 * the serving-cell report).
 */
enum {
	QENG_NBR_LTE_RAT = 2,
	QENG_NBR_LTE_EARFCN = 3,
	QENG_NBR_LTE_PCID = 4,
	QENG_NBR_LTE_RSRQ = 5,
	QENG_NBR_LTE_RSRP = 6,
	QENG_NBR_LTE_MIN_ARGC = 7,
};

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
		MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
		MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
		MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0," QUECTEL_EG25_G_CMUX_PORT_SPEED
						",127,10,3,30,10,2",
						100));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_init_chat_script, quectel_eg25_g_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

#if CONFIG_MODEM_CELLULAR_NEW_BAUDRATE != 115200
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_eg25_g_set_baudrate_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
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
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+QENG=\"servingcell\"",
							      allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+QENG=\"neighbourcell\"",
							      qeng_neighbourcell_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_periodic_chat_script,
			 quectel_eg25_g_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

static void quectel_eg25_g_parse_neighbourcell(struct modem_cellular_data *data, char **argv,
					       uint16_t argc)
{
	struct cellular_neighbor_cell cell = {0};

	/*
	 * Only LTE neighbours carry these fields; GSM/WCDMA neighbours reported
	 * while camped on LTE use a different layout.
	 */
	if (argc < QENG_NBR_LTE_MIN_ARGC ||
	    strcmp(argv[QENG_NBR_LTE_RAT], QENG_LTE_RAT_TOKEN) != 0) {
		return;
	}

	/*
	 * Inter-frequency rows list a frequency even when no cell was found on it,
	 * leaving "-" placeholders for the identity and measurement fields. Drop
	 * those so the list holds only measured neighbours, not empty candidates.
	 */
	if (strcmp(argv[QENG_NBR_LTE_PCID], QENG_UNMEASURED_FIELD) == 0 ||
	    strcmp(argv[QENG_NBR_LTE_RSRP], QENG_UNMEASURED_FIELD) == 0 ||
	    strcmp(argv[QENG_NBR_LTE_RSRQ], QENG_UNMEASURED_FIELD) == 0) {
		return;
	}

	/* All neighbour fields are decimal and unquoted. */
	cell.cell.lte.earfcn = (uint32_t)strtoul(argv[QENG_NBR_LTE_EARFCN], NULL, 10);
	cell.cell.lte.phys_cell_id = (uint16_t)strtoul(argv[QENG_NBR_LTE_PCID], NULL, 10);
	cell.cell.lte.rsrq = (int8_t)strtol(argv[QENG_NBR_LTE_RSRQ], NULL, 10);
	cell.cell.lte.rsrp = (int16_t)strtol(argv[QENG_NBR_LTE_RSRP], NULL, 10);

	/*
	 * The intra-frequency report echoes the camped cell as a row. PCI is
	 * unique per EARFCN, so a row matching the serving (EARFCN, PCI) is the
	 * serving cell, not a neighbour; drop it. The periodic script polls
	 * servingcell before neighbourcell, so the cached value is current here.
	 */
	if (data->network_status_valid &&
	    cell.cell.lte.earfcn == data->network_status.cell.lte.earfcn &&
	    cell.cell.lte.phys_cell_id == data->network_status.cell.lte.phys_cell_id) {
		return;
	}

	modem_cellular_add_neighbor_cell(data, &cell);
}

static void quectel_eg25_g_on_qeng(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	struct cellular_evt_network_status evt = {
		.status = data->registration_status_lte,
		.access_tech = data->access_tech,
	};

	/*
	 * Neighbour cell reports share the "+QENG: " prefix but carry their own
	 * field layout; handle them separately from the serving-cell report below.
	 */
	if (argc > QENG_CELLTYPE_ARGV &&
	    strncmp(argv[QENG_CELLTYPE_ARGV], QENG_NEIGHBOURCELL_PREFIX,
		    strlen(QENG_NEIGHBOURCELL_PREFIX)) == 0) {
		quectel_eg25_g_parse_neighbourcell(data, argv, argc);
		return;
	}

	/*
	 * Only the LTE report carries the fields below; a shorter line (state
	 * SEARCH/LIMSRV) or a non-LTE RAT has a different layout.
	 */
	if (argc < QENG_LTE_MIN_ARGC || strcmp(argv[QENG_LTE_RAT], QENG_LTE_RAT_TOKEN) != 0) {
		return;
	}

	/* mcc/mnc are decimal, cellid/tac hexadecimal, all unquoted. */
	evt.cell.lte.mcc = (uint16_t)strtoul(argv[QENG_LTE_MCC], NULL, 10);
	evt.cell.lte.mnc = (uint16_t)strtoul(argv[QENG_LTE_MNC], NULL, 10);
	evt.cell.lte.gci = (uint32_t)strtoul(argv[QENG_LTE_CELLID], NULL, 16);
	evt.cell.lte.phys_cell_id = (uint16_t)strtoul(argv[QENG_LTE_PCID], NULL, 10);
	evt.cell.lte.earfcn = (uint32_t)strtoul(argv[QENG_LTE_EARFCN], NULL, 10);
	evt.cell.lte.band = (uint16_t)strtoul(argv[QENG_LTE_BAND], NULL, 10);
	evt.cell.lte.tac = (uint32_t)strtoul(argv[QENG_LTE_TAC], NULL, 16);
	evt.cell.lte.rsrp = (int16_t)strtol(argv[QENG_LTE_RSRP], NULL, 10);
	evt.cell.lte.rsrq = (int8_t)strtol(argv[QENG_LTE_RSRQ], NULL, 10);

	modem_cellular_emit_network_status(data, &evt);
}

static void quectel_eg25_g_on_qeng_scan_commit(struct modem_chat *chat, char **argv, uint16_t argc,
					       void *user_data)
{
	modem_cellular_commit_neighbor_cells((struct modem_cellular_data *)user_data);
}

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
	.chat_delimiter = "\r",
	.chat_filter = "\n",
	.power_pulse_duration_ms = 1500,
	.reset_pulse_duration_ms = 500,
	.startup_time_ms = 15000,
	.shutdown_time_ms = 5000,
};

#define MODEM_CELLULAR_DEVICE_QUECTEL_EG25_G(inst)                                                 \
	MODEM_DT_INST_PPP_DEFINE(inst, MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 1500, 64);       \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst);                    \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, (user_pipe_0, 3), (user_pipe_1, 4))        \
                                                                                                   \
	MODEM_CELLULAR_DEFINE_INSTANCE(inst, &quectel_eg25_g_vendor)

DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_EG25_G)
