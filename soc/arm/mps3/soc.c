/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio/gpio_mmio32.h>
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
