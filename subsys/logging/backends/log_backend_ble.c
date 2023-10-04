/*
 * Copyright (c) 2023 Victor Chavez
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_ble.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log_ctrl.h>

static uint8_t output_buf[CONFIG_LOG_BACKEND_BLE_BUF_SIZE];
static bool panic_mode;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_BLE_OUTPUT_DEFAULT;
static logger_backend_ble_hook user_hook;
static bool first_enable;
static void *user_ctx;
/* Forward declarations*/
const struct log_backend *log_backend_ble_get(void);

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
	const int notify_res = bt_gatt_notify(NULL, log_characteristic, data, length);

	if (notify_res == 0) {
		return length;
	} else {
		return 0;
	}
}

LOG_OUTPUT_DEFINE(log_output_ble, line_out, output_buf, sizeof(output_buf));

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uint32_t flags = LOG_OUTPUT_FLAG_FORMAT_SYSLOG | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (panic_mode) {
		return;
	}

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_ble, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void init_ble(struct log_backend const *const backend)
{
	log_backend_deactivate(log_backend_ble_get());
}

static void panic(struct log_backend const *const backend)
{
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
