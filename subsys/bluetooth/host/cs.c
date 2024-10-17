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

struct reassembly_buf_meta_data {
	uint16_t conn_handle;
};

static void clear_on_disconnect(struct bt_conn *conn, uint8_t reason);

NET_BUF_POOL_FIXED_DEFINE(reassembly_buf_pool, CONFIG_BT_CHANNEL_SOUNDING_REASSEMBLY_BUFFER_CNT,
			  CONFIG_BT_CHANNEL_SOUNDING_REASSEMBLY_BUFFER_SIZE,
			  sizeof(struct reassembly_buf_meta_data), NULL);

static sys_slist_t reassembly_bufs = SYS_SLIST_STATIC_INIT(&reassembly_bufs);

struct bt_conn_le_cs_subevent_result reassembled_result;

BT_CONN_CB_DEFINE(cs_conn_callbacks) = {
	.disconnected = clear_on_disconnect,
};

/** @brief Allocates new reassembly buffer identified by the connection handle
 *
 * @param conn_handle Connection handle
 * @return struct net_buf* Reassembly buffer, NULL if allocation fails
 */
static struct net_buf *alloc_reassembly_buf(uint16_t conn_handle)
{
	struct net_buf *buf = net_buf_alloc(&reassembly_buf_pool, K_NO_WAIT);

	if (!buf) {
		LOG_ERR("Failed to allocate new reassembly buffer");
		return NULL;
	}

	struct reassembly_buf_meta_data *buf_meta_data =
		(struct reassembly_buf_meta_data *)buf->user_data;

	buf_meta_data->conn_handle = conn_handle;
	net_buf_slist_put(&reassembly_bufs, buf);

	LOG_DBG("Allocated new reassembly buffer for conn handle %d", conn_handle);
	return buf;
}

/** @brief Frees a reassembly buffer
 *
 * @note Takes the ownership of the pointer and sets it to NULL
 *
 * @param buf Double pointer to reassembly buffer
 */
static void free_reassembly_buf(struct net_buf **buf)
{
	if (!buf) {
		LOG_ERR("NULL double pointer was passed when attempting to free reassembly buffer");
		return;
	}

	if (!(*buf)) {
		LOG_WRN("Attempted double free on reassembly buffer");
		return;
	}

	struct reassembly_buf_meta_data *buf_meta_data =
		(struct reassembly_buf_meta_data *)((*buf)->user_data);

	LOG_DBG("De-allocating reassembly buffer for conn handle %d", buf_meta_data->conn_handle);
	if (!sys_slist_find_and_remove(&reassembly_bufs, &(*buf)->node)) {
		LOG_WRN("The buffer was not in the list");
	}

	net_buf_unref(*buf);
	*buf = NULL;
}

/** @brief Gets the reassembly buffer identified by the connection handle
 *
 * @param conn_handle Connection handle
 * @param allocate Allocates a new reassembly buffer if it's not allocated already
 * @return struct net_buf* Reassembly buffer, NULL if it doesn't exist or failed when allocating new
 */
static struct net_buf *get_reassembly_buf(uint16_t conn_handle, bool allocate)
{
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&reassembly_bufs, node) {
		struct net_buf *buf = CONTAINER_OF(node, struct net_buf, node);
		struct reassembly_buf_meta_data *buf_meta_data =
			(struct reassembly_buf_meta_data *)(buf->user_data);

		if (buf_meta_data->conn_handle == conn_handle) {
			return buf;
		}
	}

	return allocate ? alloc_reassembly_buf(conn_handle) : NULL;
}

/** @brief Adds step data to a reassembly buffer
 *
 * @param reassembly_buf Reassembly buffer
 * @param data Step data
 * @param data_len Step data length
 * @return true if successful, false if there is insufficient space
 */
static bool add_reassembly_data(struct net_buf *reassembly_buf, const uint8_t *data,
				uint16_t data_len)
{
	if (data_len > net_buf_tailroom(reassembly_buf)) {
		LOG_ERR("Not enough reassembly buffer space for subevent result");
		return false;
	}

	net_buf_add_mem(reassembly_buf, data, data_len);
	return true;
}

/** @brief Initializes a reassembly buffer from partial step data
 *
 * @note Upon first call, this function also registers the disconnection callback
 *       to ensure any dangling reassembly buffer is freed
 *
 * @param conn_handle Connection handle
 * @param steps Step data
 * @param step_data_len Step data length
 * @return struct net_buf* Pointer to reassembly buffer, NULL if fails to allocate or insert data
 */
