/** @file
 *  @brief Bluetooth Channel Sounding (CS) shell
 *
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/cs.h>
#include <errno.h>

#include "host/shell/bt.h"
#include "host/shell/bt_shell_private.h"

static int check_cs_sync_antenna_selection_input(uint16_t input)
{
	if (input != BT_LE_CS_ANTENNA_SELECTION_OPT_ONE &&
	    input != BT_LE_CS_ANTENNA_SELECTION_OPT_TWO &&
	    input != BT_LE_CS_ANTENNA_SELECTION_OPT_THREE &&
	    input != BT_LE_CS_ANTENNA_SELECTION_OPT_FOUR &&
	    input != BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE &&
	    input != BT_LE_CS_ANTENNA_SELECTION_OPT_NO_RECOMMENDATION) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_read_remote_supported_capabilities(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	err = bt_le_cs_read_remote_supported_capabilities(default_conn);
	if (err) {
		shell_error(sh, "bt_le_cs_read_remote_supported_capabilities returned error %d",
			    err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_set_default_settings(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	struct bt_le_cs_set_default_settings_param params;
	uint16_t antenna_input;
	int16_t tx_power_input;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	params.enable_initiator_role = shell_strtobool(argv[1], 10, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 1, Enable initiator role");
		return SHELL_CMD_HELP_PRINTED;
	}

	params.enable_reflector_role = shell_strtobool(argv[2], 10, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 2, Enable reflector role");
		return SHELL_CMD_HELP_PRINTED;
	}

	antenna_input = shell_strtoul(argv[3], 16, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 3, CS_SYNC antenna selection");
		return SHELL_CMD_HELP_PRINTED;
	}

	err = check_cs_sync_antenna_selection_input(antenna_input);
	if (err) {
		shell_help(sh);
		shell_error(sh, "CS_SYNC antenna selection input invalid");
		return SHELL_CMD_HELP_PRINTED;
	}

	tx_power_input = shell_strtol(argv[4], 10, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 4, Max TX power");
		return SHELL_CMD_HELP_PRINTED;
	}

	params.cs_sync_antenna_selection = antenna_input;
	params.max_tx_power = tx_power_input;

	err = bt_le_cs_set_default_settings(default_conn, &params);
	if (err) {
		shell_error(sh, "bt_le_cs_set_default_settings returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_read_remote_fae_table(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	err = bt_le_cs_read_remote_fae_table(default_conn);
	if (err) {
		shell_error(sh, "bt_le_cs_read_remote_fae_table returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static bool process_step_data(struct bt_le_cs_subevent_step *step, void *user_data)
{
	bt_shell_print("Subevent results contained step data: ");
	bt_shell_print("- Step mode %d\n"
		"- Step channel %d\n"
		"- Step data hexdump:",
		step->mode,
		step->channel);
	bt_shell_hexdump(step->data, step->data_len);

	return true;
}

static void cs_test_subevent_data_cb(struct bt_conn_le_cs_subevent_result *result)
{
	bt_shell_print("Received subevent results.");
	bt_shell_print("Subevent Header:\n"
		"- Procedure Counter: %d\n"
		"- Frequency Compensation: 0x%04x\n"
		"- Reference Power Level: %d\n"
		"- Procedure Done Status: 0x%02x\n"
		"- Subevent Done Status: 0x%02x\n"
		"- Procedure Abort Reason: 0x%02x\n"
		"- Subevent Abort Reason: 0x%02x\n"
		"- Number of Antenna Paths: %d\n"
		"- Number of Steps Reported: %d",
		result->header.procedure_counter,
		result->header.frequency_compensation,
		result->header.reference_power_level,
		result->header.procedure_done_status,
		result->header.subevent_done_status,
		result->header.procedure_abort_reason,
		result->header.subevent_abort_reason,
		result->header.num_antenna_paths,
		result->header.num_steps_reported);

	if (result->step_data_buf) {
		bt_le_cs_step_data_parse(result->step_data_buf, process_step_data, NULL);
	}
}

static void cs_test_end_complete_cb(void)
{
	bt_shell_print("CS Test End Complete.");
}

static int cmd_cs_test_simple(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	struct bt_le_cs_test_param params;

	params.main_mode = BT_CONN_LE_CS_MAIN_MODE_1;
	params.sub_mode = BT_CONN_LE_CS_SUB_MODE_UNUSED;
	params.main_mode_repetition = 0;
	params.mode_0_steps = 2;

	params.role = shell_strtoul(argv[1], 16, &err);

	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 1, Role selection");
		return SHELL_CMD_HELP_PRINTED;
	}

	if (params.role != BT_CONN_LE_CS_ROLE_INITIATOR &&
	    params.role != BT_CONN_LE_CS_ROLE_REFLECTOR) {
		shell_help(sh);
		shell_error(sh, "Role selection input invalid");
		return SHELL_CMD_HELP_PRINTED;
	}

	params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
	params.cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY;
	params.cs_sync_antenna_selection = BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_ONE;
	params.subevent_len = 3000;
	params.subevent_interval = 1;
	params.max_num_subevents = 1;
	params.transmit_power_level = BT_HCI_OP_LE_CS_TEST_MAXIMIZE_TX_POWER;
	params.t_ip1_time = 80;
	params.t_ip2_time = 80;
	params.t_fcs_time = 120;
	params.t_pm_time = 20;
	params.t_sw_time = 0;
	params.tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE;
	params.initiator_snr_control = BT_LE_CS_SNR_CONTROL_NOT_USED;
	params.reflector_snr_control = BT_LE_CS_SNR_CONTROL_NOT_USED;
	params.drbg_nonce = 0x1234;
	params.override_config = 0;
	params.override_config_0.channel_map_repetition = 1;
	memset(params.override_config_0.not_set.channel_map, 0,
	       sizeof(params.override_config_0.not_set.channel_map));
	params.override_config_0.not_set.channel_map[1] = 0xFF;
	params.override_config_0.not_set.channel_map[7] = 0xFF;
	params.override_config_0.not_set.channel_map[8] = 0xFF;
	params.override_config_0.not_set.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B;
	params.override_config_0.not_set.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT;
	params.override_config_0.not_set.ch3c_jump = 0x2;

	struct bt_le_cs_test_cb cs_test_cb = {
		.le_cs_test_subevent_data_available = cs_test_subevent_data_cb,
		.le_cs_test_end_complete = cs_test_end_complete_cb,
	};

	err = bt_le_cs_test_cb_register(cs_test_cb);
	if (err) {
		shell_error(sh, "bt_le_cs_test_cb_register returned error %d", err);
		return -ENOEXEC;
	}

	err = bt_le_cs_start_test(&params);
	if (err) {
		shell_error(sh, "bt_le_cs_start_test returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_remove_config(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	uint8_t config_id = strtoul(argv[1], NULL, 10);

	err = bt_le_cs_remove_config(default_conn, config_id);
	if (err) {
		shell_error(sh, "bt_le_cs_remove_config returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_create_config(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_le_cs_create_config_context context;
	struct bt_le_cs_create_config_params params;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	params.id = strtoul(argv[1], NULL, 10);
	if (!strcmp(argv[2], "local-only")) {
		context = BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_ONLY;
	} else if (!strcmp(argv[2], "local-only")) {
		context = BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE;
	} else {
		shell_error(sh, "Invalid context: %s", argv[2]);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (!strcmp(argv[3], "initiator")) {
		params.role = BT_CONN_LE_CS_ROLE_INITIATOR;
	} else if (!strcmp(argv[3], "reflector")) {
		params.role = BT_CONN_LE_CS_ROLE_REFLECTOR;
	} else {
		shell_error(sh, "Invalid role: %s", argv[3]);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	/* Set the default values */
	params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2;
	params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_1;
	params.min_main_mode_steps = 0x05;
	params.max_main_mode_steps = 0x0A;
	params.main_mode_repetition = 0;
	params.mode_0_steps = 1;
	params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
	params.cs_sync_phy = BT_CONN_LE_CS_SYNC_2M_PHY;
	params.channel_map_repetition = 1;
	params.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B;
	params.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT;
	params.ch3c_jump = 2;

	bt_le_cs_set_valid_chmap_bits(params.channel_map);

	for (int j = 4; j < argc; j++) {
		if (!strcmp(argv[j], "rtt-none")) {
			params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_1;
			params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_UNUSED;
		} else if (!strcmp(argv[j], "pbr-none")) {
			params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2;
			params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_UNUSED;
		} else if (!strcmp(argv[j], "both-none")) {
			params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_3;
			params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_UNUSED;
		} else if (!strcmp(argv[j], "pbr-rtt")) {
			params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2;
			params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_1;
		} else if (!strcmp(argv[j], "pbr-both")) {
			params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2;
			params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_3;
		} else if (!strcmp(argv[j], "both-pbr")) {
			params.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_3;
			params.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_2;
		} else if (!strcmp(argv[j], "steps")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			params.min_main_mode_steps = strtoul(argv[j], NULL, 10);
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			params.max_main_mode_steps = strtoul(argv[j], NULL, 10);
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			params.mode_0_steps = strtoul(argv[j], NULL, 10);
		} else if (!strcmp(argv[j], "aa-only")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
		} else if (!strcmp(argv[j], "32b-sound")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING;
		} else if (!strcmp(argv[j], "96b-sound")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING;
		} else if (!strcmp(argv[j], "32b-rand")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_32_BIT_RANDOM;
		} else if (!strcmp(argv[j], "64b-rand")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_64_BIT_RANDOM;
		} else if (!strcmp(argv[j], "96b-rand")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_96_BIT_RANDOM;
		} else if (!strcmp(argv[j], "128b-rand")) {
			params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_128_BIT_RANDOM;
		} else if (!strcmp(argv[j], "phy-1m")) {
			params.cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY;
		} else if (!strcmp(argv[j], "phy-2m")) {
			params.cs_sync_phy = BT_CONN_LE_CS_SYNC_2M_PHY;
		} else if (!strcmp(argv[j], "phy-2m-2b")) {
			params.cs_sync_phy = BT_CONN_LE_CS_SYNC_2M_2BT_PHY;
		} else if (!strcmp(argv[j], "chmap-rep")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			params.channel_map_repetition = strtoul(argv[j], NULL, 10);
		} else if (!strcmp(argv[j], "hat-shape")) {
			params.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT;
		} else if (!strcmp(argv[j], "x-shape")) {
			params.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_X;
		} else if (!strcmp(argv[j], "chsel-3b")) {
			params.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B;
		} else if (!strcmp(argv[j], "chsel-3c")) {
			params.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3C;
		} else if (!strcmp(argv[j], "ch3c-jump")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			params.ch3c_jump = strtoul(argv[j], NULL, 10);
		} else if (!strcmp(argv[j], "chmap")) {
			if (++j == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			if (hex2bin(argv[j], strlen(argv[j]), params.channel_map, 10) == 0) {
				shell_error(sh, "Invalid channel map");
				return -ENOEXEC;
			}

			sys_mem_swap(params.channel_map, 10);
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_le_cs_create_config(default_conn, &params, context);
	if (err) {
		shell_error(sh, "bt_le_cs_create_config returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cs_stop_test(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	err = bt_le_cs_stop_test();
	if (err) {
		shell_error(sh, "bt_cs_stop_test returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_read_local_supported_capabilities(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	struct bt_conn_le_cs_capabilities params;

	err = bt_le_cs_read_local_supported_capabilities(&params);

	if (err) {
		shell_error(sh, "bt_le_cs_read_local_supported_capabilities returned error %d",
			    err);

		return -ENOEXEC;
	}

	shell_print(
		sh,
		"Local channel sounding supported capabilities:\n"
		"- Num CS configurations: %d\n"
		"- Max consecutive CS procedures: %d\n"
		"- Num antennas supported: %d\n"
		"- Max antenna paths supported: %d\n"
		"- Initiator role supported: %s\n"
		"- Reflector role supported: %s\n"
		"- Mode 3 supported: %s\n"
		"- RTT AA only supported: %s\n"
		"- RTT AA only is 10ns precise: %s\n"
		"- RTT AA only N: %d\n"
		"- RTT sounding supported: %s\n"
		"- RTT sounding is 10ns precise: %s\n"
		"- RTT sounding N: %d\n"
		"- RTT random payload supported: %s\n"
		"- RTT random payload is 10ns precise: %s\n"
		"- RTT random payload N: %d\n"
		"- Phase-based NADM with sounding sequences supported: %s\n"
		"- Phase-based NADM with random sequences supported: %s\n"
		"- CS Sync 2M PHY supported: %s\n"
		"- CS Sync 2M 2BT PHY supported: %s\n"
		"- CS without transmitter FAE supported: %s\n"
		"- Channel selection algorithm #3c supported: %s\n"
		"- Phase-based ranging from RTT sounding sequence supported: %s\n"
		"- T_IP1 times supported: 0x%04x\n"
		"- T_IP2 times supported: 0x%04x\n"
		"- T_FCS times supported: 0x%04x\n"
		"- T_PM times supported: 0x%04x\n"
		"- T_SW time supported: %d us\n"
		"- TX SNR capability: 0x%02x",
		params.num_config_supported, params.max_consecutive_procedures_supported,
		params.num_antennas_supported, params.max_antenna_paths_supported,
		params.initiator_supported ? "Yes" : "No",
		params.reflector_supported ? "Yes" : "No", params.mode_3_supported ? "Yes" : "No",
		params.rtt_aa_only_precision == BT_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP ? "No" : "Yes",
		params.rtt_aa_only_precision == BT_CONN_LE_CS_RTT_AA_ONLY_10NS ? "Yes" : "No",
		params.rtt_aa_only_n,
		params.rtt_sounding_precision == BT_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP ? "No" : "Yes",
		params.rtt_sounding_precision == BT_CONN_LE_CS_RTT_SOUNDING_10NS ? "Yes" : "No",
		params.rtt_sounding_n,
		params.rtt_random_payload_precision == BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP
			? "No"
			: "Yes",
		params.rtt_random_payload_precision == BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS ? "Yes"
											     : "No",
		params.rtt_random_payload_n,
		params.phase_based_nadm_sounding_supported ? "Yes" : "No",
		params.phase_based_nadm_random_supported ? "Yes" : "No",
		params.cs_sync_2m_phy_supported ? "Yes" : "No",
		params.cs_sync_2m_2bt_phy_supported ? "Yes" : "No",
		params.cs_without_fae_supported ? "Yes" : "No",
		params.chsel_alg_3c_supported ? "Yes" : "No",
		params.pbr_from_rtt_sounding_seq_supported ? "Yes" : "No",
		params.t_ip1_times_supported, params.t_ip2_times_supported,
		params.t_fcs_times_supported, params.t_pm_times_supported, params.t_sw_time,
		params.tx_snr_capability);

	return 0;
}

static int cmd_write_cached_remote_supported_capabilities(const struct shell *sh, size_t argc,
							  char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	struct bt_conn_le_cs_capabilities params;

	params.num_config_supported = 1;
	params.max_consecutive_procedures_supported = 0;
	params.num_antennas_supported = 1;
	params.max_antenna_paths_supported = 1;
	params.initiator_supported = true;
	params.reflector_supported = true;
	params.mode_3_supported = true;
	params.rtt_aa_only_precision = BT_CONN_LE_CS_RTT_AA_ONLY_10NS;
	params.rtt_sounding_precision = BT_CONN_LE_CS_RTT_SOUNDING_10NS;
	params.rtt_random_payload_precision = BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS;
	params.rtt_aa_only_n = 5;
	params.rtt_sounding_n = 6;
	params.rtt_random_payload_n = 7;
	params.phase_based_nadm_sounding_supported = true;
	params.phase_based_nadm_random_supported = true;
	params.cs_sync_2m_phy_supported = true;
	params.cs_sync_2m_2bt_phy_supported = true;
	params.chsel_alg_3c_supported = true;
	params.cs_without_fae_supported = true;
	params.pbr_from_rtt_sounding_seq_supported = false;
	params.t_ip1_times_supported = BT_HCI_LE_CS_T_IP1_TIME_10US_MASK;
	params.t_ip2_times_supported = BT_HCI_LE_CS_T_IP2_TIME_10US_MASK;
	params.t_fcs_times_supported = BT_HCI_LE_CS_T_FCS_TIME_100US_MASK;
	params.t_sw_time = 0x04;
	params.tx_snr_capability = BT_HCI_LE_CS_TX_SNR_CAPABILITY_18DB_MASK;

	err = bt_le_cs_write_cached_remote_supported_capabilities(default_conn, &params);

	if (err) {
		shell_error(sh, "bt_le_cs_set_channel_classification returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_security_enable(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	err = bt_le_cs_security_enable(default_conn);

	if (err) {
		shell_error(sh, "bt_le_cs_security_enable returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_set_channel_classification(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	uint8_t channel_classification[10];

	for (int i = 0; i < 10; i++) {
		channel_classification[i] = shell_strtoul(argv[1 + i], 16, &err);

		if (err) {
			shell_help(sh);
			shell_error(sh, "Could not parse input %d, Channel Classification[%d]", i,
				    i);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_le_cs_set_channel_classification(channel_classification);

	if (err) {
		shell_error(sh, "bt_le_cs_set_channel_classification returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_set_procedure_parameters(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	struct bt_le_cs_set_procedure_parameters_param params;

	params.config_id = 0;
	params.max_procedure_len = 1000;
	params.min_procedure_interval = 5;
	params.max_procedure_interval = 5000;
	params.max_procedure_count = 1;
	params.min_subevent_len = 5000;
	params.max_subevent_len = 4000000;
	params.tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE;
	params.phy = 0x01;
	params.tx_power_delta = 0x80;
	params.preferred_peer_antenna = 1;
	params.snr_control_initiator = BT_LE_CS_SNR_CONTROL_18dB;
	params.snr_control_reflector = BT_LE_CS_SNR_CONTROL_18dB;

	err = bt_le_cs_set_procedure_parameters(default_conn, &params);

	if (err) {
		shell_error(sh, "bt_le_cs_set_procedure_parameters returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_procedure_enable(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	struct bt_le_cs_procedure_enable_param params;

	params.config_id = shell_strtoul(argv[1], 16, &err);

	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 1, Config ID");
		return SHELL_CMD_HELP_PRINTED;
	}

	params.enable = shell_strtoul(argv[2], 16, &err);

	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 2, Enable");
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_le_cs_procedure_enable(default_conn, &params);

	if (err) {
		shell_error(sh, "bt_le_cs_procedure_enable returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cs_cmds,
	SHELL_CMD_ARG(read_remote_supported_capabilities, NULL, "<None>",
		      cmd_read_remote_supported_capabilities, 1, 0),
	SHELL_CMD_ARG(
		set_default_settings, NULL,
		"<Enable initiator role: true, false> <Enable reflector role: true, false> "
		" <CS_SYNC antenna selection: 0x01 - 0x04, 0xFE, 0xFF> <Max TX power: -127 - 20>",
		cmd_set_default_settings, 5, 0),
	SHELL_CMD_ARG(read_remote_fae_table, NULL, "<None>", cmd_read_remote_fae_table, 1, 0),
	SHELL_CMD_ARG(start_simple_cs_test, NULL, "<Role selection (initiator, reflector): 0, 1>",
		      cmd_cs_test_simple, 2, 0),
	SHELL_CMD_ARG(stop_cs_test, NULL, "<None>", cmd_cs_stop_test, 1, 0),
	SHELL_CMD_ARG(
		create_config, NULL,
		"<id> <context: local-only, local-remote> <role: initiator, reflector> "
		"[rtt-none, pbr-none, both-none, pbr-rtt, pbr-both, both-pbr] [steps <min> "
		"<max> <mode-0>] [aa-only, 32b-sound, 96b-sound, 32b-rand, 64b-rand, 96b-rand, "
		"128b-rand] [phy-1m, phy-2m, phy-2m-2b] [chmap-rep <rep>] [hat-shape, x-shape] "
		"[ch3c-jump <jump>] [chmap <XXXXXXXXXXXXXXXX>] (78-0) [chsel-3b, chsel-3c]",
		cmd_create_config, 4, 15),
	SHELL_CMD_ARG(remove_config, NULL, "<id>", cmd_remove_config, 2, 0),
	SHELL_CMD_ARG(read_local_supported_capabilities, NULL, "<None>",
		      cmd_read_local_supported_capabilities, 1, 0),
	SHELL_CMD_ARG(write_cached_remote_supported_capabilities, NULL, "<None>",
		      cmd_write_cached_remote_supported_capabilities, 1, 0),
	SHELL_CMD_ARG(security_enable, NULL, "<None>", cmd_security_enable, 1, 0),
	SHELL_CMD_ARG(set_channel_classification, NULL,
		      "<Byte 0> <Byte 1> <Byte 2> <Byte 3> "
		      "<Byte 4> <Byte 5> <Byte 6> <Byte 7> <Byte 8> <Byte 9>",
		      cmd_set_channel_classification, 11, 0),
	SHELL_CMD_ARG(set_procedure_parameters, NULL, "<None>", cmd_set_procedure_parameters, 1, 0),
	SHELL_CMD_ARG(procedure_enable, NULL, "<Config ID: 0 to 3> <Enable: 0, 1>",
		      cmd_procedure_enable, 3, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_cs(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);

		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(cs, &cs_cmds, "Bluetooth CS shell commands", cmd_cs, 1, 1);
