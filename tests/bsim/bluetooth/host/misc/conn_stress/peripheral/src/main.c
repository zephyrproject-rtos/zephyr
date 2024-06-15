/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(peripheral, LOG_LEVEL_INF);

#include "bstests.h"
#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"
#include "bs_pc_backchannel.h"
#include "argparse.h"

#define TEST_ROUNDS 10
#define MIN_NOTIFICATIONS 50

#define NOTIFICATION_DATA_PREFIX     "Counter:"
#define NOTIFICATION_DATA_PREFIX_LEN (sizeof(NOTIFICATION_DATA_PREFIX) - 1)

#define CHARACTERISTIC_DATA_MAX_LEN 260
#define NOTIFICATION_DATA_LEN	    MAX(200, (CONFIG_BT_L2CAP_TX_MTU - 4))
BUILD_ASSERT(NOTIFICATION_DATA_LEN <= CHARACTERISTIC_DATA_MAX_LEN);

#define CENTRAL_SERVICE_UUID_VAL                                                                   \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdea0)

#define CENTRAL_CHARACTERISTIC_UUID_VAL                                                            \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdea1)

#define CENTRAL_SERVICE_UUID	    BT_UUID_DECLARE_128(CENTRAL_SERVICE_UUID_VAL)
#define CENTRAL_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(CENTRAL_CHARACTERISTIC_UUID_VAL)

/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));

static struct bt_uuid_128 vnd_enc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

enum {
	CONN_INFO_CONNECTED,
	CONN_INFO_SECURITY_LEVEL_UPDATED,
	CONN_INFO_MTU_EXCHANGED,
	CONN_INFO_DISCOVERING,
	CONN_INFO_SUBSCRIBED,
	/* Total number of flags - must be at the end of the enum */
	CONN_INFO_NUM_FLAGS,
};

struct active_conn_info {
	ATOMIC_DEFINE(flags, CONN_INFO_NUM_FLAGS);
	struct bt_conn *conn_ref;
	atomic_t notify_counter;
	atomic_t tx_notify_counter;
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	struct bt_conn_le_data_len_param le_data_len_param;
#endif
};

static struct active_conn_info conn_info;

static uint32_t rounds;

/* This is outside the conn context since it can remain valid across
 * connections
 */
static uint8_t central_subscription;
static uint8_t tx_data[CHARACTERISTIC_DATA_MAX_LEN];
static uint32_t notification_size; /* TODO: does this _need_ to be set in args? */

static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static const struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);

static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	central_subscription = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(
	vnd_svc, BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL,
			       NULL),
	BT_GATT_CCC(vnd_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("Updated MTU: TX: %d RX: %d bytes", tx, rx);

	if (tx == CONFIG_BT_L2CAP_TX_MTU && rx == CONFIG_BT_L2CAP_TX_MTU) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		atomic_set_bit(conn_info.flags, CONN_INFO_MTU_EXCHANGED);
		LOG_INF("Updating MTU succeeded %s", addr);
	}
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		memset(&conn_info, 0x00, sizeof(struct active_conn_info));
		LOG_ERR("Connection failed (err 0x%02x)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	rounds++;
	conn_info.conn_ref = conn;

	atomic_set(&conn_info.tx_notify_counter, 0);
	atomic_set(&conn_info.notify_counter, 0);
	atomic_set_bit(conn_info.flags, CONN_INFO_CONNECTED);

	LOG_INF("Connection %p established : %s", conn, addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("Disconnected (reason 0x%02x)", reason);

	/* With a lot of devices, it is possible that the central doesn't see
	 * the disconnect packet.
	 */
	bool valid_reason =
		reason == BT_HCI_ERR_LOCALHOST_TERM_CONN ||
		reason == BT_HCI_ERR_CONN_TIMEOUT;

	__ASSERT(valid_reason, "Disconnected (reason 0x%02x)", reason);

	memset(&conn_info, 0x00, sizeof(struct active_conn_info));

	if (rounds >= TEST_ROUNDS) {
		LOG_INF("Number of conn/disconn cycles reached, stopping advertiser...");
		bt_le_adv_stop();

		LOG_INF("Test passed");

		extern enum bst_result_t bst_result;

		bst_result = Passed;
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("LE conn param req: %s int (0x%04x (~%u ms), 0x%04x (~%u ms)) lat %d to %d",
		addr, param->interval_min, (uint32_t)(param->interval_min * 1.25),
		param->interval_max, (uint32_t)(param->interval_max * 1.25), param->latency,
		param->timeout);

	return true;
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Security for %p failed: %s level %u err %d", conn, addr, level, err);
		return;
	}

	LOG_INF("Security for %p changed: %s level %u", conn, addr, level);
	atomic_set_bit(conn_info.flags, CONN_INFO_SECURITY_LEVEL_UPDATED);
}
#endif /* CONFIG_BT_SMP */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */
};

