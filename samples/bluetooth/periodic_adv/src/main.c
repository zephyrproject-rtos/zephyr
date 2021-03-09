/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>

#define BT_LE_PER_ADV_CUSTOM_10 BT_LE_PER_ADV_PARAM(10000000 / 1250, /* 10s periodic advertisement period */ \ 
                                                 10200000 / 1250, \
                                                 BT_LE_PER_ADV_OPT_NONE)

#define BT_LE_PER_ADV_CUSTOM_3 BT_LE_PER_ADV_PARAM(3000000 / 1250, /* 3s periodic advertisement period */ \ 
                                                 3200000 / 1250, \
                                                 BT_LE_PER_ADV_OPT_NONE)

#define BT_LE_PER_ADV_CUSTOM_1 BT_LE_PER_ADV_PARAM(1000000 / 1250, /* 1s periodic advertisement period */ \ 
                                                 1200000 / 1250, \
                                                 BT_LE_PER_ADV_OPT_NONE)

#define BT_LE_EXT_ADV_NCONN_NAME_1 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M | \
/* 1M Advertisement */                                                   BT_LE_ADV_OPT_USE_NAME, \
                                                   500000 / 625, \
                                                   600000 / 625,\
                                                   NULL)

#define BT_LE_EXT_ADV_NCONN_NAME_2 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
/* 1M + 2M Advertisement */                        BT_LE_ADV_OPT_USE_NAME, \
                                                   500000 / 625, \
                                                   600000 / 625,\
                                                   NULL)

static uint8_t mfg_data[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 16),
};

void main(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	printk("Starting Periodic Advertising Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME_1, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_CUSTOM_10);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}

	while (true) {
		k_sleep(K_MSEC(2100));

		mfg_data[15]++;

		printk("Set Periodic Advertising Data...");
		err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
		if (err) {
			printk("Failed (err %d)\n", err);
			return;
		}
		printk("done.\n");
	}
}
