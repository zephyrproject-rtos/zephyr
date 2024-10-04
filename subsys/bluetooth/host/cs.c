/* cs.c - Bluetooth Channel Sounding handling */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>

#include "conn_internal.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_cs);

#if defined(CONFIG_BT_CHANNEL_SOUNDING)
#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
static struct bt_le_cs_test_cb cs_test_callbacks;
#endif

void bt_le_cs_set_valid_chmap_bits(uint8_t channel_map[10])
{
	memset(channel_map, 0xFF, 10);

	/** Channels n = 0, 1, 23, 24, 25, 77, and 78 are not allowed and shall be set to zero.
	 *  Channel 79 is reserved for future use and shall be set to zero.
	 */
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 0, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 1, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 23, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 24, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 25, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 77, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 78, 0);
	BT_LE_CS_CHANNEL_BIT_SET_VAL(channel_map, 79, 0);
}

int bt_le_cs_read_remote_supported_capabilities(struct bt_conn *conn)
{
	struct bt_hci_cp_le_read_remote_supported_capabilities *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES, buf, NULL);
}

void bt_hci_le_cs_read_remote_supported_capabilities_complete(struct net_buf *buf)
{
	struct bt_conn *conn;
	struct bt_conn_le_cs_capabilities remote_cs_capabilities;
	struct bt_hci_evt_le_cs_read_remote_supported_capabilities_complete *evt;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	if (evt->status) {
		LOG_INF("Read Remote Supported Capabilities failed (status 0x%02X)", evt->status);
		return;
	}

	conn = bt_conn_lookup_handle(sys_le16_to_cpu(evt->conn_handle), BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Could not lookup connection handle when reading remote CS capabilities");
		return;
	}

	remote_cs_capabilities.num_config_supported = evt->num_config_supported;
	remote_cs_capabilities.max_consecutive_procedures_supported =
		sys_le16_to_cpu(evt->max_consecutive_procedures_supported);
	remote_cs_capabilities.num_antennas_supported = evt->num_antennas_supported;
	remote_cs_capabilities.max_antenna_paths_supported = evt->max_antenna_paths_supported;

	remote_cs_capabilities.initiator_supported =
		evt->roles_supported & BT_HCI_LE_CS_INITIATOR_ROLE_MASK;
	remote_cs_capabilities.reflector_supported =
		evt->roles_supported & BT_HCI_LE_CS_REFLECTOR_ROLE_MASK;
	remote_cs_capabilities.mode_3_supported =
		evt->modes_supported & BT_HCI_LE_CS_MODES_SUPPORTED_MODE_3_MASK;

	remote_cs_capabilities.rtt_aa_only_n = evt->rtt_aa_only_n;
	remote_cs_capabilities.rtt_sounding_n = evt->rtt_sounding_n;
	remote_cs_capabilities.rtt_random_payload_n = evt->rtt_random_payload_n;

	if (evt->rtt_aa_only_n) {
		if (evt->rtt_capability & BT_HCI_LE_CS_RTT_AA_ONLY_N_10NS_MASK) {
			remote_cs_capabilities.rtt_aa_only_precision =
				BT_CONN_LE_CS_RTT_AA_ONLY_10NS;
		} else {
			remote_cs_capabilities.rtt_aa_only_precision =
				BT_CONN_LE_CS_RTT_AA_ONLY_150NS;
		}
	} else {
		remote_cs_capabilities.rtt_aa_only_precision = BT_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP;
	}

	if (evt->rtt_sounding_n) {
		if (evt->rtt_capability & BT_HCI_LE_CS_RTT_SOUNDING_N_10NS_MASK) {
			remote_cs_capabilities.rtt_sounding_precision =
				BT_CONN_LE_CS_RTT_SOUNDING_10NS;
		} else {
			remote_cs_capabilities.rtt_sounding_precision =
				BT_CONN_LE_CS_RTT_SOUNDING_150NS;
		}
	} else {
		remote_cs_capabilities.rtt_sounding_precision = BT_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP;
	}

	if (evt->rtt_random_payload_n) {
		if (evt->rtt_capability & BT_HCI_LE_CS_RTT_RANDOM_PAYLOAD_N_10NS_MASK) {
			remote_cs_capabilities.rtt_random_payload_precision =
				BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS;
		} else {
			remote_cs_capabilities.rtt_random_payload_precision =
				BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS;
		}
	} else {
		remote_cs_capabilities.rtt_random_payload_precision =
			BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP;
	}

	remote_cs_capabilities.phase_based_nadm_sounding_supported =
		sys_le16_to_cpu(evt->nadm_sounding_capability) &
		BT_HCI_LE_CS_NADM_SOUNDING_CAPABILITY_PHASE_BASED_MASK;

	remote_cs_capabilities.phase_based_nadm_random_supported =
		sys_le16_to_cpu(evt->nadm_random_capability) &
		BT_HCI_LE_CS_NADM_RANDOM_CAPABILITY_PHASE_BASED_MASK;

	remote_cs_capabilities.cs_sync_2m_phy_supported =
		evt->cs_sync_phys_supported & BT_HCI_LE_CS_SYNC_PHYS_2M_MASK;

	remote_cs_capabilities.cs_sync_2m_2bt_phy_supported =
		evt->cs_sync_phys_supported & BT_HCI_LE_CS_SYNC_PHYS_2M_2BT_MASK;

	remote_cs_capabilities.cs_without_fae_supported =
		sys_le16_to_cpu(evt->subfeatures_supported) &
		BT_HCI_LE_CS_SUBFEATURE_NO_TX_FAE_MASK;

	remote_cs_capabilities.chsel_alg_3c_supported =
		sys_le16_to_cpu(evt->subfeatures_supported) &
		BT_HCI_LE_CS_SUBFEATURE_CHSEL_ALG_3C_MASK;

	remote_cs_capabilities.pbr_from_rtt_sounding_seq_supported =
		sys_le16_to_cpu(evt->subfeatures_supported) &
		BT_HCI_LE_CS_SUBFEATURE_PBR_FROM_RTT_SOUNDING_SEQ_MASK;

	remote_cs_capabilities.t_ip1_times_supported = sys_le16_to_cpu(evt->t_ip1_times_supported);
	remote_cs_capabilities.t_ip2_times_supported = sys_le16_to_cpu(evt->t_ip2_times_supported);
	remote_cs_capabilities.t_fcs_times_supported = sys_le16_to_cpu(evt->t_fcs_times_supported);
	remote_cs_capabilities.t_pm_times_supported = sys_le16_to_cpu(evt->t_pm_times_supported);

	remote_cs_capabilities.t_sw_time = evt->t_sw_time_supported;
	remote_cs_capabilities.tx_snr_capability = evt->tx_snr_capability;

	notify_remote_cs_capabilities(conn, remote_cs_capabilities);

	bt_conn_unref(conn);
}

