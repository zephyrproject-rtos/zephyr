/*
 * Copyright (c) 2017 Linaro Limited
 *
 * Initial contents based on soc/arm/ti_lm3s6965/soc.c which is:
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/cpu.h>
#include <gpio/gpio_mmio32.h>
#include <init.h>
#include <soc.h>

/* Setup GPIO drivers for accessing FPGAIO registers */
GPIO_MMIO32_INIT(fpgaio_led0, FPGAIO_LED0_GPIO_NAME,
				&__MPS2_FPGAIO->led0, FPGAIO_LED0_MASK);
GPIO_MMIO32_INIT(fpgaio_button, FPGAIO_BUTTON_GPIO_NAME,
				&__MPS2_FPGAIO->button, FPGAIO_BUTTON_MASK);
GPIO_MMIO32_INIT(fpgaio_misc, FPGAIO_MISC_GPIO_NAME,
				&__MPS2_FPGAIO->misc, FPGAIO_MISC_MASK);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * @return 0
 */
static int arm_mps2_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_mps2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
