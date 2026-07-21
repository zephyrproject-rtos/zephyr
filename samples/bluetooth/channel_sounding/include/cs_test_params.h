/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/cs.h>

#define INITIATOR_ACCESS_ADDRESS 0x4D7B8A2F
#define REFLECTOR_ACCESS_ADDRESS 0x96F93DB1
#define NUM_MODE_0_STEPS         3

static struct bt_le_cs_test_param test_params_get(enum bt_conn_le_cs_role role)
{
	struct bt_le_cs_test_param params;

	params.role = role;
	params.mode = BT_CONN_LE_CS_MAIN_MODE_2_SUB_MODE_1;
	params.main_mode_repetition = 1;
	params.mode_0_steps = NUM_MODE_0_STEPS;
	params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
	params.cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY;
	params.cs_sync_antenna_selection = BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_ONE;
	params.subevent_len = 5000;
	params.subevent_interval = 0;
	params.max_num_subevents = 1;
	params.transmit_power_level = BT_HCI_OP_LE_CS_TEST_MAXIMIZE_TX_POWER;
	params.t_ip1_time = 145;
	params.t_ip2_time = 145;
	params.t_fcs_time = 150;
	params.t_pm_time = 40;
	params.t_sw_time = 0;
	params.tone_antenna_config_selection = BT_LE_CS_TONE_ANTENNA_CONFIGURATION_A1_B1;

	params.initiator_snr_control = BT_LE_CS_SNR_CONTROL_NOT_USED;
	params.reflector_snr_control = BT_LE_CS_SNR_CONTROL_NOT_USED;

	params.drbg_nonce = 0x1234;

	params.override_config = BIT(2) | BIT(5);
	params.override_config_0.channel_map_repetition = 1;

	memset(params.override_config_0.not_set.channel_map, 0, 10);

	for (uint8_t i = 40; i < 75; i++) {
		BT_LE_CS_CHANNEL_BIT_SET_VAL(params.override_config_0.not_set.channel_map, i, 1);
	}

	params.override_config_0.not_set.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B;
	params.override_config_0.not_set.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT;
	params.override_config_0.not_set.ch3c_jump = 2;
	params.override_config_2.main_mode_steps = 8;
	params.override_config_5.cs_sync_aa_initiator = INITIATOR_ACCESS_ADDRESS;
	params.override_config_5.cs_sync_aa_reflector = REFLECTOR_ACCESS_ADDRESS;

	return params;
}
