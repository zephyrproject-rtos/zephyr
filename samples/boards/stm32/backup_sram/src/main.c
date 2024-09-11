/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_backup_sram)
#define BACKUP_DEV_COMPAT st_stm32_backup_sram
#endif

#define BACKUP_MAGIC 0x600DCE11

struct backup_store {
	uint32_t value;
	uint32_t magic;
};

/** Value stored in backup SRAM. */
__stm32_backup_sram_section struct backup_store backup;

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(BACKUP_DEV_COMPAT);

	if (!device_is_ready(dev)) {
		printk("ERROR: BackUp SRAM device is not ready\n");
		return 0;
	}

	if (backup.magic != BACKUP_MAGIC) {
		backup.magic = BACKUP_MAGIC;
		backup.value = 0;
		printk("Invalid magic in backup SRAM structure - resetting value.\n");
	}

	printk("Current value in backup SRAM (%p): %d\n", &backup.value, backup.value);

	backup.value++;
#if __DCACHE_PRESENT
	SCB_CleanDCache_by_Addr(&backup, sizeof(backup));
#endif

	printk("Next reported value should be: %d\n", backup.value);
	printk("Keep VBAT power source and reset the board now!\n");
	return 0;
}
