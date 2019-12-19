/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int frdm_k32l2b3_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
	struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
	struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);

#ifdef CONFIG_UART_MCUX_LPUART_0
	/* UART0 RX, TX */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

	/* SW1 and SW3 */
	pinmux_pin_set(porta, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, 17, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red and green LEDs. */
	pinmux_pin_set(porte, 31, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* MMA8451 INT1 */
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if defined(CONFIG_I2C_0)
	/* I2C0 SCL, SDA */
	pinmux_pin_set(porte,  24, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_PS_MASK);
	pinmux_pin_set(porte,  25, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_PS_MASK);
#endif

	return 0;
}

SYS_INIT(frdm_k32l2b3_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