static uint8_t rx_notification(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			       const void *data, uint16_t length)
{
	const char *data_ptr = (const char *)data + NOTIFICATION_DATA_PREFIX_LEN;
	uint32_t received_counter;
	char addr[BT_ADDR_LE_STR_LEN];

	if (!data) {
		LOG_INF("[UNSUBSCRIBED]");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	/* TODO: enable */
	/* __ASSERT_NO_MSG(atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED)); */

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	received_counter = strtoul(data_ptr, NULL, 0);
	LOG_INF("RX %d", received_counter);

	__ASSERT(atomic_get(&conn_info.notify_counter) == received_counter,
		 "expected counter : %u , received counter : %u",
		 atomic_get(&conn_info.notify_counter), received_counter);

	atomic_inc(&conn_info.notify_counter);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	char uuid_str[BT_UUID_STR_LEN];

	if (!attr) {
		LOG_INF("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	bt_uuid_to_str(params->uuid, uuid_str, sizeof(uuid_str));
	LOG_DBG("UUID found : %s", uuid_str);

	LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

	if (discover_params.type == BT_GATT_DISCOVER_PRIMARY) {
		LOG_DBG("Primary Service Found");
		memcpy(&uuid, CENTRAL_CHARACTERISTIC_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err == -ENOMEM) {
			goto nomem;
		}

		__ASSERT(!err, "Discover failed (err %d)", err);

	} else if (discover_params.type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		LOG_DBG("Service Characteristic Found");

		params->uuid = &ccc_uuid.uuid;
		params->start_handle = attr->handle + 2;
		params->type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, params);
		if (err == -ENOMEM) {
			goto nomem;
		}

		__ASSERT(!err, "Discover failed (err %d)", err);

	} else if (atomic_test_and_clear_bit(conn_info.flags, CONN_INFO_DISCOVERING)) {

		subscribe_params.notify = rx_notification;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		LOG_ERR("subscribe");
		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err == -ENOMEM) {
			goto nomem;
		}

		if (err != -EALREADY) {
			__ASSERT(!err, "Subscribe failed (err %d)", err);
		}

		__ASSERT_NO_MSG(!atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED));
		atomic_set_bit(conn_info.flags, CONN_INFO_SUBSCRIBED);

		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
		LOG_INF("[SUBSCRIBED] addr %s", addr);
	}

	return BT_GATT_ITER_STOP;

nomem:
	/* if we're out of buffers or metadata contexts, restart discovery
	 * later.
	 */
	LOG_ERR("out of memory, retry sub later");
	atomic_clear_bit(conn_info.flags, CONN_INFO_DISCOVERING);

	return BT_GATT_ITER_STOP;
}

static void subscribe_to_service(struct bt_conn *conn)
{
	while (!atomic_test_and_set_bit(conn_info.flags, CONN_INFO_DISCOVERING) &&
	       !atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED)) {
		int err;
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		memcpy(&uuid, CENTRAL_SERVICE_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(conn, &discover_params);
		if (err == -ENOMEM) {
			LOG_DBG("out of memory, retry sub later");
			atomic_clear_bit(conn_info.flags, CONN_INFO_DISCOVERING);
		}

		__ASSERT(!err, "Subscribe failed (err %d)", err);

		while (atomic_test_bit(conn_info.flags, CONN_INFO_DISCOVERING) &&
		       !atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED)) {
			k_sleep(K_MSEC(10));
		}
	}
}

