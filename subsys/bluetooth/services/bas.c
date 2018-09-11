/** @file
 *  @brief GATT Battery Service
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <init.h>
#include <misc/__assert.h>
#include <stdbool.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <bluetooth/services/bas.h>

#include <sensor.h>

#define LOG_MODULE_NAME bas
#define LOG_LEVEL CONFIG_BT_GATT_BAS_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER();

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[BT_GATT_CCC_MAX] = {};

static struct device *battery;
static struct sensor_value charge_lvl;
static bool notif_enabled;

int bt_gatt_bas_notify(void);

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       u16_t value)
{
	ARG_UNUSED(attr);

	notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t read_blvl(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       u16_t len, u16_t offset)
{
	u8_t lvl8 = (u8_t)(charge_lvl.val1);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl8,
				 sizeof(lvl8));
}

BT_GATT_SERVICE_DEFINE(bas,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_blvl, NULL, &battery),
	BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),
);


static void charge_lvl_fetch(void)
{
	int rc;

	__ASSERT(battery != NULL, "No binding to battery sensor!\n");

	rc = sensor_sample_fetch_chan(battery, SENSOR_CHAN_STATE_OF_CHARGE);
	if (rc) {
		LOG_ERR("Could not obtain new sensor reading, err %d", rc);
		return;
	}

	rc = sensor_channel_get(battery, SENSOR_CHAN_STATE_OF_CHARGE,
				&charge_lvl);
	if (rc) {
		LOG_ERR("Could not read sensor data, err %d", rc);
		return;
	}
}

void level_changed_callback(struct device *dev, struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);

	if (trigger->type == SENSOR_TRIG_DATA_READY &&
	    trigger->chan == SENSOR_CHAN_STATE_OF_CHARGE) {
		/* fetch battery level and send notifications, if enabled */
		charge_lvl_fetch();
		bt_gatt_bas_notify();
	}
}

static int bas_init(struct device *dev)
{
	int rc;

	ARG_UNUSED(dev);

	battery = device_get_binding(CONFIG_BATTERY_SENSOR_DEV_NAME);
	if (!battery) {
		LOG_ERR("Could not bind to device %s",
			CONFIG_BATTERY_SENSOR_DEV_NAME);
		return -EIO;
	}

	struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_STATE_OF_CHARGE
	};

	rc = sensor_trigger_set(battery, &trigger, level_changed_callback);
	if (rc) {
		LOG_ERR("Could not set up sensor trigger, err %d", rc);
		return rc;
	}

	return 0;
}

int bt_gatt_bas_notify(void)
{
	u8_t lvl8;

	if (!notif_enabled) {
		return -EINVAL;
	}

	lvl8 = (u8_t)(charge_lvl.val1);

	return bt_gatt_notify(NULL, &bas.attrs[1], &lvl8, sizeof(lvl8));
}

SYS_INIT(bas_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
