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
#include <nrfx.h>
#include <soc/nrfx_coredep.h>

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

#include <system_nrf51.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

static int nordicsemi_nrf51_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	SystemInit();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

#define DELAY_CALL_OVERHEAD_US 2

void z_arch_busy_wait(u32_t time_us)
{
	if (time_us <= DELAY_CALL_OVERHEAD_US) {
		return;
	}

	time_us -= DELAY_CALL_OVERHEAD_US;
	nrfx_coredep_delay_us(time_us);
}

SYS_INIT(nordicsemi_nrf51_init, PRE_KERNEL_1, 0);
