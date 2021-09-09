/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <drivers/pinmux.h>

#include <soc.h>

struct pinmux_ports_t {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_000_036), okay)
	const struct device *porta;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_040_076), okay)
	const struct device *portb;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_100_136), okay)
	const struct device *portc;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_140_176), okay)
	const struct device *portd;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_200_236), okay)
	const struct device *porte;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_240_276), okay)
	const struct device *portf;
#endif
};

static void brd_init_pinmux_ports(struct pinmux_ports_t *pp)
{
	ARG_UNUSED(pp);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_000_036), okay)
	pp->porta = DEVICE_DT_GET(DT_NODELABEL(pinmux_000_036));

	__ASSERT_NO_MSG(device_is_ready(pp->porta));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_040_076), okay)
	pp->portb = DEVICE_DT_GET(DT_NODELABEL(pinmux_040_076));

	__ASSERT_NO_MSG(device_is_ready(pp->portb));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_100_136), okay)
	pp->portc = DEVICE_DT_GET(DT_NODELABEL(pinmux_100_136));

	__ASSERT_NO_MSG(device_is_ready(pp->portc));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_140_176), okay)
	pp->portd = DEVICE_DT_GET(DT_NODELABEL(pinmux_140_176));

	__ASSERT_NO_MSG(device_is_ready(pp->portd));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_200_236), okay)
	pp->porte = DEVICE_DT_GET(DT_NODELABEL(pinmux_200_236));

	__ASSERT_NO_MSG(device_is_ready(pp->porte));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_240_276), okay)
	pp->portf = DEVICE_DT_GET(DT_NODELABEL(pinmux_240_276));

	__ASSERT_NO_MSG(device_is_ready(pp->portf));
#endif
}

static void brd_cfg_uart(struct pinmux_ports_t *pp)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	pinmux_pin_set(pp->portc, MCHP_GPIO_104, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portc, MCHP_GPIO_105, MCHP_GPIO_CTRL_MUX_F1);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	pinmux_pin_set(pp->portd, MCHP_GPIO_170, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portd, MCHP_GPIO_171, MCHP_GPIO_CTRL_MUX_F1);
#endif
}

/* caller passes dev = NULL */
static int board_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct pinmux_ports_t pp;

	brd_init_pinmux_ports(&pp);
	brd_cfg_uart(&pp);

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
