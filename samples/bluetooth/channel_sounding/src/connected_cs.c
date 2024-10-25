/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>

static K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
static K_SEM_DEFINE(sem_config_created, 0, 1);
static K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
static K_SEM_DEFINE(sem_procedure_done, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);
static K_SEM_DEFINE(sem_data_received, 0, 1);

#define CS_CONFIG_ID      0
#define NUM_MODE_0_STEPS  1
#define NAME_LEN          30
#define STEP_DATA_BUF_LEN 512 /* Maximum GATT characteristic length */

void estimate_distance(uint8_t *local_steps, uint16_t local_steps_len, uint8_t *peer_steps,
		       uint16_t peer_steps_len, uint8_t n_ap, enum bt_conn_le_cs_role role);
static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static uint16_t step_data_attr_handle;
static struct bt_conn *connection;
static enum bt_conn_le_cs_role role_selection;
static uint8_t n_ap;
static uint8_t latest_num_steps_reported;
static uint16_t latest_step_data_len;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];
static uint8_t latest_peer_steps[STEP_DATA_BUF_LEN];

static struct bt_uuid_128 step_data_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba8));
static const struct bt_uuid_128 step_data_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba9));
static struct bt_gatt_attr gatt_attributes[] = {
	BT_GATT_PRIMARY_SERVICE(&step_data_svc_uuid),
	BT_GATT_CHARACTERISTIC(&step_data_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
			       on_attr_write_cb, NULL),
};
static struct bt_gatt_service step_data_gatt_service = BT_GATT_SERVICE(gatt_attributes);

static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(latest_local_steps)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (flags & BT_GATT_WRITE_FLAG_EXECUTE) {
		uint8_t *data = (uint8_t *)buf;

		memcpy(latest_peer_steps, &data[offset], len);
		k_sem_give(&sem_data_received);
	}

	__ASSERT(role_selection == BT_CONN_LE_CS_ROLE_INITIATOR, "Unexpected GATT write cb");

	return len;
}

static const char sample_str[] = "CS Sample";
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "CS Sample", sizeof(sample_str) - 1),
};

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result)
{
	latest_num_steps_reported = result->header.num_steps_reported;
	n_ap = result->header.num_antenna_paths;

	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
			latest_step_data_len = result->step_data_buf->len;
		} else {
			printk("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN);
			latest_num_steps_reported = 0;
		}
	}

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		k_sem_give(&sem_procedure_done);
	}
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s (%u)\n", err == 0U ? "success" : "failed", bt_gatt_get_mtu(conn));
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected to %s (err 0x%02X)\n", addr, err);

	__ASSERT(connection == conn, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
	}

	if (role_selection == BT_CONN_LE_CS_ROLE_REFLECTOR) {
		connection = bt_conn_ref(conn);
	}

	static struct bt_gatt_exchange_params mtu_exchange_params = {.func = mtu_exchange_cb};

	err = bt_gatt_exchange_mtu(connection, &mtu_exchange_params);
	if (err) {
		printk("%s: MTU exchange failed (err %d)", __func__, err);
	}

	k_sem_give(&sem_connected);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);

	bt_conn_unref(conn);
	connection = NULL;
}

static void remote_capabilities_cb(struct bt_conn *conn, struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(params);
	printk("CS capability exchange completed.\n");
	k_sem_give(&sem_remote_capabilities_obtained);
}

static void config_created_cb(struct bt_conn *conn, struct bt_conn_le_cs_config *config)
{
	printk("CS config creation complete. ID: %d\n", config->id);
	k_sem_give(&sem_config_created);
}

static void security_enabled_cb(struct bt_conn *conn)
{
	printk("CS security enabled.\n");
	k_sem_give(&sem_cs_security_enabled);
}

static void procedure_enabled_cb(struct bt_conn *conn,
				 struct bt_conn_le_cs_procedure_enable_complete *params)
{
	if (params->state == 1) {
		printk("CS procedures enabled.\n");
	} else {
		printk("CS procedures disabled.\n");
	}
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN] = {};
	int err;

	if (connection) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_data_parse(ad, data_cb, name);

	if (strcmp(name, sample_str)) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	printk("Found device with name %s, connecting...\n", name);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&connection);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	printk("Discovery: attr %p\n", attr);

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	printk("UUID %s\n", str);

	if (!bt_uuid_cmp(chrc->uuid, &step_data_char_uuid.uuid)) {
		step_data_attr_handle = chrc->value_handle;

		printk("Found expected UUID\n");

		k_sem_give(&sem_discovered);
	}

	return BT_GATT_ITER_STOP;
}

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err) {
		printk("Write failed (err %d)\n", err);

		return;
	}

	k_sem_give(&sem_written);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.le_cs_remote_capabilities_available = remote_capabilities_cb,
	.le_cs_config_created = config_created_cb,
	.le_cs_security_enabled = security_enabled_cb,
	.le_cs_procedure_enabled = procedure_enabled_cb,
	.le_cs_subevent_data_available = subevent_result_cb,
};

