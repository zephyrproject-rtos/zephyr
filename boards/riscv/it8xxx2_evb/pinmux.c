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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmuxb), okay)
	const struct device *portb = DEVICE_DT_GET(DT_NODELABEL(pinmuxb));

	__ASSERT_NO_MSG(device_is_ready(portb));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmuxh), okay)
	const struct device *porth = DEVICE_DT_GET(DT_NODELABEL(pinmuxh));

	__ASSERT_NO_MSG(device_is_ready(porth));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* SIN0 */
	pinmux_pin_set(portb, 0, IT8XXX2_PINMUX_FUNC_3);
	/* SOUT0 */
	pinmux_pin_set(portb, 1, IT8XXX2_PINMUX_FUNC_3);
	/* Pullup SIN0 to received data */
	pinmux_pin_pullup(portb, 0, PINMUX_PULLUP_ENABLE);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* SIN1 */
	pinmux_pin_set(porth, 1, IT8XXX2_PINMUX_FUNC_4);
	/* SOUT1 */
	pinmux_pin_set(porth, 2, IT8XXX2_PINMUX_FUNC_4);
	/* Pullup SIN1 to received data */
	pinmux_pin_pullup(porth, 1, PINMUX_PULLUP_ENABLE);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */

	return 0;
}
SYS_INIT(it8xxx2_evb_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
