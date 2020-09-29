/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF51 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF51 family processor.
 */

#include <kernel.h>
#include <init.h>
#include <hal/nrf_power.h>
#include <soc/nrfx_coredep.h>
#include <logging/log.h>

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

#include <system_nrf51.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/* Overrides the weak ARM implementation:
   Set general purpose retention register and reboot */
void sys_arch_reboot(int type)
{
	nrf_power_gpregret_set(NRF_POWER, (uint8_t)type);
	NVIC_SystemReset();
}

static int nordicsemi_nrf51_init(const struct device *arg)
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

#define DELAY_CALL_OVERHEAD_US 2

void arch_busy_wait(uint32_t time_us)
{
	if (time_us <= DELAY_CALL_OVERHEAD_US) {
		return;
	}

	time_us -= DELAY_CALL_OVERHEAD_US;
	nrfx_coredep_delay_us(time_us);
}

void z_platform_init(void)
{
	SystemInit();
}

SYS_INIT(nordicsemi_nrf51_init, PRE_KERNEL_1, 0);
