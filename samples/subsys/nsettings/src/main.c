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
#include <fs.h>

#ifdef CONFIG_NSETTINGS_DEFAULT_FS
#include <nffs/nffs.h>

/* NFFS work area strcut */
static struct nffs_flash_desc flash_desc;

/* mounting info */
static struct fs_mount_t nffs_mnt = {
	.type = FS_NFFS,
	.mnt_point = CONFIG_NSETTINGS_DEFAULT_FS_MNT,
	.fs_data = &flash_desc,
};
#endif /* CONFIG_NSETTINGS_DEFAULT_FS */

/* 1000 msec = 1 sec */
#define SLEEP_TIME      2000

u8_t reset_counter;

static int ps_set(int argc, char **argv, size_t len, read_cb read, void *cb_arg)
{
	int rc;
	u8_t test;

	if (argc == 1) {
		if (!strcmp(argv[0], "ra0")) {
			rc = read(&reset_counter, sizeof(reset_counter),
				  cb_arg);
			printk("Reset Counter ra0: %d\n", reset_counter);
		}
		if (!strcmp(argv[0], "ra1")) {
			printk("ra1 found\n");
		}
		if (!strcmp(argv[0], "ra2")) {
			printk("ra2 found\n");
		}
		if (!strcmp(argv[0], "rb")) {
			rc = read(&test, sizeof(test), cb_arg);
			printk("Reset Counter rb: %d\n", test);
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
	int rc;
#ifdef CONFIG_NSETTINGS_DEFAULT_FS
	struct device *flash_dev;
#endif /* CONFIG_NSETTINGS_DEFAULT_FS */


	u8_t test = 8;
	reset_counter = 0U;

#ifdef CONFIG_NSETTINGS_DEFAULT_FS
	flash_dev = device_get_binding(CONFIG_FS_NFFS_FLASH_DEV_NAME);
	if (!flash_dev) {
		printk("Error flash dev");
	}
	/* set backend storage dev */
	nffs_mnt.storage_dev = flash_dev;
	rc = fs_mount(&nffs_mnt);
	printk("Mounting nffs [%d]\n", rc);
#endif /* CONFIG_NSETTINGS_DEFAULT_FS */

	rc = ps_settings_init();
	printk("Initialize settings [%d]\n", rc);

	rc = settings_load();
	printk("Settings load [%d]\n", rc);

	while (reset_counter < 16) {

		reset_counter++;
		printk("Reset counter %u\n", reset_counter);
		if (reset_counter == 1) {
			settings_runtime_set("ps/rb", &test, sizeof(test));
		}
		settings_save_one("ps/ra0", &reset_counter,
				  sizeof(reset_counter));
		settings_save_one("ps/ra1", &reset_counter,
				  sizeof(reset_counter));
		settings_save_one("ps/ra2", &reset_counter,
				  sizeof(reset_counter));
		settings_save_one("ps/rb", &reset_counter,
				  sizeof(reset_counter));
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
		k_sleep(SLEEP_TIME);
		sys_reboot(0);
	}
	printk("Finished");
}
