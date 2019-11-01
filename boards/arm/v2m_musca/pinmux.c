/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <drivers/pinmux.h>
#include <soc.h>
#include <sys/sys_io.h>
#include <gpio/gpio_cmsdk_ahb.h>

#include "pinmux/pinmux.h"

#define IOMUX_MAIN_INSEL	(0x30 >> 2)
#define IOMUX_MAIN_OUTSEL	(0x34 >> 2)
#define IOMUX_MAIN_OENSEL	(0x38 >> 2)
#define IOMUX_MAIN_DEFAULT_IN	(0x3c >> 2)
#define IOMUX_ALTF1_INSEL	(0x40 >> 2)
#define IOMUX_ALTF1_OUTSEL	(0x44 >> 2)
#define IOMUX_ALTF1_OENSEL	(0x48 >> 2)
#define IOMUX_ALTF1_DEFAULT_IN	(0x4c >> 2)
#define IOMUX_ALTF2_INSEL	(0x50 >> 2)
#define IOMUX_ALTF2_OUTSEL	(0x54 >> 2)
#define IOMUX_ALTF2_OENSEL	(0x58 >> 2)
#define IOMUX_ALTF2_DEFAULT_IN	(0x5c >> 2)

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
static void arm_musca_pinmux_defaults(void)
{
}
#else
/*
 * Only configure pins if we are secure.  Otherwise secure violation will occur
 */
static void arm_musca_pinmux_defaults(void)
{
	volatile u32_t *scc = (u32_t *)DT_ARM_SCC_BASE_ADDRESS;

	/* there is only altfunc1, so steer all alt funcs to use 1 */
	scc[IOMUX_ALTF1_INSEL] = 0xffff;
	scc[IOMUX_ALTF1_OUTSEL] = 0xffff;
	scc[IOMUX_ALTF1_OENSEL] = 0xffff;

#if defined(CONFIG_UART_PL011_PORT0)
	/* clear bit 0/1 for GPIO0/1 to steer from ALTF1 */
	scc[IOMUX_MAIN_INSEL] &= ~(BIT(0) | BIT(1));
	scc[IOMUX_MAIN_OUTSEL] &= ~(BIT(0) | BIT(1));
	scc[IOMUX_MAIN_OENSEL] &= ~(BIT(0) | BIT(1));
#endif
	/* Enable PINs for LEDS */
	scc[IOMUX_ALTF1_OUTSEL] &= ~(BIT(2) | BIT(3) | BIT(4));
	scc[IOMUX_ALTF1_OENSEL] &= ~(BIT(2) | BIT(3) | BIT(4));
	scc[IOMUX_ALTF2_OUTSEL] &= ~(BIT(2) | BIT(3) | BIT(4));
	scc[IOMUX_ALTF2_OENSEL] &= ~(BIT(2) | BIT(3) | BIT(4));
}
#endif

static int arm_musca_pinmux_init(struct device *port)
{
	ARG_UNUSED(port);

	arm_musca_pinmux_defaults();

	return 0;
}

SYS_INIT(arm_musca_pinmux_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