int bt_le_cs_set_default_settings(struct bt_conn *conn,
				  const struct bt_le_cs_set_default_settings_param *params)
{
	struct bt_hci_cp_le_cs_set_default_settings *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_SET_DEFAULT_SETTINGS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->max_tx_power = params->max_tx_power;
	cp->cs_sync_antenna_selection = params->cs_sync_antenna_selection;
	cp->role_enable = 0;

	if (params->enable_initiator_role) {
		cp->role_enable |= BT_HCI_OP_LE_CS_INITIATOR_ROLE_MASK;
	}

	if (params->enable_reflector_role) {
		cp->role_enable |= BT_HCI_OP_LE_CS_REFLECTOR_ROLE_MASK;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_SET_DEFAULT_SETTINGS, buf, NULL);
}

int bt_le_cs_read_remote_fae_table(struct bt_conn *conn)
{
	struct bt_hci_cp_le_read_remote_fae_table *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_READ_REMOTE_FAE_TABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_READ_REMOTE_FAE_TABLE, buf, NULL);
}

void bt_hci_le_cs_read_remote_fae_table_complete(struct net_buf *buf)
{
	struct bt_conn *conn;
	struct bt_conn_le_cs_fae_table fae_table;
	struct bt_hci_evt_le_cs_read_remote_fae_table_complete *evt;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	if (evt->status) {
		LOG_INF("Read Remote FAE Table failed with status 0x%02X", evt->status);
		return;
	}

	conn = bt_conn_lookup_handle(sys_le16_to_cpu(evt->conn_handle), BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Could not lookup connection handle when reading remote FAE Table");
		return;
	}

	fae_table.remote_fae_table = evt->remote_fae_table;
	notify_remote_cs_fae_table(conn, fae_table);

	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
int bt_le_cs_test_cb_register(struct bt_le_cs_test_cb cb)
{
	cs_test_callbacks = cb;
	return 0;
}

int bt_le_cs_start_test(const struct bt_le_cs_test_param *params)
{
	struct bt_hci_op_le_cs_test *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_TEST, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	cp->main_mode_type = params->main_mode;
	cp->sub_mode_type = params->sub_mode;
	cp->main_mode_repetition = params->main_mode_repetition;
	cp->mode_0_steps = params->mode_0_steps;
	cp->role = params->role;
	cp->rtt_type = params->rtt_type;
	cp->cs_sync_phy = params->cs_sync_phy;
	cp->cs_sync_antenna_selection = params->cs_sync_antenna_selection;
	sys_put_le24(params->subevent_len, cp->subevent_len);
	cp->subevent_interval = sys_cpu_to_le16(params->subevent_interval);
	cp->max_num_subevents = params->max_num_subevents;
	cp->transmit_power_level = params->transmit_power_level;
	cp->t_ip1_time = params->t_ip1_time;
	cp->t_ip2_time = params->t_ip2_time;
	cp->t_fcs_time = params->t_fcs_time;
	cp->t_pm_time = params->t_pm_time;
	cp->t_sw_time = params->t_sw_time;
	cp->tone_antenna_config_selection = params->tone_antenna_config_selection;

	cp->reserved = 0;

	cp->snr_control_initiator = params->initiator_snr_control;
	cp->snr_control_reflector = params->reflector_snr_control;
	cp->drbg_nonce = sys_cpu_to_le16(params->drbg_nonce);
	cp->channel_map_repetition = params->override_config_0.channel_map_repetition;
	cp->override_config = sys_cpu_to_le16(params->override_config);

	uint8_t override_parameters_length = 0;

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_0_MASK) {
		const uint8_t num_channels = params->override_config_0.set.num_channels;

		net_buf_add_u8(buf, num_channels);
		override_parameters_length++;
		net_buf_add_mem(buf, params->override_config_0.set.channels, num_channels);
		override_parameters_length += num_channels;
	} else {
		net_buf_add_mem(buf, params->override_config_0.not_set.channel_map,
				sizeof(params->override_config_0.not_set.channel_map));
		net_buf_add_u8(buf, params->override_config_0.not_set.channel_selection_type);
		net_buf_add_u8(buf, params->override_config_0.not_set.ch3c_shape);
		net_buf_add_u8(buf, params->override_config_0.not_set.ch3c_jump);

		override_parameters_length +=
			(sizeof(params->override_config_0.not_set.channel_map) +
			 sizeof(params->override_config_0.not_set.channel_selection_type) +
			 sizeof(params->override_config_0.not_set.ch3c_shape) +
			 sizeof(params->override_config_0.not_set.ch3c_jump));
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_2_MASK) {
		net_buf_add_mem(buf, &params->override_config_2, sizeof(params->override_config_2));
		override_parameters_length += sizeof(params->override_config_2);
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_3_MASK) {
		net_buf_add_mem(buf, &params->override_config_3, sizeof(params->override_config_3));
		override_parameters_length += sizeof(params->override_config_3);
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_4_MASK) {
		net_buf_add_mem(buf, &params->override_config_4, sizeof(params->override_config_4));
		override_parameters_length += sizeof(params->override_config_4);
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_5_MASK) {
		net_buf_add_le32(buf, params->override_config_5.cs_sync_aa_initiator);
		net_buf_add_le32(buf, params->override_config_5.cs_sync_aa_reflector);
		override_parameters_length += sizeof(params->override_config_5);
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_6_MASK) {
		net_buf_add_mem(buf, &params->override_config_6, sizeof(params->override_config_6));
		override_parameters_length += sizeof(params->override_config_6);
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_7_MASK) {
		net_buf_add_mem(buf, &params->override_config_7, sizeof(params->override_config_7));
		override_parameters_length += sizeof(params->override_config_7);
	}

	if (params->override_config & BT_HCI_OP_LE_CS_TEST_OVERRIDE_CONFIG_8_MASK) {
		net_buf_add_mem(buf, &params->override_config_8, sizeof(params->override_config_8));
		override_parameters_length += sizeof(params->override_config_8);
	}

	cp->override_parameters_length = override_parameters_length;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_TEST, buf, NULL);
}
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */

