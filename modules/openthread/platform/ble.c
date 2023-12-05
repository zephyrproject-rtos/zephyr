/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

/* Zephyr OpenThread integration Library */
#include <zephyr/net/openthread.h>

/* OpenThread BLE driver API */
#include <openthread/platform/ble.h>

/* Zephyr Logging */

#define LOG_MODULE_NAME net_openthread_tcat
#define LOG_LEVEL       CONFIG_OPENTHREAD_LOG_LEVEL

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* BLE connection constants as defined in thread specification. */
#define TOBLE_SERVICE_UUID 0xfffb
#define RX_CHARACTERISTIC_UUID                                                                     \
	BT_UUID_128_ENCODE(0x6bd10d8b, 0x85a7, 0x4e5a, 0xba2d, 0xc83558a5f220)
#define TX_CHARACTERISTIC_UUID                                                                     \
	BT_UUID_128_ENCODE(0x7fddf61f, 0x280a, 0x4773, 0xb448, 0xba1b8fe0dd69)

#define BT_UUID_TCAT_SERVICE    BT_UUID_DECLARE_16(TOBLE_SERVICE_UUID)
#define BT_UUID_TCAT_SERVICE_RX BT_UUID_DECLARE_128(RX_CHARACTERISTIC_UUID)
#define BT_UUID_TCAT_SERVICE_TX BT_UUID_DECLARE_128(TX_CHARACTERISTIC_UUID)

#define PLAT_BLE_THREAD_DEALY 500
#define PLAT_BLE_MSG_DATA_MAX CONFIG_BT_L2CAP_TX_MTU /* must match the maximum MTU size used */

#define PLAT_BLE_MSG_CONNECT    (PLAT_BLE_MSG_DATA_MAX + 1U)
#define PLAT_BLE_MSG_DISCONNECT (PLAT_BLE_MSG_CONNECT + 1U)

/* Zephyr Kernel Objects */

static void ot_plat_ble_thread(void *, void *, void *);
static uint8_t ot_plat_ble_msg_buf[PLAT_BLE_MSG_DATA_MAX];

static K_SEM_DEFINE(ot_plat_ble_init_semaphore, 0, 1);
static K_SEM_DEFINE(ot_plat_ble_event_semaphore, 0, K_SEM_MAX_LIMIT);
RING_BUF_DECLARE(ot_plat_ble_ring_buf, CONFIG_OPENTHREAD_BLE_TCAT_RING_BUF_SIZE);
static K_THREAD_DEFINE(ot_plat_ble_tid, CONFIG_OPENTHREAD_BLE_TCAT_THREAD_STACK_SIZE,
		       ot_plat_ble_thread, NULL, NULL, NULL, 5, 0, PLAT_BLE_THREAD_DEALY);

/* OpenThread Objects */

static otInstance *ble_openthread_instance;

/* BLE service Objects */

/* forward declaration for callback functions */
static ssize_t on_receive(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags);
static void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value);

/* Service Declaration and Registration */
BT_GATT_SERVICE_DEFINE(my_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_TCAT_SERVICE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_TCAT_SERVICE_RX,
					      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL,
					      on_receive, NULL),
		       BT_GATT_CHARACTERISTIC(BT_UUID_TCAT_SERVICE_TX, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ, NULL, NULL, NULL),
		       BT_GATT_CCC(on_cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),);

/* Zephyr BLE Objects */

/* forward declaration for callback functions */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param);
static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout);

static struct bt_conn *ot_plat_ble_connection;

static struct bt_conn_cb conn_callbacks = {.connected = connected,
					   .disconnected = disconnected,
					   .le_param_req = le_param_req,
					   .le_param_updated = le_param_updated};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(TOBLE_SERVICE_UUID)),
};

/* Zephyr BLE Message Queue and Thread */

