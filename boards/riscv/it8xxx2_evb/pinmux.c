/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int it8xxx2_evb_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *p = DEVICE_DT_GET(DT_NODELABEL(pinmux));

	__ASSERT_NO_MSG(device_is_ready(p));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	pinmux_pin_set(p, 0, IT8XXX2_PINMUX_IOF1);
	pinmux_pin_set(p, 56, IT8XXX2_PINMUX_IOF1);
#endif	/* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	pinmux_pin_set(p, 3, IT8XXX2_PINMUX_IOF1);
	pinmux_pin_set(p, 59, IT8XXX2_PINMUX_IOF1);
#endif	/* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */
	return 0;
}

SYS_INIT(it8xxx2_evb_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
