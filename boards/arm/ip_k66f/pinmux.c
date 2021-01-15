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
		DEVICE_DT_GET(DT_NODELABEL(porta));
	__ASSERT_NO_MSG(device_is_ready(porta));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portb), okay)
	__unused const struct device *portb =
		DEVICE_DT_GET(DT_NODELABEL(portb));
	__ASSERT_NO_MSG(device_is_ready(portb));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portc), okay)
	__unused const struct device *portc =
		DEVICE_DT_GET(DT_NODELABEL(portc));
	__ASSERT_NO_MSG(device_is_ready(portc));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portd), okay)
	__unused const struct device *portd =
		DEVICE_DT_GET(DT_NODELABEL(portd));
	__ASSERT_NO_MSG(device_is_ready(portd));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(porte), okay)
	__unused const struct device *porte =
		DEVICE_DT_GET(DT_NODELABEL(porte));
	__ASSERT_NO_MSG(device_is_ready(porte));
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
