/*
 * Copyright (c) 2023 Victor Chavez
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_ble.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log_ctrl.h>

#define ATT_NOTIFY_SIZE 3
#define LOG_BACKEND_BLE_BUF_SIZE (CONFIG_BT_L2CAP_TX_MTU - ATT_NOTIFY_SIZE)

static uint8_t output_buf[LOG_BACKEND_BLE_BUF_SIZE];
static bool panic_mode;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_BLE_OUTPUT_DEFAULT;
static logger_backend_ble_hook user_hook;
static bool first_enable;
static void *user_ctx;
static struct bt_conn *ble_backend_conn;

/* Forward declarations*/
static const struct log_backend *log_backend_ble_get(void);
static void log_backend_ble_connect(struct bt_conn *conn, uint8_t err);
static void log_backend_ble_disconnect(struct bt_conn *conn, uint8_t reason);

/**
 * @brief Callback for the subscription to the ble logger notification characteristic
 * @details This callback enables/disables automatically the logger when the notification
 *			is subscribed.
 *  @param attr   The attribute that's changed value
 *  @param value  New value
 */
static void log_notify_changed(const struct bt_gatt_attr *attr, uint16_t value);

/** BLE Logger based on the UUIDs for the NRF Connect SDK NUS service
 *	https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.3.0/nrf/libraries/bluetooth_services/services/nus.html
 */
#define NUS_SERVICE_UUID                                                                           \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))

#define LOGGER_TX_SERVICE_UUID                                                                     \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6E400003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))

#define LOGGER_RX_SERVICE_UUID                                                                     \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6E400002, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))

BT_CONN_CB_DEFINE(log_backend_ble) = {
	.connected = log_backend_ble_connect,
	.disconnected = log_backend_ble_disconnect,
	.le_param_req = NULL,
	.le_param_updated = NULL
};

/**
 * @brief BLE Service that represents this backend
 * @note Only transmission characteristic is used. The RX characteristic
 *       is added to make the backend usable with the NRF toolbox app
 *       which expects both characteristics.
 */
BT_GATT_SERVICE_DEFINE(ble_log_svc, BT_GATT_PRIMARY_SERVICE(NUS_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC(LOGGER_TX_SERVICE_UUID, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ, NULL, NULL, NULL),
		       BT_GATT_CCC(log_notify_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(LOGGER_RX_SERVICE_UUID, BT_GATT_CHRC_WRITE, 0,
					      NULL, NULL, NULL),

);

/* Log characteristic attribute is defined after the first attribute (i.e. the service) */
const struct bt_gatt_attr *log_characteristic = &ble_log_svc.attrs[1];

void logger_backend_ble_set_hook(logger_backend_ble_hook hook, void *ctx)
{
	user_hook = hook;
	user_ctx = ctx;
}

static void log_backend_ble_connect(struct bt_conn *conn, uint8_t err)
{
	if (err == 0) {
		ble_backend_conn = conn;
	}
}

static void log_backend_ble_disconnect(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);
	ble_backend_conn = NULL;
}

void log_notify_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	const bool notify_enabled = value == BT_GATT_CCC_NOTIFY;

	if (notify_enabled) {
		if (first_enable == false) {
			first_enable = true;
			log_backend_enable(log_backend_ble_get(), NULL, CONFIG_LOG_MAX_LEVEL);
		} else {
			log_backend_activate(log_backend_ble_get(), NULL);
		}
	} else {
		log_backend_deactivate(log_backend_ble_get());
	}
	if (user_hook != NULL) {
		user_hook(notify_enabled, user_ctx);
	}
}

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	ARG_UNUSED(output_ctx);
	const uint16_t mtu_size = bt_gatt_get_mtu(ble_backend_conn);
	const uint16_t attr_data_len = mtu_size - ATT_NOTIFY_SIZE;
	uint16_t notify_len;

	if (attr_data_len < LOG_BACKEND_BLE_BUF_SIZE) {
		notify_len = attr_data_len;
	} else {
		notify_len = LOG_BACKEND_BLE_BUF_SIZE;
	}

	struct bt_gatt_notify_params notify_param = {
		.uuid = NULL,
		.attr = log_characteristic,
		.data = data,
		.len = notify_len,
		.func = NULL,
		.user_data = NULL,
	#if defined(CONFIG_BT_EATT)
		.chan_opt = BT_ATT_CHAN_OPT_NONE
	#endif
	};

	const int notify_res = bt_gatt_notify_cb(ble_backend_conn, &notify_param);
	/* ignore notification result and continue sending msg*/
	ARG_UNUSED(notify_res);

	return length;
}

LOG_OUTPUT_DEFINE(log_output_ble, line_out, output_buf, sizeof(output_buf));

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	uint32_t flags = LOG_OUTPUT_FLAG_FORMAT_SYSLOG | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (panic_mode) {
		return;
	}

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_ble, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);
	log_format_current = log_type;
	return 0;
}

static void init_ble(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);
	log_backend_deactivate(log_backend_ble_get());
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);
	panic_mode = true;
}

/**
 * @brief Backend ready function for ble logger
 * @details After initialization of the logger, this function avoids
 *          the logger subys to enable it. The logger is enabled automatically
 *          via the notification changed callback.
 * @param backend Logger backend
 * @return Zephyr permission denied
 */
static int backend_ready(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
	return -EACCES;
}

const struct log_backend_api log_backend_ble_api = {.process = process,
						    .dropped = NULL,
						    .panic = panic,
						    .init = init_ble,
						    .is_ready = backend_ready,
						    .format_set = format_set,
						    .notify = NULL};

LOG_BACKEND_DEFINE(log_backend_ble, log_backend_ble_api, true);

const struct log_backend *log_backend_ble_get(void)
{
	return &log_backend_ble;
}
