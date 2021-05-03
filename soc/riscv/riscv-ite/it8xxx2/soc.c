/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

static int ite_it8xxx2_init(const struct device *arg)
{
	ARG_UNUSED(arg);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* UART1 board init */
	/* bit2: clocks to UART1 modules are not gated. */
	IT8XXX2_ECPM_CGCTRL3R &= ~BIT(2);
	IT8XXX2_ECPM_AUTOCG &= ~BIT(6);

	/* bit3: UART1 belongs to the EC side. */
	IT83XX_GCTRL_RSTDMMC |= BIT(3);
	/* reset UART before config it */
	IT83XX_GCTRL_RSTC4 = BIT(1);

	/* switch UART1 on without hardware flow control */
	IT8XXX2_GPIO_GRC1 |= BIT(0);

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* UART2 board init */
	/* setting voltage 3.3v */
	IT8XXX2_GPIO_GRC21 &= ~(BIT(0) | BIT(1));
	/* bit2: clocks to UART2 modules are not gated. */
	IT8XXX2_ECPM_CGCTRL3R &= ~BIT(2);
	IT8XXX2_ECPM_AUTOCG &= ~BIT(5);

	/* bit3: UART2 belongs to the EC side. */
	IT83XX_GCTRL_RSTDMMC |= BIT(2);
	/* reset UART before config it */
	IT83XX_GCTRL_RSTC4 = BIT(2);

	/* switch UART2 on without hardware flow control */
	IT8XXX2_GPIO_GRC1 |= BIT(2);

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */
	return 0;
}
SYS_INIT(ite_it8xxx2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
