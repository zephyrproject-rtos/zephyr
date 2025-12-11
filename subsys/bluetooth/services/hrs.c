/** @file
 *  @brief HRS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hrs.h>

#define LOG_LEVEL CONFIG_BT_HRS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hrs);


#define GATT_PERM_READ_MASK     (BT_GATT_PERM_READ | \
				 BT_GATT_PERM_READ_ENCRYPT | \
				 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_WRITE_MASK    (BT_GATT_PERM_WRITE | \
				 BT_GATT_PERM_WRITE_ENCRYPT | \
				 BT_GATT_PERM_WRITE_AUTHEN)

#ifndef CONFIG_BT_HRS_DEFAULT_PERM_RW_AUTHEN
#define CONFIG_BT_HRS_DEFAULT_PERM_RW_AUTHEN 0
#endif
#ifndef CONFIG_BT_HRS_DEFAULT_PERM_RW_ENCRYPT
#define CONFIG_BT_HRS_DEFAULT_PERM_RW_ENCRYPT 0
#endif
#ifndef CONFIG_BT_HRS_DEFAULT_PERM_RW
#define CONFIG_BT_HRS_DEFAULT_PERM_RW 0
#endif

/**
 * @brief GATT ATTR Error that should be returned in case
 * HRS Control point request is not supported.
 */
#define BT_HRS_ATT_ERR_CONTROL_POINT_NOT_SUPPORTED 0x80

#define HRS_GATT_PERM_DEFAULT (						\
	CONFIG_BT_HRS_DEFAULT_PERM_RW_AUTHEN ?				\
	(BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN) :	\
	CONFIG_BT_HRS_DEFAULT_PERM_RW_ENCRYPT ?				\
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT) :	\
	(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE))			\

static uint8_t hrs_blsc;
static sys_slist_t hrs_cbs = SYS_SLIST_STATIC_INIT(&hrs_cbs);

static void hrmc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	struct bt_hrs_cb *listener;

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("HRS notifications %s", notif_enabled ? "enabled" : "disabled");

	SYS_SLIST_FOR_EACH_CONTAINER(&hrs_cbs, listener, _node) {
		if (listener->ntf_changed) {
			listener->ntf_changed(notif_enabled);
		}
	}
}

static ssize_t read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hrs_blsc,
				 sizeof(hrs_blsc));
}

static ssize_t ctrl_point_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	int err = -ENOTSUP;
	struct bt_hrs_cb *listener;

	LOG_INF("HRS CTRL Point Written %d", len);

	SYS_SLIST_FOR_EACH_CONTAINER(&hrs_cbs, listener, _node) {
		if (listener->ctrl_point_write) {
			err = listener->ctrl_point_write(*((uint8_t *)buf));
			/* If we get an error other than ENOTSUP then immediately
			 * break the loop and return a generic gatt error, assuming this
			 * listener supports this request code, but failed to serve it
			 */
			if ((err != 0) && (err != -ENOTSUP)) {
				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}
		}
	}
	if (err) {
		return BT_GATT_ERR(BT_HRS_ATT_ERR_CONTROL_POINT_NOT_SUPPORTED);
	} else {
		return len;
	}
}

/* Heart Rate Service Declaration */
BT_GATT_SERVICE_DEFINE(hrs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(hrmc_ccc_cfg_changed,
		    HRS_GATT_PERM_DEFAULT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ,
			       HRS_GATT_PERM_DEFAULT & GATT_PERM_READ_MASK,
			       read_blsc, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE,
			       HRS_GATT_PERM_DEFAULT & GATT_PERM_WRITE_MASK,
			       NULL, ctrl_point_write, NULL),
);

static int hrs_init(void)
{

	hrs_blsc = 0x01;

	return 0;
}

int bt_hrs_cb_register(struct bt_hrs_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	sys_slist_append(&hrs_cbs, &cb->_node);

	return 0;
}

int bt_hrs_cb_unregister(struct bt_hrs_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&hrs_cbs, &cb->_node)) {
		return -ENOENT;
	}

	return 0;
}

int bt_hrs_notify(uint16_t heartrate)
{
	int rc;
	static uint8_t hrm[2];

	hrm[0] = 0x06; /* uint8, sensor contact */
	hrm[1] = heartrate;

	rc = bt_gatt_notify(NULL, &hrs_svc.attrs[1], &hrm, sizeof(hrm));

	return rc == -ENOTCONN ? 0 : rc;
}

SYS_INIT(hrs_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
