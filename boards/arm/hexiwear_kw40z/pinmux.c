/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <fsl_port.h>

static int hexiwear_kw40z_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTB
	struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif

#ifdef CONFIG_PINMUX_MCUX_PORTC
	struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif

#ifdef CONFIG_UART_MCUX_LPUART_0
	/* UART0 RX, TX */
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(portc,  7, PORT_PCR_MUX(kPORT_MuxAlt4));
#endif

#if CONFIG_I2C_1
	/* I2C1 SCL, SDA */
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAlt3)
					| PORT_PCR_PS_MASK);
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAlt3)
					| PORT_PCR_PS_MASK);
#endif

#if CONFIG_ADC
	/* ADC0_SE1 */
	pinmux_pin_set(portb,  1, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

	return 0;
}

SYS_INIT(hexiwear_kw40z_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
