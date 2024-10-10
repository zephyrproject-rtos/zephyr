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

static K_SEM_DEFINE(sem_results_available, 0, 1);
static K_SEM_DEFINE(sem_test_complete, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);
static K_SEM_DEFINE(sem_data_received, 0, 1);

#define INITIATOR_ACCESS_ADDRESS 0x4D7B8A2F
#define REFLECTOR_ACCESS_ADDRESS 0x96F93DB1
#define NUM_MODE_0_STEPS 1
#define NO_SAMPLE 0xFF
#define NAME_LEN 30
#define STEP_DATA_BUF_LEN 200
#define MAX_NUM_SAMPLES 200

#define CS_FREQUENCY_MHZ(ch) (2402u + 1u * (ch))
#define SPEED_OF_LIGHT_M_PER_S 299792458.0
#define PI 3.14159265358979323846

static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static uint16_t step_data_attr_handle;
static struct bt_conn *connection;
static enum bt_conn_le_cs_role role_selection;
static uint8_t n_ap;
static uint8_t latest_num_steps_reported;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];
static uint8_t latest_peer_steps[STEP_DATA_BUF_LEN];

struct iq_sample_and_channel {
	uint8_t channel;
	struct bt_le_cs_iq_sample local_iq_sample;
	struct bt_le_cs_iq_sample peer_iq_sample;
};

static struct iq_sample_and_channel samples[MAX_NUM_SAMPLES];

static struct bt_uuid_128 step_data_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba6));
static const struct bt_uuid_128 step_data_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4567, 0x2389, 0x1254, 0xf67f9fedcba7));
static struct bt_gatt_attr gatt_attributes[] = {
	BT_GATT_PRIMARY_SERVICE(&step_data_svc_uuid),
	BT_GATT_CHARACTERISTIC(&step_data_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ, NULL, on_attr_write_cb,
			       (void *)&latest_local_steps),
};
static struct bt_gatt_service step_data_gatt_service = BT_GATT_SERVICE(gatt_attributes);

static void calc_complex_product(int32_t z_a_real, int32_t z_a_imag, int32_t z_b_real,
				 int32_t z_b_imag, int32_t *z_out_real, int32_t *z_out_imag)
{
	*z_out_real = z_a_real * z_b_real - z_a_imag * z_b_imag;
	*z_out_imag = z_a_real * z_b_imag + z_a_imag * z_b_real;
}

static double m_estimate_phase_slope(double *x_values, double *y_values, uint8_t n_samples)
{
	if (n_samples == 0) {
		return 0.0;
	}

	/* Linear regression. Estimates b in y = a + b x */

	double b_est;

	double y_mean = 0;
	double x_mean = 0;

	for (uint8_t i = 0; i < n_samples; i++) {
		y_mean += y_values[i];
		x_mean += x_values[i];
	}

	y_mean /= n_samples;
	x_mean /= n_samples;

	double b_est_upper = 0;
	double b_est_lower = 0;

	for (uint8_t i = 0; i < n_samples; i++) {
		b_est_upper += (x_values[i] - x_mean) * (y_values[i] - y_mean);
		b_est_lower += (x_values[i] - x_mean) * (x_values[i] - x_mean);
	}

	b_est = b_est_upper / b_est_lower;

	return b_est;
}

struct processing_context {
	bool local_steps;
	uint8_t index;
};

static bool process_step_data(struct bt_le_cs_subevent_step *step, void *user_data)
{
	struct processing_context *context = (struct processing_context *)user_data;

	if (step->mode == BT_CONN_LE_CS_MAIN_MODE_2) {
		struct bt_hci_le_cs_step_data_mode_2 *step_data =
			(struct bt_hci_le_cs_step_data_mode_2 *)step->data;

		if (context->local_steps) {
			for (uint8_t i = 0; i < (n_ap + 1); i++) {
				samples[context->index].channel = step->channel;
				samples[context->index].local_iq_sample = bt_le_cs_parse_pct(
					step_data->tone_info[i].phase_correction_term);
				context->index++;
			}
		} else {
			for (uint8_t i = 0; i < (n_ap + 1); i++) {
				samples[context->index].peer_iq_sample = bt_le_cs_parse_pct(
					step_data->tone_info[i].phase_correction_term);
				context->index++;
			}
		}
	}

	return true;
}

