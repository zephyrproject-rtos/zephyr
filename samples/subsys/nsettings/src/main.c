/*
 * NSETTINGS Sample for Zephyr
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <misc/reboot.h>
#include <device.h>
#include <string.h>
#include <nsettings/settings.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME      10000

u8_t reset_counter;

static int ps_set(int argc, char **argv, size_t len, read_cb read, void *cb_arg)
{
	int rc;

	if (argc == 1) {
		if (!strcmp(argv[0], "ra0")) {
			rc = read(&reset_counter, sizeof(reset_counter),
				  cb_arg);
			printk("ra0 found\n");
		}
		if (!strcmp(argv[0], "ra1")) {
			printk("ra1 found\n");
		}
		if (!strcmp(argv[0], "ra2")) {
			printk("ra2 found\n");
		}
		if (!strcmp(argv[0], "rb")) {
			printk("rb found\n");
		}
		return (len < 0) ? len : 0;
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
		printk("settings_subsys_init failed (err %d)\n", err);
		return err;
	}

	err = settings_register(&ps_settings);
	if (err) {
		printk("ps_settings_register failed (err %d)\n", err);
		return err;
	}
	return 0;
}

void main(void)
{
	reset_counter = 0U;
	ps_settings_init();
	settings_load();

	while (reset_counter < 6) {
		k_sleep(SLEEP_TIME);
		reset_counter++;
		printk("Reset counter %u\n", reset_counter);
		settings_save_one("ps/ra0", &reset_counter,
				  sizeof(reset_counter));
		settings_save_one("ps/ra1", &reset_counter,
				  sizeof(reset_counter));
		settings_save_one("ps/ra2", &reset_counter,
				  sizeof(reset_counter));
		settings_save_one("ps/rb", &reset_counter,
				  sizeof(reset_counter));
		if (reset_counter == 2) {
			settings_delete("ps/rb");
		}
		if (reset_counter == 4) {
			settings_delete("ps/r");
		}
		sys_reboot(0);
	}
	printk("Finished");
}
