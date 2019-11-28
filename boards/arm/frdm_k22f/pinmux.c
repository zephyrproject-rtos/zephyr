/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <fsl_port.h>

static int frdm_k22f_pinmux_init(struct device *dev)
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

#ifdef CONFIG_UART_MCUX_0
#error "No UART0 is used"
#endif

#ifdef CONFIG_UART_MCUX_1
	/* UART1 RX, TX */
	pinmux_pin_set(porte, 0, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porte, 1, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#ifdef CONFIG_UART_MCUX_2
	/* UART2 RX, TX */
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

	/* SW2 */
	pinmux_pin_set(portc, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* SW3 */
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT1 */
	pinmux_pin_set(portd, 0, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* FXOS8700 INT2 */
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red, green, blue LEDs */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));

#ifdef CONFIG_SPI_0
	/* SPI0 CS0, SCK, SOUT, SIN */
	pinmux_pin_set(portd, 4, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 11, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if CONFIG_I2C_0
	/* I2C0 SCL, SDA */
	pinmux_pin_set(portb, 2, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portb, 3, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
#endif

	return 0;
}

SYS_INIT(frdm_k22f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
