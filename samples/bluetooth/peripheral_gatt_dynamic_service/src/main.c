/*
 * Copyright (c) 2025 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gatt_dynamic_service, CONFIG_LOG_DEFAULT_LEVEL);

#define BT_UUID_CUSTOM_SERVICE_VAL                                                                 \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

static const struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);

static const struct bt_uuid_128 custom_characteristic_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

#define BT_UUID_DYNAMIC_SERVICE_VAL                                                                \
	BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x56789abcdef0)

static const struct bt_uuid_128 dynamic_service_uuid =
	BT_UUID_INIT_128(BT_UUID_DYNAMIC_SERVICE_VAL);

static const struct bt_uuid_128 dynamic_characteristic_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x56789abcdef1));

#define CUSTOM_VALUE_MAX_LEN 20
static uint8_t custom_value[CUSTOM_VALUE_MAX_LEN + 1] = "Hello BLE";
static uint8_t dynamic_value[CUSTOM_VALUE_MAX_LEN + 1] = "Dynamic BLE";

static struct bt_conn *default_conn;

enum bt_sample_adv_evt {
	BT_SAMPLE_EVT_CONNECTED,
	BT_SAMPLE_EVT_DISCONNECTED,
	BT_SAMPLE_EVT_MAX,
};

enum bt_sample_adv_st {
	BT_SAMPLE_ST_ADV,
	BT_SAMPLE_ST_CONNECTED,
};

static ATOMIC_DEFINE(evt_bitmask, BT_SAMPLE_EVT_MAX);
static volatile enum bt_sample_adv_st app_st = BT_SAMPLE_ST_ADV;
static struct k_poll_signal poll_sig = K_POLL_SIGNAL_INITIALIZER(poll_sig);
static struct k_poll_event poll_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &poll_sig);

static ssize_t read_custom_value(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

static ssize_t write_custom_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > CUSTOM_VALUE_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	value[offset + len] = 0;

	LOG_INF("Written value: %s", value);
	return len;
}

BT_GATT_SERVICE_DEFINE(custom_svc, BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
		       BT_GATT_CHARACTERISTIC(&custom_characteristic_uuid.uuid,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					      read_custom_value, write_custom_value, custom_value));

static struct bt_gatt_attr dynamic_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&dynamic_service_uuid),
	BT_GATT_CHARACTERISTIC(&dynamic_characteristic_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_custom_value,
			       write_custom_value, dynamic_value),
};

static struct bt_gatt_service dynamic_svc = {
	.attrs = dynamic_attrs,
	.attr_count = ARRAY_SIZE(dynamic_attrs),
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void raise_evt(enum bt_sample_adv_evt evt)
{
	(void)atomic_set_bit(evt_bitmask, evt);
	k_poll_signal_raise(poll_evt.signal, 1);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
		return;
	}

	LOG_INF("Connected");
	__ASSERT(!default_conn, "Attempting to override existing connection object!");
	default_conn = bt_conn_ref(conn);
	raise_evt(BT_SAMPLE_EVT_CONNECTED);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);
	__ASSERT(conn == default_conn, "Unexpected disconnected callback");
	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static void recycled_cb(void)
{
	LOG_INF("Connection object recycled. Disconnect complete!");
	raise_evt(BT_SAMPLE_EVT_DISCONNECTED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.recycled = recycled_cb,
};

static int register_dynamic_service(void)
{
	int err;

	err = bt_gatt_service_register(&dynamic_svc);
	if (err) {
		LOG_ERR("Dynamic service registration failed (err %d)", err);
		return err;
	}

	LOG_INF("Dynamic service registered successfully");
	return 0;
}

static int unregister_dynamic_service(void)
{
	int err;

	err = bt_gatt_service_unregister(&dynamic_svc);
	if (err) {
		LOG_ERR("Dynamic service unregistration failed (err %d)", err);
		return err;
	}

	LOG_INF("Dynamic service unregistered successfully");
	return 0;
}

static int start_advertising(void)
{
	int err;

	LOG_INF("Starting Advertising");
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Failed to start advertising (err %d)", err);
	}

	return err;
}

int main(void)
{
	int err;
	int toggle = 1;
	struct k_timer toggle_timer;

	k_timer_init(&toggle_timer, NULL, NULL);
	k_timer_start(&toggle_timer, K_SECONDS(10), K_SECONDS(10));

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

#if IS_ENABLED(CONFIG_SETTINGS)
	err = settings_load();
	if (err) {
		LOG_ERR("Settings loading failed (err %d)", err);
		return err;
	}
#endif

	err = register_dynamic_service();
	if (err) {
		LOG_ERR("Failed to register dynamic service (err %d)", err);
		return err;
	}

	err = start_advertising();
	if (err) {
		return err;
	}

	while (1) {
		k_poll(&poll_evt, 1, K_SECONDS(1));
		k_poll_signal_reset(poll_evt.signal);
		poll_evt.state = K_POLL_STATE_NOT_READY;

		if (k_timer_status_get(&toggle_timer) > 0) {
			if (toggle) {
				err = unregister_dynamic_service();
				if (err) {
					LOG_ERR("Failed to unregister dynamic service");
				}
			} else {
				err = register_dynamic_service();
				if (err) {
					LOG_ERR("Failed to register dynamic service");
				}
			}
			toggle = !toggle;
		}

		if (atomic_test_and_clear_bit(evt_bitmask, BT_SAMPLE_EVT_CONNECTED) &&
		    app_st == BT_SAMPLE_ST_ADV) {
			LOG_INF("Connected state!");
			app_st = BT_SAMPLE_ST_CONNECTED;

			LOG_INF("Starting service toggle timer");
		} else if (atomic_test_and_clear_bit(evt_bitmask, BT_SAMPLE_EVT_DISCONNECTED) &&
			   app_st == BT_SAMPLE_ST_CONNECTED) {
			LOG_INF("Disconnected state! Restarting advertising");
			app_st = BT_SAMPLE_ST_ADV;
			err = start_advertising();
			if (err) {
				return err;
			}
		}
	}

	return 0;
}
