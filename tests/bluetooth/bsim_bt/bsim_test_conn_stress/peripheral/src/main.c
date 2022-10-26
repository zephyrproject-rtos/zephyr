/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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

#define TERM_PRINT(fmt, ...)   printk("\e[39m[Peripheral] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_INFO(fmt, ...)    printk("\e[94m[Peripheral] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_SUCCESS(fmt, ...) printk("\e[92m[Peripheral] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_ERR(fmt, ...)                                                                         \
	printk("\e[91m[Peripheral] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)
#define TERM_WARN(fmt, ...)                                                                        \
	printk("\e[93m[Peripheral] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)

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
	CONN_INFO_SECURITY_LEVEL_UPDATED,
	CONN_INFO_CONN_PARAMS_UPDATED,
	CONN_INFO_LL_DATA_LEN_TX_UPDATED,
	CONN_INFO_LL_DATA_LEN_RX_UPDATED,
	CONN_INFO_MTU_EXCHANGED,
	CONN_INFO_SUBSCRIBED_TO_SERVICE,
	/* Total number of flags - must be at the end of the enum */
	CONN_INFO_NUM_FLAGS,
};

struct active_conn_info {
	ATOMIC_DEFINE(flags, CONN_INFO_NUM_FLAGS);
	struct bt_conn *conn_ref;
	uint32_t notify_counter;
};

static uint8_t simulate_vnd;
static int64_t uptime_ref;
static uint32_t tx_notify_counter;
static struct active_conn_info conn_info;
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static struct bt_conn_le_data_len_param le_data_len_param;
#endif
static uint8_t vnd_value[CHARACTERISTIC_DATA_MAX_LEN];

static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	simulate_vnd = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	if (simulate_vnd) {
		tx_notify_counter = 0;
		uptime_ref = k_uptime_get();
	}
}

/* Vendor Primary Service Declaration */
#if defined(CONFIG_BT_SMP)
BT_GATT_SERVICE_DEFINE(
	vnd_svc, BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, NULL, NULL,
			       NULL),
	BT_GATT_CCC(vnd_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
);
#else
BT_GATT_SERVICE_DEFINE(
	vnd_svc, BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL,
			       NULL),
	BT_GATT_CCC(vnd_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);
