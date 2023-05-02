/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

typedef void (*entry_t)(void);

void start_fastboot(void)
{
	entry_t entry = (entry_t)(CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET);

	/* switch U-MODE -> M-MODE */
	__asm__ __volatile__("ecall");

	/* Here we go */
	entry();
}