void bt_hci_le_cs_subevent_result(struct net_buf *buf)
{
	struct bt_conn *conn = NULL;
	struct bt_hci_evt_le_cs_subevent_result *evt;
	struct bt_conn_le_cs_subevent_result result;
	struct net_buf_simple step_data_buf;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	if (evt->subevent_done_status == BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_PARTIAL) {
		LOG_WRN("Discarded incomplete CS subevent results.");
		return;
	}

#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
	if (sys_le16_to_cpu(evt->conn_handle) == BT_HCI_LE_CS_TEST_CONN_HANDLE) {
		if (!cs_test_callbacks.le_cs_test_subevent_data_available) {
			LOG_WRN("No callback registered. Discarded subevent results from CS Test.");
			return;
		}
	} else
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */
	{
		conn = bt_conn_lookup_handle(sys_le16_to_cpu(evt->conn_handle), BT_CONN_TYPE_LE);
		if (!conn) {
			LOG_ERR("Unknown connection handle when processing subevent results");
			return;
		}
	}

	result.header.procedure_counter = sys_le16_to_cpu(evt->procedure_counter);
	result.header.frequency_compensation = sys_le16_to_cpu(evt->frequency_compensation);
	result.header.procedure_done_status = evt->procedure_done_status;
	result.header.subevent_done_status = evt->subevent_done_status;
	result.header.procedure_abort_reason = evt->procedure_abort_reason;
	result.header.subevent_abort_reason = evt->subevent_abort_reason;
	result.header.reference_power_level = evt->reference_power_level;
	result.header.num_antenna_paths = evt->num_antenna_paths;
	result.header.num_steps_reported = evt->num_steps_reported;

	if (evt->num_steps_reported) {
		net_buf_simple_init_with_data(&step_data_buf,
					      evt->steps,
					      buf->len);
		result.step_data_buf = &step_data_buf;
	} else {
		result.step_data_buf = NULL;
	}

#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
	if (sys_le16_to_cpu(evt->conn_handle) == BT_HCI_LE_CS_TEST_CONN_HANDLE) {
		result.header.config_id = 0;
		result.header.start_acl_conn_event = 0;

		cs_test_callbacks.le_cs_test_subevent_data_available(&result);

	} else
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */
	{
		result.header.config_id = evt->config_id;
		result.header.start_acl_conn_event =
			sys_le16_to_cpu(evt->start_acl_conn_event_counter);

		notify_cs_subevent_result(conn, &result);

		bt_conn_unref(conn);
	}
}

