/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_mesh.h"
#include "storage.h"

static u8_t STORAGE_ID;
u8_t reset_counter;

static void save_reset_counter(void)
{
	char buf[4];

	settings_str_from_value(SETTINGS_INT8, &reset_counter, buf,
				sizeof(buf));

	settings_save_one("ps/1", buf);
}

static void storage_work_handler(struct k_work *work)
{
	switch (STORAGE_ID) {
	case RESET_COUNTER:
		save_reset_counter();
		break;
	}
}

K_WORK_DEFINE(storage_work, storage_work_handler);

void save_on_flash(u8_t id)
{
	STORAGE_ID = id;
	k_work_submit(&storage_work);
}

static int ps_set(int argc, char **argv, char *val)
{
	if (argc == 1) {
		if (!strcmp(argv[0], "1")) {
		    return SETTINGS_VALUE_SET(val, SETTINGS_INT8,
					      reset_counter);
		}
	}

	return -ENOENT;
}

static struct settings_handler ps_settings = {
	.name = "ps",
	.h_set = ps_set,
};

int ps_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		printk("settings_subsys_init failed (err %d)", err);
		return err;
	}

	err = settings_register(&ps_settings);
	if (err) {
		printk("ps_settings_register failed (err %d)", err);
		return err;
	}

	return 0;
}
