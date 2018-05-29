/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <fsl_port.h>

static int frdm_kl25z_pinmux_init(struct device *dev)
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
#if defined(CONFIG_PINMUX_MCUX_PORTC)
	struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif
#if defined(CONFIG_PINMUX_MCUX_PORTE)
	struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

#ifdef CONFIG_UART_MCUX_LPSCI_0
	/* UART0 RX, TX */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

	/* SW0 and SW1 */
	pinmux_pin_set(porta, 16, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, 17, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red, green, blue LEDs. */
	pinmux_pin_set(portd,  1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 18, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 19, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if defined(CONFIG_I2C_0)
	/* I2C0 SCL, SDA */
	pinmux_pin_set(porte,  24, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_PS_MASK);
	pinmux_pin_set(porte,  25, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_PS_MASK);
#endif

#if CONFIG_ADC_0
	/* ADC0_SE12 */
	pinmux_pin_set(portb,  2, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

	return 0;
}

SYS_INIT(frdm_kl25z_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
