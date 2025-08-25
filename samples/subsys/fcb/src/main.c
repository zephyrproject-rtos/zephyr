/*
 * FCB sample demonstrates how to use the FCB subsystem to store an integer value in FLASH
 * memory. The sample first retrieve information about sectors which will be used, then
 * initialize the FCB instance, tries to load the last value from memory, increments the
 * loaded/default value, and stores it in the FLASH, and restarts after 1 second.
 *
 * Copyright (c) 2025 Tomas Jurena
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fcb.h>

#define FCB_FLASH_AREA    storage_partition
#define FCB_FLASH_AREA_ID FIXED_PARTITION_ID(FCB_FLASH_AREA)

BUILD_ASSERT(FIXED_PARTITION_EXISTS(FCB_FLASH_AREA),
	     "Board does not have fixed partition 'storage_partition'\n");

static int inc_boot_cnt(struct fcb *fcbp, uint32_t boot_cnt)
{
	uint8_t data[MAX(fcbp->f_align, sizeof(boot_cnt))];
	struct fcb_entry loc;
	int rc;

	printk("Saving boot_cnt %u to FCB\n", boot_cnt);
	memset(data, fcbp->f_erase_value, sizeof(data));
	memcpy(data, &boot_cnt, sizeof(boot_cnt));

	rc = fcb_append(fcbp, sizeof(data), &loc);
	if (rc) {
		fcb_rotate(fcbp);
		rc = fcb_append(fcbp, sizeof(data), &loc);
		if (rc) {
			printk("Cannot set new default state\n");
			return rc;
		}
	}

	rc = flash_area_write(fcbp->fap, FCB_ENTRY_FA_DATA_OFF(loc), data, sizeof(data));
	if (rc) {
		printk("Flash write error\n");
		return rc;
	}

	rc = fcb_append_finish(fcbp, &loc);
	if (rc) {
		printk("Append finnish failed\n");
		return rc;
	}

	printk("New boot_cnt value written to flash\n");
	return rc;
}

int main(void)
{
	const struct device *flash_device = FIXED_PARTITION_DEVICE(FCB_FLASH_AREA);
	uint32_t n_sectors = CONFIG_FCB_SECTOR_CNT, boot_cnt = 0;
	struct flash_sector sectors[CONFIG_FCB_SECTOR_CNT] = {0};
	struct fcb_entry entry = {0};
	struct fcb fcb_inst = {0};
	int rc = 0;

	if (!device_is_ready(flash_device)) {
		printk("Flash device %s is not ready\n", flash_device->name);
		return 0;
	}

	rc = flash_area_get_sectors(FCB_FLASH_AREA_ID, &n_sectors, sectors);
	if (rc) {
		printk("Failed to retrieved the number of sectors (%d)\n", rc);
		return rc;
	}

	fcb_inst.f_sectors = sectors;
	fcb_inst.f_sector_cnt = n_sectors;

	rc = fcb_init(FCB_FLASH_AREA_ID, &fcb_inst);
	if (rc != 0) {
		printk("FCB initialization failure! rc: %d\n", rc);
		return rc;
	}

	rc = fcb_offset_last_n(&fcb_inst, 1, &entry);
	if (rc == 0) {
		rc = flash_area_read(fcb_inst.fap, FCB_ENTRY_FA_DATA_OFF(entry), &boot_cnt,
				     sizeof(boot_cnt));
		if (rc) {
			printk("Unable to read last state\n");
			return rc;
		}
	}

	printk("Booted %u times\n", boot_cnt);

	boot_cnt++;
	inc_boot_cnt(&fcb_inst, boot_cnt);

	printk("Rebooting in 1 second\n");
	k_sleep(K_SECONDS(1));
	sys_reboot(0);
}
