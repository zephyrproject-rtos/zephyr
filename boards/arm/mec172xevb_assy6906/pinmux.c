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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay)

#if DT_PROP(DT_INST(0, microchip_xec_qmspi), port_sel) == 0
/* shared QMSPI port: Two chip selects, single, dual, or quad */
static void brd_cfg_qmspi(struct pinmux_ports_t *pp)
{
#if DT_PROP(DT_INST(0, microchip_xec_qmspi), chip_select) == 0
	pinmux_pin_set(pp->portb, MCHP_GPIO_055, MCHP_GPIO_CTRL_MUX_F2 |
						 MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
#else
	pinmux_pin_set(pp->porta, MCHP_GPIO_002, MCHP_GPIO_CTRL_MUX_F2 |
						 MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
#endif
	pinmux_pin_set(pp->portb, MCHP_GPIO_056, MCHP_GPIO_CTRL_MUX_F2);
	pinmux_pin_set(pp->porte, MCHP_GPIO_223, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->porte, MCHP_GPIO_224, MCHP_GPIO_CTRL_MUX_F2);
#if DT_PROP(DT_INST(0, microchip_xec_qmspi), lines) == 4
	pinmux_pin_set(pp->porte, MCHP_GPIO_227, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->porta, MCHP_GPIO_016, MCHP_GPIO_CTRL_MUX_F2);
#endif
}
#elif DT_PROP(DT_INST(0, microchip_xec_qmspi), port_sel) == 1
/* private QMSPI port: One chip select, single, dual, or quad */
static void brd_cfg_qmspi(struct pinmux_ports_t *pp)
{
	pinmux_pin_set(pp->portc, MCHP_GPIO_124, MCHP_GPIO_CTRL_MUX_F1 |
						 MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	pinmux_pin_set(pp->portc, MCHP_GPIO_125, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portc, MCHP_GPIO_121, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portc, MCHP_GPIO_122, MCHP_GPIO_CTRL_MUX_F1);
#if DT_PROP(DT_INST(0, microchip_xec_qmspi), lines) == 4
	pinmux_pin_set(pp->portc, MCHP_GPIO_123, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portc, MCHP_GPIO_126, MCHP_GPIO_CTRL_MUX_F1);
#endif
}
#elif DT_PROP(DT_INST(0, microchip_xec_qmspi), port_sel) == 2
/* internal QMSPI port. One chip select, single or dual */
static void brd_cfg_qmspi(struct pinmux_ports_t *pp)
{
	pinmux_pin_set(pp->porta, MCHP_GPIO_024, MCHP_GPIO_CTRL_MUX_F1 |
						 MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	pinmux_pin_set(pp->porta, MCHP_GPIO_023, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portf, MCHP_GPIO_245, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(pp->portf, MCHP_GPIO_243, MCHP_GPIO_CTRL_MUX_F1);
}
#else
BUILD_ASSERT(0, "QMSPI DT port_del illegal value!");
#endif /* DT_PROP(DT_INST(0, microchip_xec_qmspi), port_sel) */
#else
static void brd_cfg_qmspi(struct pinmux_ports_t *pp)
{
	ARG_UNUSED(pp);
}
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay) */

/* caller passes dev = NULL */
static int board_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct pinmux_ports_t pp;

	brd_init_pinmux_ports(&pp);
	brd_cfg_uart(&pp);
	brd_cfg_qmspi(&pp);

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