static bool ot_plat_ble_queue_msg(const uint8_t *aData, uint16_t aLen, int8_t aRssi)
{
	otError error = OT_ERROR_NONE;
	uint16_t len = 0;

	if (aLen <= PLAT_BLE_MSG_DATA_MAX && aData == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	k_sched_lock();

	len = sizeof(aLen) + sizeof(aRssi) + ((aLen <= PLAT_BLE_MSG_DATA_MAX) ? aLen : 0);

	if (ring_buf_space_get(&ot_plat_ble_ring_buf) >= len) {
		ring_buf_put(&ot_plat_ble_ring_buf, (uint8_t *)&aLen, sizeof(aLen));
		ring_buf_put(&ot_plat_ble_ring_buf, &aRssi, sizeof(aRssi));
		if (aLen <= PLAT_BLE_MSG_DATA_MAX) {
			ring_buf_put(&ot_plat_ble_ring_buf, aData, aLen);
		}
		k_sem_give(&ot_plat_ble_event_semaphore);
	} else {
		error = OT_ERROR_NO_BUFS;
	}

	k_sched_unlock();

	return error;
}

static void ot_plat_ble_thread(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	uint16_t len;
	int8_t rssi;
	otBleRadioPacket my_packet;

	LOG_INF("%s started", __func__);

	while (1) {
		k_sem_take(&ot_plat_ble_event_semaphore, K_FOREVER);
		ring_buf_get(&ot_plat_ble_ring_buf, (uint8_t *)&len, sizeof(len));
		ring_buf_get(&ot_plat_ble_ring_buf, &rssi, sizeof(rssi));
		if (len <= PLAT_BLE_MSG_DATA_MAX) {
			ring_buf_get(&ot_plat_ble_ring_buf, ot_plat_ble_msg_buf, len);
		}

		openthread_api_mutex_lock(openthread_get_default_context());

		if (len <= PLAT_BLE_MSG_DATA_MAX) {
			/* The packet parameter in otPlatBleGattServerOnWriteRequest is not const.
			 * Re-write all members.
			 */
			my_packet.mValue = ot_plat_ble_msg_buf;
			my_packet.mPower = rssi;
			my_packet.mLength = len;
			otPlatBleGattServerOnWriteRequest(ble_openthread_instance, 0, &my_packet);
		} else if (len == PLAT_BLE_MSG_CONNECT) {
			otPlatBleGapOnConnected(ble_openthread_instance, 0);
		} else if (len == PLAT_BLE_MSG_DISCONNECT) {
			otPlatBleGapOnDisconnected(ble_openthread_instance, 0);
		}
		openthread_api_mutex_unlock(openthread_get_default_context());
	}
}

/* Zephyr BLE service callbacks */

/* This function is called whenever the RX Characteristic has been written to by a Client */
static ssize_t on_receive(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Received data, handle %" PRIu16 ", len %" PRIu16, attr->handle, len);

	otError error = ot_plat_ble_queue_msg(buf, len, 0);

	if (error != OT_ERROR_NONE) {
		LOG_WRN("Error queuing message: %s", otThreadErrorToString(error));
	}

	return len;
}

/* This function is called whenever a Notification has been sent by the TX Characteristic */
static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);

	LOG_DBG("Data sent");
}

/* This function is called whenever the CCCD register has been changed by the client */
void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	uint16_t mtu;
	otError error = OT_ERROR_NONE;

	ARG_UNUSED(attr);

	switch (value) {
	case BT_GATT_CCC_NOTIFY:

		error = ot_plat_ble_queue_msg(NULL, PLAT_BLE_MSG_CONNECT, 0);
		if (error != OT_ERROR_NONE) {
			LOG_WRN("Error queuing message: %s", otThreadErrorToString(error));
		}

		error = otPlatBleGattMtuGet(ble_openthread_instance, &mtu);
		if (error != OT_ERROR_NONE) {
			LOG_WRN("Error retrieving mtu: %s", otThreadErrorToString(error));
		}

		LOG_INF("CCCD update (mtu=%" PRIu16 ")!", mtu);

		break;

	default:
		break;
	}
}

otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle,
				    const otBleRadioPacket *aPacket)
{
	ARG_UNUSED(aInstance);

	/* TO DO change to indications. */
	const struct bt_gatt_attr *attr = &my_service.attrs[3];

	struct bt_gatt_notify_params params = {.uuid = BT_UUID_TCAT_SERVICE_TX,
					       .attr = attr,
					       .data = aPacket->mValue,
					       .len = aPacket->mLength,
					       .func = on_sent};

	LOG_DBG("Send data, handle %d, len %d", attr->handle, aPacket->mLength);

	/* Only one connection supported */
	if (aHandle != 0) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (ot_plat_ble_connection == NULL) {
		return OT_ERROR_INVALID_STATE;
	}

	/* Check whether notifications are enabled or not */
	if (bt_gatt_is_subscribed(ot_plat_ble_connection, attr, BT_GATT_CCC_NOTIFY)) {
		if (bt_gatt_notify_cb(ot_plat_ble_connection, &params)) {
			LOG_WRN("Error, unable to send notification");
			return OT_ERROR_INVALID_ARGS;
		}
	} else {
		LOG_WRN("Warning, notification not enabled on the selected attribute");
		return OT_ERROR_INVALID_STATE;
	}

	return OT_ERROR_NONE;
}

otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu)
{
	ARG_UNUSED(aInstance);

	if (ot_plat_ble_connection == NULL) {
		return OT_ERROR_FAILED;
	}

	if (aMtu != NULL) {
		*aMtu = bt_gatt_get_mtu(ot_plat_ble_connection);
	}

	return OT_ERROR_NONE;
}

otError otPlatBleGapDisconnect(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	if (ot_plat_ble_connection == NULL) {
		return OT_ERROR_INVALID_STATE;
	}

	if (bt_conn_disconnect(ot_plat_ble_connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN)) {
		return OT_ERROR_INVALID_STATE;
	}

	return OT_ERROR_NONE;
}

/* Zephyr BLE callbacks */

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];
	uint16_t mtu;
	otError error = OT_ERROR_NONE;

	ot_plat_ble_connection = bt_conn_ref(conn);

	if (err) {
		LOG_WRN("Connection failed (err %u)", err);
		return;
	} else if (bt_conn_get_info(conn, &info)) {
		LOG_WRN("Could not parse connection info");
	} else {
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		error = otPlatBleGattMtuGet(ble_openthread_instance, &mtu);
		if (error != OT_ERROR_NONE) {
			LOG_WRN("Error retrieving mtu: %s", otThreadErrorToString(error));
		}

		LOG_INF("Connection established (mtu=%" PRIu16 ")!", mtu);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	otError error = OT_ERROR_NONE;

	LOG_INF("Disconnected (reason %" PRIu8 ")", reason);

	if (ot_plat_ble_connection) {
		bt_conn_unref(ot_plat_ble_connection);
		ot_plat_ble_connection = NULL;

		error = ot_plat_ble_queue_msg(NULL, PLAT_BLE_MSG_DISCONNECT, 0);
		if (error != OT_ERROR_NONE) {
			LOG_WRN("Error queuing message: %s", otThreadErrorToString(error));
		}
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];
	uint16_t mtu;
	otError error = OT_ERROR_NONE;

	if (bt_conn_get_info(conn, &info)) {
		LOG_INF("Could not parse connection info");
	} else {
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		error = otPlatBleGattMtuGet(ble_openthread_instance, &mtu);

		if (error != OT_ERROR_NONE) {
			LOG_WRN("Error retrieving mtu: %s", otThreadErrorToString(error));
		}

		LOG_INF("Connection parameters updated (mtu=%" PRIu16 ")!", mtu);
	}
}

static void bt_ready(int err)
{
	if (err) {
		LOG_WRN("BLE init failed with error code %d", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	k_sem_give(&ot_plat_ble_init_semaphore); /* BLE stack up an running */
}

otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval)
{
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aInterval);

	/* TO DO advertisement format change */
	int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err != 0 && err != -EALREADY) {
		LOG_WRN("Advertising failed to start (err %d)", err);
		return OT_ERROR_INVALID_STATE;
	}

	LOG_INF("Advertising successfully started");

	return OT_ERROR_NONE;
}

otError otPlatBleGapAdvStop(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);

	int err = bt_le_adv_stop();

	if (err != 0 && err != -EALREADY) {
		LOG_WRN("Advertisement failed to stop (err %d)", err);
		return OT_ERROR_FAILED;
	}
	return OT_ERROR_NONE;
}

/* Zephyr BLE initialization */

otError otPlatBleEnable(otInstance *aInstance)
{
	int err;

	ble_openthread_instance = aInstance;
	err = bt_enable(bt_ready);

	if (err != 0 && err != -EALREADY) {
		LOG_WRN("BLE enable failed with error code %d", err);
		return OT_ERROR_FAILED;
	} else if (err == -EALREADY) {
		err = k_sem_take(&ot_plat_ble_init_semaphore, K_MSEC(500));
		return OT_ERROR_NONE;
	}

	err = k_sem_take(&ot_plat_ble_init_semaphore, K_MSEC(500));

	if (!err) {
		LOG_INF("Bluetooth initialized");
	} else {
		LOG_INF("BLE initialization did not complete in time");
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError otPlatBleDisable(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);
	/* This function intentionally does nothing since disabling advertisement disables BLE
	 * stack.
	 */
	return OT_ERROR_NONE;
}
