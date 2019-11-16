/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int frdm_kl46z_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTA
	struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
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

#ifdef CONFIG_UART_MCUX_LPSCI_0
	/* UART0 RX, TX */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

	/* SW1 and SW3 */
	pinmux_pin_set(portc, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red and green LEDs. */
	pinmux_pin_set(portd,  5, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 29, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* MMA8451 INT1, INT2 */
	pinmux_pin_set(portc, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if defined(CONFIG_I2C_0)
	/* I2C0 SCL, SDA */
	pinmux_pin_set(porte,  24, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_PS_MASK);
	pinmux_pin_set(porte,  25, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_PS_MASK);
#endif

#if CONFIG_ADC_0
	/* ADC0_SE3 */
	pinmux_pin_set(porte,  22, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

	return 0;
}

SYS_INIT(frdm_kl46z_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