#endif

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	TERM_INFO("Updated MTU: TX: %d RX: %d bytes", tx, rx);

	if (tx == CONFIG_BT_L2CAP_TX_MTU && rx == CONFIG_BT_L2CAP_TX_MTU) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		atomic_set_bit(conn_info.flags, CONN_INFO_MTU_EXCHANGED);
		TERM_SUCCESS("Updating MTU succeeded %s", addr);
	}
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		memset(&conn_info, 0x00, sizeof(struct active_conn_info));
		TERM_ERR("Connection failed (err 0x%02x)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	conn_info.conn_ref = conn;
	TERM_SUCCESS("Connection %p established : %s", conn, addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	memset(&conn_info, 0x00, sizeof(struct active_conn_info));
	TERM_ERR("Disconnected (reason 0x%02x)", reason);
	__ASSERT(reason == BT_HCI_ERR_LOCALHOST_TERM_CONN, "Disconnected (reason 0x%02x)", reason);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_PRINT("LE conn param req: %s int (0x%04x (~%u ms), 0x%04x (~%u ms)) lat %d to %d",
		   addr, param->interval_min, (uint32_t)(param->interval_min * 1.25),
		   param->interval_max, (uint32_t)(param->interval_max * 1.25), param->latency,
		   param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_INFO("LE conn param updated: %s int 0x%04x (~%u ms) lat %d to %d", addr, interval,
		  (uint32_t)(interval * 1.25), latency, timeout);

	atomic_set_bit(conn_info.flags, CONN_INFO_CONN_PARAMS_UPDATED);
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		TERM_ERR("Security for %p failed: %s level %u err %d", conn, addr, level, err);
		return;
	}

	TERM_INFO("Security for %p changed: %s level %u", conn, addr, level);
	atomic_set_bit(conn_info.flags, CONN_INFO_SECURITY_LEVEL_UPDATED);
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_PRINT("Data length updated: %s max tx %u (%u us) max rx %u (%u us)", addr,
		   info->tx_max_len, info->tx_max_time, info->rx_max_len, info->rx_max_time);

	if (info->rx_max_len == BT_GAP_DATA_LEN_MAX) {
		TERM_INFO("RX Data length flag updated for %s", addr);
		atomic_set_bit(conn_info.flags, CONN_INFO_LL_DATA_LEN_RX_UPDATED);
	}
	if (info->tx_max_len == BT_GAP_DATA_LEN_MAX) {
		TERM_INFO("TX Data length flag updated for %s", addr);
		atomic_set_bit(conn_info.flags, CONN_INFO_LL_DATA_LEN_TX_UPDATED);
	}
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = le_data_len_updated,
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */
};

static void bt_ready(void)
{
	int err;

	TERM_PRINT("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		TERM_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	TERM_SUCCESS("Advertising successfully started");
}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static uint16_t tx_time_calc(uint8_t phy, uint16_t max_len)
{
	/* Access address + header + payload + MIC + CRC */
	uint16_t total_len = 4 + 2 + max_len + 4 + 3;

	switch (phy) {
	case BT_GAP_LE_PHY_1M:
		/* 1 byte preamble, 8 us per byte */
		return 8 * (1 + total_len);
	case BT_GAP_LE_PHY_2M:
		/* 2 byte preamble, 4 us per byte */
		return 4 * (2 + total_len);
	case BT_GAP_LE_PHY_CODED:
		/* S8: Preamble + CI + TERM1 + 64 us per byte + TERM2 */
		return 80 + 16 + 24 + 64 * (total_len) + 24;
	default:
		return 0;
	}
}

static void update_ll_max_data_length(struct bt_conn *conn, void *data)
{
	if (atomic_test_bit(conn_info.flags, CONN_INFO_LL_DATA_LEN_TX_UPDATED) == false) {
		int err;
		char addr[BT_ADDR_LE_STR_LEN];

		le_data_len_param = *BT_LE_DATA_LEN_PARAM_DEFAULT;

		/* Update LL transmission payload size in bytes*/
		le_data_len_param.tx_max_len = BT_GAP_DATA_LEN_MAX;
		/* Update LL transmission payload time in us*/
		le_data_len_param.tx_max_time =
			tx_time_calc(BT_GAP_LE_PHY_2M, le_data_len_param.tx_max_len);
		TERM_PRINT("Calculated tx time: %d", le_data_len_param.tx_max_time);

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		TERM_PRINT("Updating LL data length for %s...", addr);
		err = bt_conn_le_data_len_update(conn, &le_data_len_param);
		if (err) {
			TERM_ERR("Updating LL data length failed %s", addr);
			return;
		}

		while (atomic_test_bit(conn_info.flags, CONN_INFO_LL_DATA_LEN_TX_UPDATED) ==
		       false) {
			k_sleep(K_MSEC(10));
		}

		TERM_SUCCESS("Updating LL data length succeeded %s", addr);
	}
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	const char *data_ptr = (const char *)data + NOTIFICATION_DATA_PREFIX_LEN;
	uint32_t received_counter;
	char addr[BT_ADDR_LE_STR_LEN];

	if (!data) {
		TERM_INFO("[UNSUBSCRIBED]");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	received_counter = strtoul(data_ptr, NULL, 0);

	if ((conn_info.notify_counter % 30) == 0) {
		TERM_PRINT("[NOTIFICATION] addr %s data %s length %u cnt %u", addr, data, length,
			   received_counter);
	}

	__ASSERT(conn_info.notify_counter == received_counter,
		 "expected counter : %u , received counter : %u", conn_info.notify_counter,
		 received_counter);
	conn_info.notify_counter++;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	char uuid_str[BT_UUID_STR_LEN];

	if (!attr) {
		TERM_INFO("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	bt_uuid_to_str(params->uuid, uuid_str, sizeof(uuid_str));
	TERM_PRINT("UUID found : %s", uuid_str);

	TERM_PRINT("[ATTRIBUTE] handle %u", attr->handle);

	if (discover_params.type == BT_GATT_DISCOVER_PRIMARY) {
		TERM_PRINT("Primary Service Found");
		memcpy(&uuid, CENTRAL_CHARACTERISTIC_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TERM_ERR("Discover failed (err %d)", err);
		}
	} else if (discover_params.type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		TERM_PRINT("Service Characteristic Found");
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TERM_ERR("Discover failed (err %d)", err);
		}
	} else {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			TERM_ERR("Subscribe failed (err %d)", err);
		} else {
			char addr[BT_ADDR_LE_STR_LEN];

			bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

			atomic_set_bit(conn_info.flags, CONN_INFO_SUBSCRIBED_TO_SERVICE);
			TERM_INFO("[SUBSCRIBED] addr %s", addr);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void subscribe_to_service(struct bt_conn *conn)
{
	if (atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED_TO_SERVICE) == false) {
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
		if (err) {
			TERM_ERR("Discover failed(err %d)", err);
			return;
		}

		while (atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED_TO_SERVICE) == false) {
			k_sleep(K_MSEC(10));
		}
	}
}

void main(void)
{
	struct bt_gatt_attr *vnd_ind_attr;
	char str[BT_UUID_STR_LEN];
	int err;

	err = bt_enable(NULL);
	if (err) {
		TERM_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	bt_ready();

	bt_gatt_cb_register(&gatt_callbacks);

	vnd_ind_attr = bt_gatt_find_by_uuid(vnd_svc.attrs, vnd_svc.attr_count, &vnd_enc_uuid.uuid);

	bt_uuid_to_str(&vnd_enc_uuid.uuid, str, sizeof(str));
	TERM_PRINT("Indicate VND attr %p (UUID %s)", vnd_ind_attr, str);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (true) {

		if (conn_info.conn_ref == NULL) {
			k_sleep(K_MSEC(10));
			continue;
		}

#if defined(CONFIG_BT_SMP)
		if (atomic_test_bit(conn_info.flags, CONN_INFO_SECURITY_LEVEL_UPDATED) == false) {
			k_sleep(K_MSEC(10));
			continue;
		}
#endif

		if (atomic_test_bit(conn_info.flags, CONN_INFO_CONN_PARAMS_UPDATED) == false) {
			k_sleep(K_MSEC(10));
			continue;
		}

		if (atomic_test_bit(conn_info.flags, CONN_INFO_MTU_EXCHANGED) == false) {
			k_sleep(K_MSEC(10));
			continue;
		}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
		bt_conn_foreach(BT_CONN_TYPE_LE, update_ll_max_data_length, NULL);
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

		subscribe_to_service(conn_info.conn_ref);

		if (atomic_test_bit(conn_info.flags, CONN_INFO_SUBSCRIBED_TO_SERVICE) == false) {
			k_sleep(K_MSEC(10));
			continue;
		}

		k_sleep(K_SECONDS(1));

		/* Vendor indication simulation */
		if (simulate_vnd && vnd_ind_attr) {
			memset(vnd_value, 0x00, sizeof(vnd_value));
			snprintk(vnd_value, NOTIFICATION_DATA_LEN, "%s%u", NOTIFICATION_DATA_PREFIX,
				 tx_notify_counter++);
			err = bt_gatt_notify(NULL, vnd_ind_attr, vnd_value, NOTIFICATION_DATA_LEN);
			if (err) {
				TERM_ERR("Couldn't send GATT notification");
			}
		}

		if (((k_uptime_get() - uptime_ref) / 1000) >= 70) {
			err = bt_conn_disconnect(conn_info.conn_ref, BT_HCI_ERR_REMOTE_POWER_OFF);

			if (err) {
				TERM_ERR("Terminating conn failed (err %d)", err);
			}

			while (conn_info.conn_ref != NULL) {
				k_sleep(K_MSEC(10));
			}
		}
	}
}