void bt_hci_le_cs_config_complete_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_cs_config_complete *evt;
	struct bt_conn_le_cs_config config;
	struct bt_conn *conn;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	if (evt->status) {
		LOG_INF("CS Config failed (status 0x%02X)", evt->status);
		return;
	}

	conn = bt_conn_lookup_handle(sys_le16_to_cpu(evt->handle), BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Could not lookup connection handle when reading CS configuration");
		return;
	}

	if (evt->action == BT_HCI_LE_CS_CONFIG_ACTION_REMOVED) {
		notify_cs_config_removed(conn, evt->config_id);
		bt_conn_unref(conn);
		return;
	}

	config.id = evt->config_id;
	config.main_mode_type = evt->main_mode_type;
	config.sub_mode_type = evt->sub_mode_type;
	config.min_main_mode_steps = evt->min_main_mode_steps;
	config.max_main_mode_steps = evt->max_main_mode_steps;
	config.main_mode_repetition = evt->main_mode_repetition;
	config.mode_0_steps = evt->mode_0_steps;
	config.role = evt->role;
	config.rtt_type = evt->rtt_type;
	config.cs_sync_phy = evt->cs_sync_phy;
	config.channel_map_repetition = evt->channel_map_repetition;
	config.channel_selection_type = evt->channel_selection_type;
	config.ch3c_shape = evt->ch3c_shape;
	config.ch3c_jump = evt->ch3c_jump;
	config.t_ip1_time_us = evt->t_ip1_time;
	config.t_ip2_time_us = evt->t_ip2_time;
	config.t_fcs_time_us = evt->t_fcs_time;
	config.t_pm_time_us = evt->t_pm_time;
	memcpy(config.channel_map, evt->channel_map, ARRAY_SIZE(config.channel_map));

	notify_cs_config_created(conn, &config);
	bt_conn_unref(conn);
}