int main(void)
{
	int err;
	struct bt_gatt_discover_params discover_params;
	struct bt_gatt_write_params write_params;

	console_init();

	printk("Starting Channel Sounding Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	while (true) {
		printk("Choose device role - type i (initiator) or r (reflector): ");

		char input_char = console_getchar();

		printk("\n");

		if (input_char == 'i') {
			printk("Initiator selected.\n");
			role_selection = BT_CONN_LE_CS_ROLE_INITIATOR;
			break;
		} else if (input_char == 'r') {
			printk("Reflector selected.\n");
			role_selection = BT_CONN_LE_CS_ROLE_REFLECTOR;
			break;
		}

		printk("Invalid role.\n");
	}

	if (role_selection == BT_CONN_LE_CS_ROLE_INITIATOR) {
		err = bt_gatt_service_register(&step_data_gatt_service);
		if (err) {
			printk("bt_gatt_service_register() returned err %d\n", err);
			return 0;
		}

		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return 0;
		}
	} else {
		err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
						      BT_GAP_ADV_FAST_INT_MAX_1, NULL),
				      ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return 0;
		}
	}

	k_sem_take(&sem_connected, K_FOREVER);

	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = true,
		.enable_reflector_role = true,
		.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

	err = bt_le_cs_set_default_settings(connection, &default_settings);
	if (err) {
		printk("Failed to configure default CS settings (err %d)\n", err);
	}

	if (role_selection == BT_CONN_LE_CS_ROLE_REFLECTOR) {
		discover_params.uuid = &step_data_char_uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(connection, &discover_params);
		if (err) {
			printk("Discovery failed (err %d)\n", err);
			return 0;
		}

		err = k_sem_take(&sem_discovered, K_SECONDS(10));
		if (err) {
			printk("Timed out during GATT discovery\n");
			return 0;
		}
	}

	if (role_selection == BT_CONN_LE_CS_ROLE_INITIATOR) {
		err = bt_conn_set_security(connection, BT_SECURITY_L2);
		if (err) {
			printk("Failed to encrypt connection (err %d)\n", err);
			return 0;
		}

		err = bt_le_cs_read_remote_supported_capabilities(connection);
		if (err) {
			printk("Failed to exchange CS capabilities (err %d)\n", err);
			return 0;
		}

		k_sem_take(&sem_remote_capabilities_obtained, K_FOREVER);

		struct bt_le_cs_create_config_params config_params = {
			.id = CS_CONFIG_ID,
			.main_mode_type = BT_CONN_LE_CS_MAIN_MODE_2,
			.sub_mode_type = BT_CONN_LE_CS_SUB_MODE_1,
			.min_main_mode_steps = 2,
			.max_main_mode_steps = 10,
			.main_mode_repetition = 0,
			.mode_0_steps = NUM_MODE_0_STEPS,
			.role = role_selection,
			.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY,
			.cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY,
			.channel_map_repetition = 1,
			.channel_selection_type = BT_CONN_LE_CS_CHSEL_TYPE_3B,
			.ch3c_shape = BT_CONN_LE_CS_CH3C_SHAPE_HAT,
			.ch3c_jump = 2,
		};

		bt_le_cs_set_valid_chmap_bits(config_params.channel_map);

		err = bt_le_cs_create_config(connection, &config_params,
					     BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
		if (err) {
			printk("Failed to create CS config (err %d)\n", err);
			return 0;
		}

		k_sem_take(&sem_config_created, K_FOREVER);

		err = bt_le_cs_security_enable(connection);
		if (err) {
			printk("Failed to start CS Security (err %d)\n", err);
			return 0;
		}

		k_sem_take(&sem_cs_security_enabled, K_FOREVER);

		const struct bt_le_cs_set_procedure_parameters_param procedure_params = {
			.config_id = CS_CONFIG_ID,
			.max_procedure_len = 12,
			.min_procedure_interval = 100,
			.max_procedure_interval = 100,
			.max_procedure_count = 0,
			.min_subevent_len = 6750,
			.max_subevent_len = 6750,
			.tone_antenna_config_selection =
				BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE,
			.phy = 0x01,
			.tx_power_delta = 0x80,
			.preferred_peer_antenna = 1,
			.snr_control_initiator = BT_LE_CS_INITIATOR_SNR_CONTROL_NOT_USED,
			.snr_control_reflector = BT_LE_CS_REFLECTOR_SNR_CONTROL_NOT_USED,
		};

		err = bt_le_cs_set_procedure_parameters(connection, &procedure_params);
		if (err) {
			printk("Failed to set procedure parameters (err %d)\n", err);
			return 0;
		}

		struct bt_le_cs_procedure_enable_param params = {
			.config_id = CS_CONFIG_ID,
			.enable = 1,
		};

		err = bt_le_cs_procedure_enable(connection, &params);
		if (err) {
			printk("Failed to enable CS procedures (err %d)\n", err);
			return 0;
		}
	}

	while (true) {
		k_sem_take(&sem_procedure_done, K_FOREVER);

		if (role_selection == BT_CONN_LE_CS_ROLE_REFLECTOR) {
			write_params.func = write_func;
			write_params.handle = step_data_attr_handle;
			write_params.length = STEP_DATA_BUF_LEN;
			write_params.data = &latest_local_steps[0];
			write_params.offset = 0;

			err = bt_gatt_write(connection, &write_params);
			if (err) {
				printk("Write failed (err %d)\n", err);
				return 0;
			}

			err = k_sem_take(&sem_written, K_SECONDS(10));
			if (err) {
				printk("Timed out during GATT write\n");
				return 0;
			}
		}

		if (role_selection == BT_CONN_LE_CS_ROLE_INITIATOR) {
			k_sem_take(&sem_data_received, K_FOREVER);

			estimate_distance(
				latest_local_steps, latest_step_data_len, latest_peer_steps,
				latest_step_data_len -
					NUM_MODE_0_STEPS *
						(sizeof(struct
							bt_hci_le_cs_step_data_mode_0_initiator) -
						 sizeof(struct
							bt_hci_le_cs_step_data_mode_0_reflector)),
				n_ap, role_selection);
		}
	}

	return 0;
}
