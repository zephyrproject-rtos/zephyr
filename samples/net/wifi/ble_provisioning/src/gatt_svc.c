/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi.h>

#include "gatt_svc.h"
#include "wifi_prov.h"

LOG_MODULE_REGISTER(gatt_svc, LOG_LEVEL_INF);

/*
 * Custom 128-bit service: 8b84xxxx-f6fa-4e33-9a0c-e14d9f35f201
 *   0001 -> service
 *   0002 -> SSID (write)
 *   0003 -> Password (write)
 *   0004 -> Security type (write, 1 byte: see enum wifi_security_type)
 *   0005 -> Control (write, 1 byte: 0x01 connect, 0x02 erase)
 *   0006 -> Status (read, notify)
 */
#define PROV_UUID_16(short16)                                                                      \
	BT_UUID_128_ENCODE(0x8b840000 | (short16), 0xf6fa, 0x4e33, 0x9a0c, 0xe14d9f35f201)

#if defined(CONFIG_WIFI_BLE_PROV_SECURITY_AUTH)
#define PROV_WRITE_PERM BT_GATT_PERM_WRITE_AUTHEN
#define PROV_READ_PERM  BT_GATT_PERM_READ_AUTHEN
#define PROV_CCC_PERM   (BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN)
#elif defined(CONFIG_WIFI_BLE_PROV_SECURITY_ENCRYPT)
#define PROV_WRITE_PERM BT_GATT_PERM_WRITE_ENCRYPT
#define PROV_READ_PERM  BT_GATT_PERM_READ_ENCRYPT
#define PROV_CCC_PERM   (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT)
#else /* CONFIG_WIFI_BLE_PROV_SECURITY_NONE */
#define PROV_WRITE_PERM BT_GATT_PERM_WRITE
#define PROV_READ_PERM  BT_GATT_PERM_READ
#define PROV_CCC_PERM   (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
#endif

static const struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0001));
static const struct bt_uuid_128 ssid_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0002));
static const struct bt_uuid_128 pass_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0003));
static const struct bt_uuid_128 sec_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0004));
static const struct bt_uuid_128 ctrl_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0005));
static const struct bt_uuid_128 stat_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0006));

static struct gatt_svc_callbacks user_cb;
static uint8_t current_status = GATT_SVC_STATUS_IDLE;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static ssize_t write_ssid(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len == 0 || len > WIFI_PROV_SSID_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (user_cb.ssid_written) {
		user_cb.ssid_written(buf, len);
	}

	return len;
}

static ssize_t write_password(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > WIFI_PROV_PSK_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (user_cb.password_written) {
		user_cb.password_written(buf, len);
	}

	return len;
}

static ssize_t write_security(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t sec;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	sec = ((const uint8_t *)buf)[0];

	if (sec != WIFI_PROV_SECURITY_AUTO && sec > WIFI_SECURITY_TYPE_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (user_cb.security_written) {
		user_cb.security_written(sec);
	}

	return len;
}

static ssize_t write_control(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (user_cb.control_written) {
		user_cb.control_written(((const uint8_t *)buf)[0]);
	}

	return len;
}

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &current_status,
				 sizeof(current_status));
}

/* clang-format off */
BT_GATT_SERVICE_DEFINE(prov_svc,
	BT_GATT_PRIMARY_SERVICE(&svc_uuid),
	BT_GATT_CHARACTERISTIC(&ssid_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       PROV_WRITE_PERM,
			       NULL, write_ssid, NULL),
	BT_GATT_CHARACTERISTIC(&pass_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       PROV_WRITE_PERM,
			       NULL, write_password, NULL),
	BT_GATT_CHARACTERISTIC(&sec_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       PROV_WRITE_PERM,
			       NULL, write_security, NULL),
	BT_GATT_CHARACTERISTIC(&ctrl_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       PROV_WRITE_PERM,
			       NULL, write_control, NULL),
	BT_GATT_CHARACTERISTIC(&stat_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       PROV_READ_PERM,
			       read_status, NULL, &current_status),
	BT_GATT_CCC(NULL, PROV_CCC_PERM),
);
/* clang-format on */

int gatt_svc_init(const struct gatt_svc_callbacks *cb)
{
	if (cb) {
		user_cb = *cb;
	}

	return 0;
}

void gatt_svc_set_status(enum gatt_svc_status status)
{
	current_status = (uint8_t)status;

	LOG_INF("status -> %u", current_status);

	(void)bt_gatt_notify_uuid(NULL, &stat_uuid.uuid, prov_svc.attrs, &current_status,
				  sizeof(current_status));
}

int gatt_svc_adv_start(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err == -EALREADY) {
		return 0;
	}

	if (err) {
		LOG_ERR("adv start failed (%d)", err);
		return err;
	}

	LOG_INF("advertising as '%s'", CONFIG_BT_DEVICE_NAME);

	return 0;
}

int gatt_svc_adv_stop(void)
{
	int err = bt_le_adv_stop();

	if (err && err != -EALREADY) {
		LOG_ERR("adv stop failed (%d)", err);
		return err;
	}

	return 0;
}
