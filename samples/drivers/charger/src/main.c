/*
 * Copyright (c) 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/printk.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/charger.h>
#include <sys/errno.h>

static const struct device *chgdev = DEVICE_DT_GET(DT_NODELABEL(charger));

int main(void)
{
	union charger_propval val;
	int ret;

	if (chgdev == NULL) {
		printk("Error: no charger device found.\n");
		return 0;
	}

	if (!device_is_ready(chgdev)) {
		printk("Error: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       chgdev->name);
		return 0;
	}

	printk("Found device \"%s\", getting charger data\n", chgdev->name);

	while (1) {
		/* Poll until external power is presented to the charger */
		do {
			ret = charger_get_prop(chgdev, CHARGER_PROP_ONLINE, &val);
			if (ret < 0) {
				return ret;
			}

			k_msleep(100);
		} while (val.online == CHARGER_ONLINE_OFFLINE);

		val.status = CHARGER_STATUS_CHARGING;

		ret = charger_charge_enable(chgdev, true);
		if (ret == -ENOTSUP) {
			printk("Enabling charge not supported, assuming auto charge enable\n");
			continue;
		} else if (ret < 0) {
			return ret;
		}

		k_msleep(500);

		ret = charger_get_prop(chgdev, CHARGER_PROP_STATUS, &val);
		if (ret < 0) {
			return ret;
		}

		switch (val.status) {
		case CHARGER_STATUS_CHARGING:
			printk("Charging in progress...\n");

			ret = charger_get_prop(chgdev, CHARGER_PROP_CHARGE_TYPE, &val);
			if (ret < 0) {
				return ret;
			}

			printk("Device \"%s\" charge type is %d\n", chgdev->name, val.charge_type);
			break;
		case CHARGER_STATUS_NOT_CHARGING:
			printk("Charging halted...\n");

			ret = charger_get_prop(chgdev, CHARGER_PROP_HEALTH, &val);
			if (ret < 0) {
				return ret;
			}

			printk("Device \"%s\" health is %d\n", chgdev->name, val.health);
			break;
		case CHARGER_STATUS_FULL:
			printk("Charging complete!");
			return 0;
		case CHARGER_STATUS_DISCHARGING:
			printk("External power removed, discharging\n");

			ret = charger_get_prop(chgdev, CHARGER_PROP_ONLINE, &val);
			if (ret < 0) {
				return ret;
			}
			break;
		default:
			return -EIO;
		}

		k_msleep(500);
	}
}