static double estimate_distance_using_phase_slope(void)
{
	int32_t combined_i;
	int32_t combined_q;
	struct net_buf_simple buf;

	double theta[MAX_NUM_SAMPLES];
	double frequencies[MAX_NUM_SAMPLES];

	struct processing_context context = {
		.local_steps = true,
		.index = 0,
	};

	printk("Parsing local steps\n");

	net_buf_simple_init_with_data(&buf, &latest_local_steps[1], latest_local_steps[0]);

	bt_le_cs_step_data_parse(&buf, process_step_data, &context);

	context.index = 0;
	context.local_steps = false;

	printk("Parsing peer steps\n");

	net_buf_simple_init_with_data(&buf, &latest_peer_steps[1], latest_peer_steps[0]);

	bt_le_cs_step_data_parse(&buf, process_step_data, &context);

	for (uint8_t i = 0; i < context.index; i++) {
		calc_complex_product(samples[i].local_iq_sample.i, samples[i].local_iq_sample.q,
				     samples[i].peer_iq_sample.i, samples[i].peer_iq_sample.q,
				     &combined_i, &combined_q);

		theta[i] = atan2(1.0 * combined_q, 1.0 * combined_i);

		printk("theta: %f\n", theta[i]);
	}

	for (uint8_t i = 1; i < context.index; i++) {
		if (theta[i] - theta[i - 1] > PI) {
			while (theta[i] - theta[i - 1] > PI) {
				theta[i] -= 2.0 * PI;
			}
		} else if (theta[i - 1] - theta[i] > PI) {
			while (theta[i - 1] - theta[i] > PI) {
				theta[i] += 2.0 * PI;
			}
		}
	}

	for (uint8_t i = 0; i < context.index; i++) {
		printk("unwrapped theta: %f\n", theta[i]);
	}

	for (uint8_t i = 0; i < context.index; i++) {
		frequencies[i] = 1.0 * CS_FREQUENCY_MHZ(samples[i].channel);
	}

	double phase_slope = m_estimate_phase_slope(frequencies, theta, context.index);

	double distance_estimate = -phase_slope * (SPEED_OF_LIGHT_M_PER_S / (4 * PI));

	distance_estimate /= 1000000.0; /* Scale to meters. */

	return distance_estimate;
}

static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(latest_local_steps)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&latest_peer_steps, buf, len);

	printk("Got new step data with %d bytes.\n", ((uint8_t *)buf)[0]);

	__ASSERT(role_selection == BT_CONN_LE_CS_ROLE_INITIATOR, "Unexpected GATT write cb");

	k_sem_give(&sem_data_received);

	return len;
}

static struct bt_le_cs_test_param test_params_get(enum bt_conn_le_cs_role role)
{
	struct bt_le_cs_test_param params;

	params.role = role;
	params.main_mode = BT_CONN_LE_CS_MAIN_MODE_2;
	params.sub_mode = BT_CONN_LE_CS_SUB_MODE_UNUSED;
	params.main_mode_repetition = 1;
	params.mode_0_steps = NUM_MODE_0_STEPS;
	params.rtt_type = BT_CONN_LE_CS_RTT_TYPE_AA_ONLY;
	params.cs_sync_phy = BT_CONN_LE_CS_SYNC_1M_PHY;
	params.cs_sync_antenna_selection = BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_ONE;
	params.subevent_len = 5000;
	params.subevent_interval = 0;
	params.max_num_subevents = 0;
	params.transmit_power_level = BT_HCI_OP_LE_CS_TEST_MAXIMIZE_TX_POWER;
	params.t_ip1_time = 0x91;
	params.t_ip2_time = 0x91;
	params.t_fcs_time = 0x96;
	params.t_pm_time = 0x28;
	params.t_sw_time = 0x0;
	params.tone_antenna_config_selection = BT_LE_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_ONE;

	params.initiator_snr_control = BT_LE_CS_TEST_INITIATOR_SNR_CONTROL_NOT_USED;
	params.reflector_snr_control = BT_LE_CS_TEST_REFLECTOR_SNR_CONTROL_NOT_USED;

	params.drbg_nonce = 0x5678;

	params.override_config = BIT(0) | BIT(5);
	params.override_config_0.channel_map_repetition = 1;

