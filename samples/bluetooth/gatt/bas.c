/** @file
 *  @brief BAS Service sample
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
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#if defined(CONFIG_SOC_SERIES_NRF51X)
#include <soc.h>
#include "bat.h"
#endif

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t is_notify_enabled;
static u8_t battery = 100;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	is_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

static void isr_nrf51_adc(void *arg)
{
	extern void C_ADC_IRQHandler(void);

	C_ADC_IRQHandler();
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

static struct bt_gatt_service bas_svc = BT_GATT_SERVICE(attrs);

void bas_init(void)
{
	bt_gatt_service_register(&bas_svc);

#if defined(CONFIG_SOC_SERIES_NRF51X)
	IRQ_CONNECT(NRF5_IRQ_ADC_IRQn, 1, isr_nrf51_adc, NULL, 0);
	bat_acquire();
#endif
}

void bas_notify(void)
{
	if (!is_notify_enabled) {
		return;
	}

#if defined(CONFIG_SOC_SERIES_NRF51X)
	/* TODO: Notify on fresh sample.*/
	/* NOTE: Below implementation acquires previous captured levels */
	battery = bat_level_get(BAT_LEVELS_CR2023, c_bat_levels_cr2032);
	bat_acquire();
#else
	battery--;
	if (!battery) {
		/* Software eco battery charger */
		battery = 100;
	}
#endif

	bt_gatt_notify(NULL, &attrs[2], &battery, sizeof(battery));
}
