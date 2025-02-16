/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/retention/retention.h>
#include <zephyr/retention/bootmode.h>
#include <stm32_ll_system.h>

static const uint32_t bootloader = DT_REG_ADDR(DT_INST(0, st_stm32_bootloader));
static FUNC_NORETURN void jump_to_bootloader(void)
{
	uint32_t i = 0;
	void (*jmp)(void);

	__disable_irq();

	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	for (i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++)
	{
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SYSTEMFLASH);

	jmp = (void (*)(void)) (void (*)(void)) (*((uint32_t *) ((bootloader + 4))));

	__set_CONTROL(0);
	__set_MSP(*(uint32_t *)bootloader);

	__enable_irq();

	jmp();

	while (1) { }
}

static int bootloader_check_boot_init(void)
{
	if (bootmode_check(BOOT_MODE_TYPE_BOOTLOADER) > 0) {
		bootmode_clear();
		jump_to_bootloader();
	}

	return 0;
}

SYS_INIT(bootloader_check_boot_init, EARLY, 0);
