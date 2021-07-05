/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Raspberry Pi RP2040 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Raspberry Pi RP2040 family processor.
 */

#include <kernel.h>
#include <init.h>
#include <logging/log.h>

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

//#include <system_nrf51.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/* Overrides the weak ARM implementation:
   Set general purpose retention register and reboot */
void sys_arch_reboot(int type)
{
	NVIC_SystemReset();
}

static int raspberry_pi_rp2040_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

void z_platform_init(void)
{
	SystemInit();
}

SYS_INIT(raspberry_pi_rp2040_init, PRE_KERNEL_1, 0);
