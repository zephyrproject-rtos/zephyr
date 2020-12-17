/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int ip_k66f_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(porta), okay)
	__unused const struct device *porta =
		device_get_binding(DT_LABEL(DT_NODELABEL(porta)));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portb), okay)
	__unused const struct device *portb =
		device_get_binding(DT_LABEL(DT_NODELABEL(portb)));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portc), okay)
	__unused const struct device *portc =
		device_get_binding(DT_LABEL(DT_NODELABEL(portc)));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portd), okay)
	__unused const struct device *portd =
		device_get_binding(DT_LABEL(DT_NODELABEL(portd)));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(porte), okay)
	__unused const struct device *porte =
		device_get_binding(DT_LABEL(DT_NODELABEL(porte)));
#endif

	/* Red0, Red2 LEDs */
	pinmux_pin_set(porta, 8, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, 10, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && CONFIG_NET_L2_ETHERNET
	pinmux_pin_set(porta, 12, PORT_PCR_MUX(kPORT_MuxAlt4));/* RMII_RXD1 */
	pinmux_pin_set(porta, 13, PORT_PCR_MUX(kPORT_MuxAlt4));/* RMII_RXD0 */
	pinmux_pin_set(porta, 14, PORT_PCR_MUX(kPORT_MuxAlt4));/* RMII_CRS_DV */
	pinmux_pin_set(porta, 15, PORT_PCR_MUX(kPORT_MuxAlt4));/* RMII_RX_EN */
	pinmux_pin_set(porta, 16, PORT_PCR_MUX(kPORT_MuxAlt4));/* RMII_TXD0 */
	pinmux_pin_set(porta, 17, PORT_PCR_MUX(kPORT_MuxAlt4));/* RMII_TXD1 */
	pinmux_pin_set(porta, 24, PORT_PCR_MUX(kPORT_MuxAsGpio));/* !ETH_RST */
	pinmux_pin_set(porta, 25, PORT_PCR_MUX(kPORT_MuxAsGpio));/* !ETH_PME */
	pinmux_pin_set(porta, 26, PORT_PCR_MUX(kPORT_MuxAsGpio));/* !ETH_INT */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(porte), okay)
	/* RMII_REF_CLK */
	pinmux_pin_set(porte, 26, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay) && CONFIG_SPI
	/* SPI1 CS0, SCK, SOUT, SIN - Control of KSZ8794 */
	pinmux_pin_set(portb, 10, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 11, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

	return 0;
}

SYS_INIT(ip_k66f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
