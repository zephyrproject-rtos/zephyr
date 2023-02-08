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

/** Value stored in backup SRAM. */
__stm32_backup_sram_section uint32_t backup_value;

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(BACKUP_DEV_COMPAT);

	if (!device_is_ready(dev)) {
		printk("ERROR: BackUp SRAM device is not ready\n");
		return;
	}

	printk("Current value in backup SRAM (%p): %d\n", &backup_value, backup_value);

	backup_value++;
#if __DCACHE_PRESENT
	SCB_CleanDCache_by_Addr(&backup_value, sizeof(backup_value));
#endif

	printk("Next reported value should be: %d\n", backup_value);
	printk("Keep VBAT power source and reset the board now!\n");
	return 0;
}
