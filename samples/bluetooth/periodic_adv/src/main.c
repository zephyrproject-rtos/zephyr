/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data per_adv_ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	printk("Starting Periodic Advertising Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	while (true) {
		printk("Start Extended Advertising...");
		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising "
			       "(err %d)\n", err);
			return 0;
		}
		printk("done.\n");

		for (int i = 0; i < 3; i++) {
			k_sleep(K_SECONDS(10));

			mfg_data[2]++;

			printk("Set Periodic Advertising Data...");
			err = bt_le_per_adv_set_data(adv, per_adv_ad, ARRAY_SIZE(per_adv_ad));
			if (err) {
				printk("Failed (err %d)\n", err);
				return 0;
			}
			printk("done.\n");
		}

		k_sleep(K_SECONDS(10));

		printk("Stop Extended Advertising...");
		err = bt_le_ext_adv_stop(adv);
		if (err) {
			printk("Failed to stop extended advertising "
			       "(err %d)\n", err);
			return 0;
		}
		printk("done.\n");

		k_sleep(K_SECONDS(10));
	}
	return 0;
}
