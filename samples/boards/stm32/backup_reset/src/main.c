/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stm32_ll_rcc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_backup_sram)
#define BACKUP_DEV_COMPAT st_stm32_backup_sram
#endif

/** Value stored in backup SRAM. */
__stm32_backup_sram_section uint32_t backup_value;

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(BACKUP_DEV_COMPAT);

	/*
	 * Detect power on and reset the backUp value.
	 * On stm32F2x or stm32F446 or stm32F7x or stm32F405-based
	 * (STM32F405/415, STM32F407/417, STM32F427/437, STM32F429/439)
	 * the BackUp SRAM is not resetted when the VBAT is off
	 * but when the protection level changes from 1 to 0.
	 * This sample forces resetting the content at power ON (whatever Vbat).
	 */
	if (LL_RCC_IsActiveFlag_BORRST()) {
		backup_value = 0;
		LL_RCC_ClearResetFlags();
	}

	if (!device_is_ready(dev)) {
		printk("ERROR: BackUp SRAM device is not ready\n");
		return 0;
	}

	printk("Current value in backup SRAM (%p): %d\n", &backup_value, backup_value);

	backup_value++;
#if __DCACHE_PRESENT
	SCB_CleanDCache_by_Addr(&backup_value, sizeof(backup_value));
#endif

	printk("Next reported value after reset should be: %d\n", backup_value);
	printk("Reset or Power off/on the board now!\n");
	return 0;
}
