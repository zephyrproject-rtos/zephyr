/*
 * Copyright 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int rv32m1_vega_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_RV32M1_PORTA
	__unused struct device *porta =
		device_get_binding(CONFIG_PINMUX_RV32M1_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_RV32M1_PORTB
	__unused struct device *portb =
		device_get_binding(CONFIG_PINMUX_RV32M1_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_RV32M1_PORTC
	__unused struct device *portc =
		device_get_binding(CONFIG_PINMUX_RV32M1_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_RV32M1_PORTD
	__unused struct device *portd =
		device_get_binding(CONFIG_PINMUX_RV32M1_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_RV32M1_PORTE
	__unused struct device *porte =
		device_get_binding(CONFIG_PINMUX_RV32M1_PORTE_NAME);
#endif

#ifdef CONFIG_UART_RV32M1_LPUART_0
	/* LPUART0 RX, TX */
	pinmux_pin_set(portc, 7, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 8, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#ifdef CONFIG_UART_RV32M1_LPUART_1
	/* LPUART1 RX, TX */
	pinmux_pin_set(portc, 29, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 30, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if CONFIG_I2C_0
	/* LPI2C0 SCL, SDA - Arduino header */
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(portc, 9, PORT_PCR_MUX(kPORT_MuxAlt4));
#endif

#if CONFIG_I2C_3
	/* LPI2C3 SCL, SDA - FXOS8700 */
	pinmux_pin_set(porte, 30, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porte, 29, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

	/* FXOS8700 INT1, INT2, RST */
	pinmux_pin_set(porte, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 22, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 27, PORT_PCR_MUX(kPORT_MuxAsGpio));

	return 0;
}

SYS_INIT(rv32m1_vega_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