	static uint8_t channels[] = {2, 3, 4, 5, 8, 10, 13, 24, 35, 38, 42, 50, 55, 60, 66};

	params.override_config_0.set.channels = channels;
	params.override_config_0.set.num_channels = ARRAY_SIZE(channels);
	params.override_config_5.cs_sync_aa_initiator = INITIATOR_ACCESS_ADDRESS;
	params.override_config_5.cs_sync_aa_reflector = REFLECTOR_ACCESS_ADDRESS;

	return params;
}

static const char sample_str[] = "CS Test Sample";
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "CS Test Sample", sizeof(sample_str) - 1),
};

static void subevent_result_cb(struct bt_conn_le_cs_subevent_result *result)
{
	printk("Got results with %d steps\n", result->header.num_steps_reported);

	printk("Procedure status = 0x%x, subevent status = 0x%x, abort reasons = 0x%x, 0x%x\n",
	       result->header.procedure_done_status, result->header.subevent_done_status,
	       result->header.procedure_abort_reason, result->header.subevent_abort_reason);

	latest_num_steps_reported = result->header.num_steps_reported;
	n_ap = result->header.num_antenna_paths;

	if (result->step_data_buf) {
		if (result->step_data_buf->len <= (STEP_DATA_BUF_LEN - 1)) {
			latest_local_steps[0] = result->step_data_buf->len;
			memcpy(&latest_local_steps[1], result->step_data_buf->data,
			       result->step_data_buf->len);
		} else {
			printk("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN - 1);
			latest_num_steps_reported = 0;
		}
	}

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE ||
	    result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_ABORTED) {
		k_sem_give(&sem_results_available);
	}
}

static void end_cb(void)
{
	k_sem_give(&sem_test_complete);
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

	k_sem_give(&sem_disconnected);
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
};

int main(void)
{
	int err;
	struct bt_le_cs_test_param test_params;
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

	struct bt_le_cs_test_cb cs_test_cb = {
		.le_cs_test_subevent_data_available = subevent_result_cb,
		.le_cs_test_end_complete = end_cb,
	};

	err = bt_le_cs_test_cb_register(cs_test_cb);
	if (err) {
		printk("Failed to register callbacks (err %d)\n", err);
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
	}

	while (true) {
		while (true) {
			if (role_selection == BT_CONN_LE_CS_ROLE_INITIATOR) {
				k_sleep(K_SECONDS(2));
			} else {
				k_sleep(K_SECONDS(1));
			}

			test_params = test_params_get(role_selection);

			err = bt_le_cs_start_test(&test_params);
			if (err) {
				printk("Failed to start CS test (err %d)", err);
				return 0;
			}

			k_sem_take(&sem_results_available, K_SECONDS(5));

			err = bt_le_cs_stop_test();
			if (err) {
				printk("Failed to stop CS test (err %d)", err);
				return 0;
			}

			k_sem_take(&sem_test_complete, K_FOREVER);

			if (latest_num_steps_reported > NUM_MODE_0_STEPS) {
				break;
			}
		}

		if (role_selection == BT_CONN_LE_CS_ROLE_INITIATOR) {
			err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, device_found);
			if (err) {
				printk("Scanning failed to start (err %d)\n", err);
				return 0;
			}
		} else {
			err = bt_le_adv_start(
				BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
						BT_GAP_ADV_FAST_INT_MAX_1, NULL),
				ad, ARRAY_SIZE(ad), NULL, 0);
			if (err) {
				printk("Advertising failed to start (err %d)\n", err);
				return 0;
			}
		}

		k_sem_take(&sem_connected, K_FOREVER);

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

			write_params.func = write_func;
			write_params.handle = step_data_attr_handle;
			write_params.offset = 0;
			write_params.data = &latest_local_steps;
			write_params.length = sizeof(latest_local_steps);

			err = bt_gatt_write(connection, &write_params);
			if (err) {
				printk("Write failed (err %d)\n", err);
				return 0;
			}
		}

		if (role_selection == BT_CONN_LE_CS_ROLE_INITIATOR) {
			k_sem_take(&sem_data_received, K_FOREVER);

			double distance = estimate_distance_using_phase_slope();

			printk("Estimated distance to reflector: %f meters\n", distance);

			bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}

		k_sem_take(&sem_disconnected, K_FOREVER);

		printk("Re-running CS test...\n");
	}

	return 0;
}
