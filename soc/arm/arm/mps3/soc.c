/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/gpio/gpio_mmio32.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/linker/linker-defs.h>


/* Setup GPIO drivers for accessing FPGAIO registers */
#define FPGAIO_NODE(n) DT_INST(n, arm_mps3_fpgaio_gpio)
#define FPGAIO_INIT(n)						\
	GPIO_MMIO32_INIT(FPGAIO_NODE(n),			\
			DT_REG_ADDR(FPGAIO_NODE(n)),		\
			BIT_MASK(DT_PROP(FPGAIO_NODE(n), ngpios)))

/* We expect there to be 3 arm,mps3-fpgaio-gpio devices:
 * led0, button, and misc
 */
FPGAIO_INIT(0);
FPGAIO_INIT(1);
FPGAIO_INIT(2);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * @return 0
 */
static int arm_mps3_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_mps3_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