static struct net_buf *start_reassembly(uint16_t conn_handle, const uint8_t *steps,
					uint16_t step_data_len)
{
	struct net_buf *reassembly_buf = get_reassembly_buf(conn_handle, true);

	if (!reassembly_buf) {
		LOG_ERR("No buffer allocated for the result reassembly");
		return NULL;
	}

	if (reassembly_buf->len) {
		LOG_WRN("Over-written incomplete CS subevent results");
	}

	net_buf_reset(reassembly_buf);

	bool success = add_reassembly_data(reassembly_buf, steps, step_data_len);

	return success ? reassembly_buf : NULL;
}

/** @brief Adds more step data to reassembly buffer identified by the connection handle
 *
 * @param conn_handle Connection handle
 * @param steps Step data
 * @param step_data_len Step data length
 * @return struct net_buf* Pointer to reassembly buffer, NULL if fails to insert data
 */
static struct net_buf *continue_reassembly(uint16_t conn_handle, const uint8_t *steps,
					   uint16_t step_data_len)
{
	struct net_buf *reassembly_buf = get_reassembly_buf(conn_handle, false);

	if (!reassembly_buf) {
		LOG_ERR("No reassembly buffer was allocated for this CS procedure, possibly due to "
			"an out-of-order subevent result continue event");
		return NULL;
	}

	if (!reassembly_buf->len) {
		LOG_WRN("Discarded out-of-order partial CS subevent results");
		return NULL;
	}

	if (!step_data_len) {
		return reassembly_buf;
	}

	bool success = add_reassembly_data(reassembly_buf, steps, step_data_len);

	return success ? reassembly_buf : NULL;
}

/**
 * @brief Disconnect callback to clear any dangling reassembly buffer
 *
 * @param conn Connection
 * @param reason Reason
 */
static void clear_on_disconnect(struct bt_conn *conn, uint8_t reason)
{
	struct net_buf *buf = get_reassembly_buf(conn->handle, false);

	if (buf) {
		free_reassembly_buf(&buf);
	}
}

/** @brief Invokes user callback for new subevent results
 *
 * @param conn Connection context, NULL for CS Test subevent results
 * @param p_result Pointer to subevent results
 */
static void invoke_subevent_result_callback(struct bt_conn *conn,
					    struct bt_conn_le_cs_subevent_result *p_result)
{
#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
	if (!conn) {
		cs_test_callbacks.le_cs_test_subevent_data_available(p_result);
	} else
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */
	{
		notify_cs_subevent_result(conn, p_result);
	}
}

/** @brief Resets reassembly results
 *
 */
static void reset_reassembly_results(void)
{
	memset(&reassembled_result, 0, sizeof(struct bt_conn_le_cs_subevent_result));
}

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
	struct bt_conn_le_cs_subevent_result *p_result = &result;
	struct net_buf_simple step_data_buf;
	struct net_buf *reassembly_buf = NULL;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	uint16_t conn_handle = sys_le16_to_cpu(evt->conn_handle);

#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
	if (conn_handle == BT_HCI_LE_CS_TEST_CONN_HANDLE) {
		if (!cs_test_callbacks.le_cs_test_subevent_data_available) {
			LOG_WRN("No callback registered. Discarded subevent results from CS Test.");
			return;
		}
	} else
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */
	{
		conn = bt_conn_lookup_handle(conn_handle, BT_CONN_TYPE_LE);
		if (!conn) {
			LOG_ERR("Unknown connection handle when processing subevent results");
			return;
		}
	}

	if (evt->subevent_done_status != BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_PARTIAL) {
		p_result->step_data_buf = NULL;
		if (evt->num_steps_reported) {
			net_buf_simple_init_with_data(&step_data_buf, evt->steps, buf->len);
			p_result->step_data_buf = &step_data_buf;
		}
	} else {
		if (evt->procedure_done_status != BT_HCI_LE_CS_PROCEDURE_DONE_STATUS_PARTIAL) {
			LOG_WRN("Procedure status is inconsistent with subevent status. Discarding "
				"subevent results");
			goto abort;
		}

		if (!evt->num_steps_reported) {
			LOG_WRN("Discarding partial results without step data");
			goto abort;
		}

		reassembly_buf = start_reassembly(conn_handle, evt->steps, buf->len);
		if (!reassembly_buf) {
			goto abort;
		}

		p_result = &reassembled_result;
		p_result->step_data_buf = (struct net_buf_simple *)&reassembly_buf->data;
	}

	p_result->header.procedure_counter = sys_le16_to_cpu(evt->procedure_counter);
	p_result->header.frequency_compensation = sys_le16_to_cpu(evt->frequency_compensation);
	p_result->header.procedure_done_status = evt->procedure_done_status;
	p_result->header.subevent_done_status = evt->subevent_done_status;
	p_result->header.procedure_abort_reason = evt->procedure_abort_reason;
	p_result->header.subevent_abort_reason = evt->subevent_abort_reason;
	p_result->header.reference_power_level = evt->reference_power_level;
	p_result->header.num_antenna_paths = evt->num_antenna_paths;
	p_result->header.num_steps_reported = evt->num_steps_reported;
	p_result->header.abort_step =
		evt->subevent_done_status == BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_ABORTED ? 0 : 255;

	p_result->header.config_id = 0;
	p_result->header.start_acl_conn_event = 0;
	if (conn) {
		p_result->header.config_id = evt->config_id;
		p_result->header.start_acl_conn_event =
			sys_le16_to_cpu(evt->start_acl_conn_event_counter);
	}

	if (evt->subevent_done_status != BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_PARTIAL) {
		invoke_subevent_result_callback(conn, p_result);
	}

	if (evt->procedure_done_status != BT_CONN_LE_CS_PROCEDURE_INCOMPLETE) {
		/* We can now clear the any reassembly buffer allocated for this procedure,
		 * to avoid code duplication, we're using the abort label to do so
		 */
		goto abort;
	}

	if (conn) {
		bt_conn_unref(conn);
		conn = NULL;
	}

	return;

