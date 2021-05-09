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
	/* uart1 board init */
	CGCTRL3R &= ~(BIT(2));
	AUTOCG &= ~(BIT(6));
	RSTDMMC |= BIT(3);	/* Set EC side control */
	RSTC4 = BIT(1);	/* W-One to reset controller */
	GPCRB0 = 0x00;		/* tx pin init */
	GPCRB1 = 0x00;		/* rx pin init */
	GPCRF3 = 0x00;		/* rts pin init */
	GPCRD5 = 0x00;		/* cts pin init */
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* uart2 board init */
	GCR21 &= ~(BIT(0) | BIT(1));	/* setting voltage 3.3v */
	CGCTRL3R &= ~(BIT(2));
	AUTOCG &= ~(BIT(5));
	RSTDMMC |= BIT(2);	/* Set EC side control */
	RSTC4 = BIT(2);	/* W-One to reset controller */
	GPCRH1 = 0x00;		/* tx pin init */
	GPCRH2 = 0x00;		/* rx pin init */
	GPCRE5 = 0x00;		/* rts pin init */
	GPCRI7 = 0x00;		/* cts pin init */
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */
	return 0;
}
SYS_INIT(ite_it8xxx2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
