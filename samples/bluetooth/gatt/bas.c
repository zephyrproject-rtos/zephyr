/** @file
 *  @brief BAS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static uint8_t simulate_blvl;
static uint8_t battery = 100;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	simulate_blvl = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Battery Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_PERM_READ,
			   read_blvl, NULL, &battery),
	BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),
};

void bas_init(void)
{
	bt_gatt_register(attrs, ARRAY_SIZE(attrs));
}

void bas_notify(void)
{
	if (!simulate_blvl) {
		return;
	}

	battery--;
	if (!battery) {
		/* Software eco battery charger */
		battery = 100;
	}

	bt_gatt_notify(NULL, &attrs[2], &battery, sizeof(battery));
}
