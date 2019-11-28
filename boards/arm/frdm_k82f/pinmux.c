/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <fsl_port.h>

static int frdm_k82f_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTA
	__unused struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTB
	__unused struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTC
	__unused struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	__unused struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTE
	__unused struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

#ifdef CONFIG_PWM_3
	/* Red, green, blue LEDs as PWM channels */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAlt4));
#else
	/* Red, green, blue LEDs as GPIOs */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

	/* Buttons */
	pinmux_pin_set(porta, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 6, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT1 */
	pinmux_pin_set(portc, 13, PORT_PCR_MUX(kPORT_MuxAsGpio));

#ifdef CONFIG_I2C_3
	/* I2C3 SDA, SCL */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt4)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt4)
					| PORT_PCR_ODE_MASK);
#endif

#ifdef CONFIG_SPI_1
	/* SPI1 SCK, SOUT, SIN, PCS0 */
	pinmux_pin_set(porte, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 4, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 5, PORT_PCR_MUX(kPORT_MuxAlt2));
	/* SPI1 NOR RESET, WP */
	pinmux_pin_set(porte, 0, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#ifdef CONFIG_UART_MCUX_LPUART_4
	/* LPUART4 RX, TX */
	pinmux_pin_set(portc, 14, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 15, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

	return 0;
}

SYS_INIT(frdm_k82f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
