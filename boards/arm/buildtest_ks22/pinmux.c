/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int buildtest_ks22_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);
#ifdef CONFIG_PINMUX_MCUX_PORTA
	struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTB
	struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTC
	struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTE
	struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

	/* Red, green, blue LEDs */
	pinmux_pin_set(portb, 0, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 6, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* SW1, SW2, SW3, SW4 */
	pinmux_pin_set(portb, 19, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 0, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 2, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(uart0))
	/* LPUART0 RX, TX */
	pinmux_pin_set(portc, 3, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 4, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(uart1))
	#error "No UART1 is used"
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(uart2))
	#error "No UART2 is used"
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(uart3))
	#error "No UART3 is used"
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(i2c0))
	/* I2C0 SCL, SDA */
	pinmux_pin_set(portb, 2, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portb, 3, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(spi0))
	/* SPI0 CS0, SCK, SOUT, SIN */
	pinmux_pin_set(portd, 0, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(can0))
	/* CAN0 Tx, Rx */
	pinmux_pin_set(porta, 12, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porta, 13, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

	return 0;
}

SYS_INIT(buildtest_ks22_pinmux_init, PRE_KERNEL_1,
	CONFIG_PINMUX_INIT_PRIORITY);