abort:
	if (conn) {
		bt_conn_unref(conn);
		conn = NULL;
	}

	reassembly_buf = get_reassembly_buf(conn_handle, false);
	if (reassembly_buf) {
		free_reassembly_buf(&reassembly_buf);
	}
}

void bt_hci_le_cs_subevent_result_continue(struct net_buf *buf)
{
	struct bt_conn *conn = NULL;
	struct bt_hci_evt_le_cs_subevent_result_continue *evt;
	struct net_buf *reassembly_buf = NULL;
	uint16_t conn_handle;

	if (buf->len < sizeof(*evt)) {
		LOG_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	conn_handle = sys_le16_to_cpu(evt->conn_handle);

#if defined(CONFIG_BT_CHANNEL_SOUNDING_TEST)
	if (conn_handle == BT_HCI_LE_CS_TEST_CONN_HANDLE) {
		if (!cs_test_callbacks.le_cs_test_subevent_data_available) {
			LOG_WRN("No callback registered. Discarded subevent results from CS Test.");
			return;
		}
	} else
#endif /* CONFIG_BT_CHANNEL_SOUNDING_TEST */
	{
		conn = bt_conn_lookup_handle(conn_handle, BT_CONN_TYPE_LE);
		if (!conn) {
			LOG_ERR("Unknown connection handle when processing subevent results");
			return;
		}
	}

	uint16_t step_data_len = evt->num_steps_reported ? buf->len : 0;

	reassembly_buf = continue_reassembly(conn_handle, evt->steps, step_data_len);
	if (!reassembly_buf) {
		goto abort;
	}

	reassembled_result.header.procedure_done_status = evt->procedure_done_status;
	reassembled_result.header.subevent_done_status = evt->subevent_done_status;
	reassembled_result.header.procedure_abort_reason = evt->procedure_abort_reason;
	reassembled_result.header.subevent_abort_reason = evt->subevent_abort_reason;

	if (evt->num_antenna_paths != reassembled_result.header.num_antenna_paths) {
		LOG_WRN("Received inconsistent number of antenna paths from the controller: %d, "
			"previous number was: %d",
			evt->num_antenna_paths, reassembled_result.header.num_antenna_paths);
	}

	if (evt->subevent_done_status == BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_ABORTED &&
	    reassembled_result.header.num_steps_reported < reassembled_result.header.abort_step) {
		reassembled_result.header.abort_step = reassembled_result.header.num_steps_reported;
	}

	reassembled_result.header.num_steps_reported += evt->num_steps_reported;

	if (evt->subevent_done_status != BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_PARTIAL) {
		invoke_subevent_result_callback(conn, &reassembled_result);
		net_buf_reset(reassembly_buf);
		reset_reassembly_results();
	}

	if (evt->procedure_done_status != BT_HCI_LE_CS_PROCEDURE_DONE_STATUS_PARTIAL) {
		if (evt->subevent_done_status == BT_HCI_LE_CS_SUBEVENT_DONE_STATUS_PARTIAL) {
			LOG_WRN("Procedure status is inconsistent with subevent status. Discarding "
				"subevent results");
			goto abort;
		}

		free_reassembly_buf(&reassembly_buf);
	}

	if (conn) {
		bt_conn_unref(conn);
		conn = NULL;
	}

	return;

abort:
	if (conn) {
		bt_conn_unref(conn);
		conn = NULL;
	}

	if (reassembly_buf) {
		free_reassembly_buf(&reassembly_buf);
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

	struct net_buf *reassembly_buf = get_reassembly_buf(BT_HCI_LE_CS_TEST_CONN_HANDLE, false);

	if (reassembly_buf) {
		LOG_WRN("De-allocating a dangling reassembly buffer");
		free_reassembly_buf(&reassembly_buf);
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
