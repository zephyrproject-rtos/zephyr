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

#if defined(CONFIG_ARM_MPU)
extern void arm_core_mpu_disable(void);
#endif

static const uint32_t bootloader = DT_REG_ADDR(DT_INST(0, st_stm32_bootloader));
static FUNC_NORETURN void jump_to_bootloader(void)
{
	int i;
	void (*jmp)(void);

	__disable_irq();

	for (i = 0; i < ARRAY_SIZE(NVIC->ICER); i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

#if defined(CONFIG_ARM_MPU)
	/*
	 * Needed to allow the bootloader to erase/write to flash, and the MPU
	 * is set up in the arch init phase.
	 */
	arm_core_mpu_disable();
#endif

	/*
	 * Disable SysTick before jumping, to keep the bootloader happy
	 */
	SysTick->CTRL = 0;

	LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SYSTEMFLASH);

	jmp = (void (*)(void))(void (*)(void))(*((uint32_t *)((bootloader + 4))));

	/*
	 * We need to clear a few things set by the Zephyr early startup code
	 */
	__set_CONTROL(0);
#if !defined(CONFIG_CPU_CORTEX_M0)
	__set_BASEPRI(0);
#endif

	__set_MSP(*(uint32_t *)bootloader);

	__enable_irq();

	jmp();

	CODE_UNREACHABLE;
}

static int bootloader_check_boot_init(void)
{
	if (bootmode_check(BOOT_MODE_TYPE_BOOTLOADER) > 0) {
		bootmode_clear();
		jump_to_bootloader();
	}

	return 0;
}

SYS_INIT(bootloader_check_boot_init, POST_KERNEL, CONFIG_STM32_BOOTLOADER_INIT_PRIORITY);