int bt_le_cs_create_config(struct bt_conn *conn, struct bt_le_cs_create_config_params *params,
			   enum bt_le_cs_create_config_context context)
{
	struct bt_hci_cp_le_cs_create_config *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_CREATE_CONFIG, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->config_id = params->id;
	cp->create_context = context;
	cp->main_mode_type = params->main_mode_type;
	cp->sub_mode_type = params->sub_mode_type;
	cp->min_main_mode_steps = params->min_main_mode_steps;
	cp->max_main_mode_steps = params->max_main_mode_steps;
	cp->main_mode_repetition = params->main_mode_repetition;
	cp->mode_0_steps = params->mode_0_steps;
	cp->role = params->role;
	cp->rtt_type = params->rtt_type;
	cp->cs_sync_phy = params->cs_sync_phy;
	cp->channel_map_repetition = params->channel_map_repetition;
	cp->channel_selection_type = params->channel_selection_type;
	cp->ch3c_shape = params->ch3c_shape;
	cp->ch3c_jump = params->ch3c_jump;
	cp->reserved = 0;
	memcpy(cp->channel_map, params->channel_map, ARRAY_SIZE(cp->channel_map));

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_CREATE_CONFIG, buf, NULL);
}

int bt_le_cs_remove_config(struct bt_conn *conn, uint8_t config_id)
{
	struct bt_hci_cp_le_cs_remove_config *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_REMOVE_CONFIG, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->config_id = config_id;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_REMOVE_CONFIG, buf, NULL);
}

#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
int bt_le_cs_stop_test(void)
{
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_TEST_END, 0);
	if (!buf) {
		return -ENOBUFS;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_TEST_END, buf, NULL);
}

void bt_hci_le_cs_test_end_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_cs_test_end_complete *evt;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	if (evt->status) {
		LOG_INF("CS Test End failed with status 0x%02X", evt->status);
		return;
	}

	if (cs_test_callbacks.le_cs_test_end_complete) {
		cs_test_callbacks.le_cs_test_end_complete();
	}
}
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */

void bt_le_cs_step_data_parse(struct net_buf_simple *step_data_buf,
			      bool (*func)(struct bt_le_cs_subevent_step *step, void *user_data),
			      void *user_data)
{
	if (!step_data_buf) {
		LOG_INF("Tried to parse empty step data.");
		return;
	}

	while (step_data_buf->len > 1) {
		struct bt_le_cs_subevent_step step;

		step.mode = net_buf_simple_pull_u8(step_data_buf);
		step.channel = net_buf_simple_pull_u8(step_data_buf);
		step.data_len = net_buf_simple_pull_u8(step_data_buf);

		if (step.data_len == 0) {
			LOG_WRN("Encountered zero-length step data.");
			return;
		}

		step.data = step_data_buf->data;

		if (!func(&step, user_data)) {
			return;
		}

		net_buf_simple_pull(step_data_buf, step.data_len);
	}
}

#endif /* CONFIG_BT_CHANNEL_SOUNDING */
