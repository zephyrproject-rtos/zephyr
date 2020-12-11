/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <drivers/gpio.h>
#include <fsl_port.h>

static int hexiwear_k64_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTB
	const struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTC
	const struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	const struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTE
	const struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm3), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* Red, green, blue LEDs as PWM channels */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd,  0, PORT_PCR_MUX(kPORT_MuxAlt4));
#else
	/* Red, green, blue LEDs as GPIOs */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd,  0, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay) && CONFIG_I2C
	/* I2C0 SCL, SDA - heart rate, light, humidity */
	pinmux_pin_set(portb,  0, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portb,  1, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);

	/* 3V3B_EN */
	pinmux_pin_set(portb, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));

	const struct device *gpiob =
	       device_get_binding(DT_LABEL(DT_NODELABEL(gpiob)));

	gpio_pin_configure(gpiob, 12, GPIO_OUTPUT_LOW);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	/* I2C1 SCL, SDA - accel/mag, gyro, pressure */
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portc, 11, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
#endif
	/* FXAS21002 INT2 */
	pinmux_pin_set(portc, 18, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT2 */
	pinmux_pin_set(portd, 13, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) && CONFIG_SERIAL
	/* UART0 RX, TX */
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) && CONFIG_SERIAL
	/* UART4 RX, TX - BLE */
	pinmux_pin_set(porte, 24, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porte, 25, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if defined(CONFIG_MAX30101) && DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
	const struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);

	/* LDO - MAX30101 power supply */
	pinmux_pin_set(porta, 29, PORT_PCR_MUX(kPORT_MuxAsGpio));

	const struct device *gpioa =
	       device_get_binding(DT_LABEL(DT_NODELABEL(gpioa)));

	gpio_pin_configure(gpioa, 29, GPIO_OUTPUT_HIGH);
#endif

#ifdef CONFIG_BATTERY_SENSE
	pinmux_pin_set(portc, 14, PORT_PCR_MUX(kPORT_MuxAsGpio));

	const struct device *gpioc =
	       device_get_binding(DT_LABEL(DT_NODELABEL(gpioc)));

	gpio_pin_configure(gpioc, 14, GPIO_OUTPUT_LOW);
#endif

	return 0;
}

SYS_INIT(hexiwear_k64_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