void set_tx_payload(uint32_t count)
{
	memset(tx_data, 0x00, sizeof(tx_data));
	snprintk(tx_data, notification_size, "%s%u", NOTIFICATION_DATA_PREFIX, count);
}

void disconnect(void)
{
	/* we should always be the ones doing the disconnecting */
	__ASSERT_NO_MSG(conn_info.conn_ref);

	int err = bt_conn_disconnect(conn_info.conn_ref,
				     BT_HCI_ERR_REMOTE_POWER_OFF);

	if (err) {
		LOG_ERR("Terminating conn failed (err %d)", err);
	}

	/* wait for disconnection callback */
	while (atomic_test_bit(conn_info.flags, CONN_INFO_CONNECTED)) {
		k_sleep(K_MSEC(10));
	}
}

void test_peripheral_main(void)
{
	struct bt_gatt_attr *vnd_attr;
	char name[10];
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_DBG("Bluetooth initialized");

	sprintf(name, "per-%d", get_device_nbr());
	bt_set_name(name);

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, NULL, 0, NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		__ASSERT_NO_MSG(err);
	}
	LOG_INF("Started advertising");

	bt_gatt_cb_register(&gatt_callbacks);

	vnd_attr = bt_gatt_find_by_uuid(vnd_svc.attrs, vnd_svc.attr_count, &vnd_enc_uuid.uuid);

	while (true) {
		LOG_DBG("Waiting for connection from central..");
		while (!atomic_test_bit(conn_info.flags, CONN_INFO_CONNECTED)) {
			k_sleep(K_MSEC(10));
		}

		LOG_DBG("Subscribing to central..");
		subscribe_to_service(conn_info.conn_ref);

		LOG_DBG("Waiting until central subscribes..");
		while (!central_subscription) {
			k_sleep(K_MSEC(10));
		}

		while (!atomic_test_bit(conn_info.flags, CONN_INFO_MTU_EXCHANGED)) {
			k_sleep(K_MSEC(10));
		}

		LOG_INF("Begin sending notifications to central..");
		while (central_subscription &&
		       atomic_test_bit(conn_info.flags, CONN_INFO_CONNECTED)) {

			set_tx_payload(atomic_get(&conn_info.tx_notify_counter));
			err = bt_gatt_notify(NULL, vnd_attr, tx_data, notification_size);
			if (err) {
				if (atomic_get(&conn_info.tx_notify_counter) > 0) {
					atomic_dec(&conn_info.tx_notify_counter);
				}
				LOG_DBG("Couldn't send GATT notification");
				k_msleep(10);
			} else {
				atomic_inc(&conn_info.tx_notify_counter);
				LOG_INF("TX %d", atomic_get(&conn_info.tx_notify_counter));
			}

			if (atomic_get(&conn_info.tx_notify_counter) > MIN_NOTIFICATIONS &&
			    atomic_get(&conn_info.notify_counter) > MIN_NOTIFICATIONS) {
				LOG_INF("Disconnecting..");
				disconnect();
			}
		}
	}
}

void test_init(void)
{
	extern enum bst_result_t bst_result;

	LOG_INF("Initializing Test");
	bst_result = Failed;
}

static void test_args(int argc, char **argv)
{
	notification_size = NOTIFICATION_DATA_LEN;

	if (argc >= 1) {
		char const *ptr;

		ptr = strstr(argv[0], "notify_size=");
		if (ptr != NULL) {
			ptr += strlen("notify_size=");
			notification_size = atol(ptr);
			notification_size = MIN(NOTIFICATION_DATA_LEN, notification_size);
		}
	}

	bs_trace_raw(0, "Notification data size : %d\n", notification_size);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral Connection Stress",
		.test_args_f = test_args,
		.test_pre_init_f = test_init,
		.test_main_f = test_peripheral_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_conn_stress_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

extern struct bst_test_list *test_main_conn_stress_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_conn_stress_install,
	NULL
};

int main(void)
{
	bst_main();

	return 0;
}
