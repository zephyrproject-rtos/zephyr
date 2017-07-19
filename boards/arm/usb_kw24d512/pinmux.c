/*
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <fsl_port.h>

static int usb_kw24d512_pinmux_init(struct device *dev)
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

#ifdef CONFIG_UART_MCUX_0
	/* UART0 RX, TX */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

	/* SW1 */
	pinmux_pin_set(portc, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* blue LEDs D2, D3 */
	pinmux_pin_set(portd, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));

#ifdef CONFIG_SPI_1
	/* SPI1 CS0, SCK, SOUT, SIN */
	pinmux_pin_set(portb, 10, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 11, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#ifdef CONFIG_IEEE802154_MCR20A
	/* Reset, IRQ_B */
	pinmux_pin_set(portb, 19, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif
	return 0;
}

SYS_INIT(usb_kw24d512_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
