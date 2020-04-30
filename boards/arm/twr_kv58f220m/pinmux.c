/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int twr_kv58f220m_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTA
	__unused const struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTB
	__unused const struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTC
	__unused const struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	__unused const struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTE
	__unused const struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

	/* LEDs */
	pinmux_pin_set(porte, 11, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 29, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 30, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Buttons */
	pinmux_pin_set(porta, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT1, INT2 */
	pinmux_pin_set(portc, 18, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 19, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	/* I2C1 SCL, SDA */
	pinmux_pin_set(portd, 8, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portd, 9, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) && CONFIG_SERIAL
	/* UART0 RX, TX */
	pinmux_pin_set(portb, 0, PORT_PCR_MUX(kPORT_MuxAlt7));
	pinmux_pin_set(portb, 1, PORT_PCR_MUX(kPORT_MuxAlt7));
#endif

	return 0;
}

SYS_INIT(twr_kv58f220m_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
