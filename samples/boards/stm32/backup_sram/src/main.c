/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/** Value stored in backup SRAM. */
__stm32_backup_sram_section uint32_t backup_value;

void main(void)
{
	printk("Current value in backup SRAM: %d\n", backup_value);

	backup_value++;
#if __DCACHE_PRESENT
	SCB_CleanDCache_by_Addr(&backup_value, sizeof(backup_value));
#endif

	printk("Next reported value should be: %d\n", backup_value);
	printk("Keep VBAT power source and reset the board now!\n");
}
