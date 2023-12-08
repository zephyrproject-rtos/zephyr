/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

/* List of supported CCLK frequencies */
#define CLK_160MHZ                        160000000u

/* Check System Clock value. */
#if (DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_160MHZ)
	#error "Unsupported clock-frequency. Supported values: 160 MHz"
#endif

#define SYS_BASE                          0xf1700000
#define SYS_CORE_RESET_CTRL               0x218
#define __write_reg32(addr, value)        (*(volatile uint32_t *)(addr) = (uint32_t)(value))

/**
 * @brief Perform basic initialization at boot.
 *
 * @return 0
 */
static int soc_w91_init(void)
{
	/* Done by FreeRTOS clock_init() and clock_postinit() */
	/* hal/soc/scm2010/soc.c (hal/drivers/clk/clk-scm2010.c) */
	return 0;
}

/**
 * @brief Reset the system.
 */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	__write_reg32(SYS_BASE + SYS_CORE_RESET_CTRL, 1 << (csr_read(mhartid) * 4));
}

SYS_INIT(soc_w91_init, PRE_KERNEL_1, 0);
