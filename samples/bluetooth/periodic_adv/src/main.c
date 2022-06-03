/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/direction.h>

#if defined(DISABLED_CODE)
static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};
#endif

/* Length of CTE in unit of 8[us] */
#define CTE_LEN (0x14U)
/* Number of CTE send in single periodic advertising train */
#define PER_ADV_EVENT_CTE_COUNT 1

struct bt_df_adv_cte_tx_param cte_params = { .cte_len = CTE_LEN,
					     .cte_count = PER_ADV_EVENT_CTE_COUNT,
#if defined(CONFIG_BT_DF_CTE_TX_AOD)
					     .cte_type = BT_DF_CTE_TYPE_AOD_2US,
					     .num_ant_ids = ARRAY_SIZE(ant_patterns),
					     .ant_ids = ant_patterns
#else
					     .cte_type = BT_DF_CTE_TYPE_AOA,
					     .num_ant_ids = 0,
					     .ant_ids = NULL
#endif /* CONFIG_BT_DF_CTE_TX_AOD */
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
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return;
	}

	printk("Update CTE params...");
	err = bt_df_set_adv_cte_tx_param(adv, &cte_params);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success\n");

	printk("Enable CTE...");
	err = bt_df_adv_cte_tx_enable(adv);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success\n");

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}

	while (true) {
		printk("Start Extended Advertising...");
		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising "
			       "(err %d)\n", err);
			return;
		}
		printk("done.\n");

#if defined(DISABLED_CODE)
		for (int i = 0; i < 3; i++) {
			k_sleep(K_SECONDS(10));

			mfg_data[2]++;

			printk("Set Periodic Advertising Data...");
			err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
			if (err) {
				printk("Failed (err %d)\n", err);
				return;
			}
			printk("done.\n");
		}
#endif
		k_sleep(K_SECONDS(10));

		printk("Stop Periodic Advertising...");
		err = bt_le_per_adv_stop(adv);
		if (err) {
			printk("Failed to stop Periodic Advertising "
			       "(err %d)\n", err);
			return;
		}
		printk("done.\n");

		k_sleep(K_SECONDS(10));

		printk("Start Periodic Advertising...");
		err = bt_le_per_adv_start(adv);
		if (err) {
			printk("Failed to start Periodic Advertising "
			       "(err %d)\n", err);
			return;
		}
		printk("done.\n");

		k_sleep(K_SECONDS(10));

		printk("Stop Extended Advertising...");
		err = bt_le_ext_adv_stop(adv);
		if (err) {
			printk("Failed to stop extended advertising "
			       "(err %d)\n", err);
			return;
		}
		printk("done.\n");

		k_sleep(K_SECONDS(10));
	}
}
