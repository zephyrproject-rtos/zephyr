/*
 * SETTINGS Sample for Zephyr
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <misc/reboot.h>
#include <string.h>
#include <settings/settings.h>

#ifdef CONFIG_SETTINGS_DEFAULT_FS
#include <device.h>
#include <fs.h>
#include <nffs/nffs.h>

/* NFFS work area strcut */
static struct nffs_flash_desc flash_desc;

/* mounting info */
static struct fs_mount_t nffs_mnt = {
	.type = FS_NFFS,
	.mnt_point = CONFIG_SETTINGS_DEFAULT_FS_MNT,
	.fs_data = &flash_desc,
};
#endif /* CONFIG_SETTINGS_DEFAULT_FS */

#define SLEEP_TIME	K_SECONDS(2)

u8_t rst_cnt; /* reset counter */

static int ps_set(int argc, char **argv, size_t len, settings_read_fn read,
		  void *store)
{
	int rc;
	u8_t test;

	if (argc == 1) {
		if (!strcmp(argv[0], "rst_cnt")) {
			if (len) {
				rc = read(store, &rst_cnt, sizeof(rst_cnt));
				printk("Reset Counter: %d\n", rst_cnt);
			} else {
				/* catch deleted item */
				rst_cnt = 0;
				printk("Reset Counter deleted\n");
			}
		}
		if (!strcmp(argv[0], "ra0")) {
			if (len) {
				rc = read(store, &test, sizeof(test));
				printk("Entry ra0: %d\n", test);
			} else {
				printk("Entry ra0 deleted\n");
			}
		}
		if (!strcmp(argv[0], "ra1")) {
			if (len) {
				printk("Entry ra1 set\n");
			} else {
				printk("Entry ra1 deleted\n");
			}
		}
		if (!strcmp(argv[0], "ra2")) {
			if (len) {
				printk("Entry ra2 set\n");
			} else {
				printk("Entry ra2 deleted\n");
			}
		}
	}

	return (len < 0) ? len : 0;

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
#ifdef CONFIG_SETTINGS_DEFAULT_FS
	struct device *flash_dev;
#endif /* CONFIG_SETTINGS_DEFAULT_FS */
	u8_t test0 = 0U, test1 = 0U, test2 = 0U;

	rst_cnt = 0U;

#ifdef CONFIG_SETTINGS_DEFAULT_FS
	flash_dev = device_get_binding(CONFIG_FS_NFFS_FLASH_DEV_NAME);
	if (!flash_dev) {
		printk("Error flash dev");
	}
	/* set backend storage dev */
	nffs_mnt.storage_dev = flash_dev;
	rc = fs_mount(&nffs_mnt);
	printk("Mounting nffs [%d]\n", rc);
#endif /* CONFIG_SETTINGS_DEFAULT_FS */

	rc = ps_settings_init();
	printk("Initialize settings [%d]\n", rc);

	rc = settings_load();
	printk("Settings load [%d]\n", rc);

	while (rst_cnt < 6) {

		rst_cnt++;
		test0 = rst_cnt + 16;
		test1 = test0 + 16;
		test2 = test1 + 16;
		printk("Reset counter %u\n", rst_cnt);
		if (rst_cnt == 1) {
			settings_runtime_set("ps/ra0", &test0, sizeof(test0));
			settings_runtime_set("ps/ra1", &test1, sizeof(test1));
			settings_runtime_set("ps/ra2", &test2, sizeof(test2));
		}
		settings_save_one("ps/rst_cnt", &rst_cnt, sizeof(rst_cnt));
		settings_save_one("ps/ra0", &test0, sizeof(test0));
		settings_save_one("ps/ra1", &test1, sizeof(test1));
		settings_save_one("ps/ra2", &test2, sizeof(test2));
		if (rst_cnt == 2) {
			/* Delete one entry ps/ra0 */
			settings_delete("ps/ra0");
		}
		if (rst_cnt == 4) {
			/* Delete all entries that match ps/ra... */
			settings_delete("ps/ra");
		}
		k_sleep(SLEEP_TIME);
		sys_reboot(0);
	}
	printk("Finished");
}
