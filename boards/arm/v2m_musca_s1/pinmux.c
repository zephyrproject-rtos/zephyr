/*
 * Copyright (c) 2019-2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinmux.h>
#include <soc.h>
#include <zephyr/sys/sys_io.h>

#define IOMUX_MAIN_INSEL        (0x868 >> 2)
#define IOMUX_MAIN_OUTSEL       (0x870 >> 2)
#define IOMUX_MAIN_OENSEL       (0x878 >> 2)
#define IOMUX_MAIN_DEFAULT_IN   (0x880 >> 2)
#define IOMUX_ALTF1_INSEL       (0x888 >> 2)
#define IOMUX_ALTF1_OUTSEL      (0x890 >> 2)
#define IOMUX_ALTF1_OENSEL      (0x898 >> 2)
#define IOMUX_ALTF1_DEFAULT_IN  (0x8A0 >> 2)
#define IOMUX_ALTF2_INSEL       (0x8A8 >> 2)
#define IOMUX_ALTF2_OUTSEL      (0x8B0 >> 2)
#define IOMUX_ALTF2_OENSEL      (0x8B8 >> 2)
#define IOMUX_ALTF2_DEFAULT_IN  (0x8C0 >> 2)

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
static void arm_musca_s1_pinmux_defaults(void)
{
}
#else
/*
 * Only configure pins if we are secure.  Otherwise secure violation will occur.
 */
static void arm_musca_s1_pinmux_defaults(void)
{
	volatile uint32_t *scc = (uint32_t *)DT_REG_ADDR(DT_INST(0, arm_scc));

	/* there is only altfunc1, so steer all alt funcs to use 1 */
	scc[IOMUX_ALTF1_INSEL]  = 0xffff;
	scc[IOMUX_ALTF1_OUTSEL] = 0xffff;
	scc[IOMUX_ALTF1_OENSEL] = 0xffff;

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	/* clear bit 0/1 for GPIO0/1 to steer from ALTF1 */
	scc[IOMUX_MAIN_INSEL]  &= ~(BIT(0) | BIT(1));
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

static int arm_musca_pinmux_init(const struct device *port)
{
	ARG_UNUSED(port);

	arm_musca_s1_pinmux_defaults();

	return 0;
}

SYS_INIT(arm_musca_pinmux_init, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
