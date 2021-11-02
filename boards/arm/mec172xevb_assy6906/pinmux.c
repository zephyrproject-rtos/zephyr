/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <drivers/clock_control/mchp_xec_clock_control.h>
#include <drivers/pinmux.h>

#include <soc.h>

enum gpio_ports {
	port_000_036 = 0,
	port_040_076,
	port_100_136,
	port_140_176,
	port_200_236,
	port_240_276,
	port_max,
};

struct pin_info {
	enum gpio_ports port_num;
	uint8_t pin;
	uint32_t flags;
};

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

const struct pin_info uart_pin_table[] = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	{ port_100_136, MCHP_GPIO_104, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_100_136, MCHP_GPIO_105, MCHP_GPIO_CTRL_MUX_F1 },
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	{ port_140_176, MCHP_GPIO_170, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_140_176, MCHP_GPIO_171, MCHP_GPIO_CTRL_MUX_F1 },
#endif
};

/* eSPI: Reset#, Alert#, CS#, CLK, IO0 - IO4 */
const struct pin_info espi_pin_table[] = {
#if defined(CONFIG_ESPI_XEC_V2) && DT_NODE_HAS_STATUS(DT_NODELABEL(espi0), okay)
	{ port_040_076, MCHP_GPIO_061, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_063, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_066, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_065, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_070, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_071, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_072, MCHP_GPIO_CTRL_MUX_F1 },
	{ port_040_076, MCHP_GPIO_073, MCHP_GPIO_CTRL_MUX_F1 },
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

const struct device *get_port_device(struct pinmux_ports_t *pp,
				     uint8_t port_num)
{
	switch (port_num) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_000_036), okay)
	case port_000_036:
		return pp->porta;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_040_076), okay)
	case port_040_076:
		return pp->portb;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_100_136), okay)
	case port_100_136:
		return pp->portc;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_140_176), okay)
	case port_140_176:
		return pp->portd;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_200_236), okay)
	case port_200_236:
		return pp->porte;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_240_276), okay)
	case port_240_276:
		return pp->portf;
#endif
	default:
		return NULL;
	}
}

static void brd_pin_table_init(struct pinmux_ports_t *pp,
			       const struct pin_info *table, size_t nentries)
{
	for (size_t n = 0; n < nentries; n++) {
		const struct device *dev =
			get_port_device(pp, table[n].port_num);

		if (!dev) {
			continue;
		}

		pinmux_pin_set(dev, table[n].pin, table[n].flags);
	}
}

/* caller passes dev = NULL */
static int board_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct pinmux_ports_t pp;

	brd_init_pinmux_ports(&pp);
	brd_pin_table_init(&pp, uart_pin_table, ARRAY_SIZE(uart_pin_table));
	brd_pin_table_init(&pp, espi_pin_table, ARRAY_SIZE(espi_pin_table));

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
