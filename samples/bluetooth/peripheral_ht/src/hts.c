/** @file
 *  @brief HTS Service sample
 */

/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
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

static u8_t simulate_htm;
static u8_t indicating;
static struct bt_gatt_indicate_params ind_params;

static u8_t temperature = 20U;

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	simulate_htm = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			u8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
	indicating = 0U;
}

/* Heart Rate Service Declaration */
BT_GATT_SERVICE_DEFINE(hts_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_MEASUREMENT, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	/* more optional Characteristics */
);

void hts_init(void)
{
}

void hts_indicate(void)
{
	/* Temperature measurements simulation */
	if (simulate_htm) {
		if (indicating) {
			return;
		}

		temperature++;
		if (temperature == 30U) {
			temperature = 20U;
		}

		static u8_t htm[1 + 4];
		htm[0] = 0x0; /* uint8, temperature in celsius*/
		/* (IEEE-11073 32-bit FLOAT). */
		htm[1] = temperature; /* mantissa = temperature*/
		htm[2] = htm[3] = htm[4] = 0; /* sign = 0, exponent= 0 */

		ind_params.attr = &hts_svc.attrs[2];
		ind_params.func = indicate_cb;
		ind_params.data = &htm;
		ind_params.len = sizeof(htm);

		if (bt_gatt_indicate(NULL, &ind_params) == 0) {
			indicating = 1U;
		}
	}
}
