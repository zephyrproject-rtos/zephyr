/*
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ans.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(peripheral_ans, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * Sample loops forever, incrementing number of new and unread notifications. Number of new and
 * unread notifications will overflow and loop back around.
 */
static uint8_t num_unread;
static uint8_t num_new;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ANS_VAL))};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static void start_adv(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err != 0) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started");
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = start_adv,
};

int main(void)
{
	int ret;

	LOG_INF("Sample - Bluetooth Peripheral ANS");

	ret = bt_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable bluetooth: %d", ret);
		return ret;
	}

	start_adv();

	num_unread = 0;
	num_new = 0;

	/* At runtime, enable support for given categories */
	uint16_t new_alert_mask = (1 << BT_ANS_CAT_SIMPLE_ALERT) | (1 << BT_ANS_CAT_HIGH_PRI_ALERT);
	uint16_t unread_mask = 1 << BT_ANS_CAT_SIMPLE_ALERT;

	ret = bt_ans_set_new_alert_support_category(new_alert_mask);
	if (ret != 0) {
		LOG_ERR("Unable to set new alert support category mask! (err: %d)", ret);
	}

	ret = bt_ans_set_unread_support_category(unread_mask);
	if (ret != 0) {
		LOG_ERR("Unable to set unread support category mask! (err: %d)", ret);
	}

	while (true) {
		static const char test_msg[] = "Test Alert!";
		static const char high_pri_msg[] = "Prio Alert!";

		num_new++;

		ret = bt_ans_notify_new_alert(NULL, BT_ANS_CAT_SIMPLE_ALERT, num_new, test_msg);
		if (ret != 0) {
			LOG_ERR("Failed to push new alert! (err: %d)", ret);
		}
		k_sleep(K_SECONDS(1));

		ret = bt_ans_notify_new_alert(NULL, BT_ANS_CAT_HIGH_PRI_ALERT, num_new,
					      high_pri_msg);
		if (ret != 0) {
			LOG_ERR("Failed to push new alert! (err: %d)", ret);
		}
		k_sleep(K_SECONDS(1));

		ret = bt_ans_set_unread_count(NULL, BT_ANS_CAT_SIMPLE_ALERT, num_unread);
		if (ret != 0) {
			LOG_ERR("Failed to push new unread count! (err: %d)", ret);
		}

		num_unread++;

		k_sleep(K_SECONDS(5));
	}

	return 0;
}
