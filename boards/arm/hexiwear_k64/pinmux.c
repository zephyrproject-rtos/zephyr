/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <gpio.h>
#include <fsl_port.h>

static int hexiwear_k64_pinmux_init(struct device *dev)
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

#ifdef CONFIG_PWM_3
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

#if CONFIG_I2C_0
	/* I2C0 SCL, SDA - heart rate, light, humidity */
	pinmux_pin_set(portb,  0, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portb,  1, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);

	/* 3V3B_EN */
	pinmux_pin_set(portb, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));

	struct device *gpiob = device_get_binding(GPIO_B_LABEL);

	gpio_pin_configure(gpiob, 12, GPIO_DIR_OUT);
	gpio_pin_write(gpiob, 12, 0);
#endif

#if CONFIG_I2C_1
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

#ifdef CONFIG_UART_MCUX_0
	/* UART0 RX, TX */
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#ifdef CONFIG_UART_MCUX_4
	/* UART4 RX, TX - BLE */
	pinmux_pin_set(porte, 24, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porte, 25, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#ifdef CONFIG_MAX30101
	/* LDO - MAX30101 power supply */
	pinmux_pin_set(porta, 29, PORT_PCR_MUX(kPORT_MuxAsGpio));

	struct device *gpioa = device_get_binding(GPIO_A_LABEL);

	gpio_pin_configure(gpioa, 29, GPIO_DIR_OUT);
	gpio_pin_write(gpioa, 29, 1);
#endif

#ifdef CONFIG_BATTERY_SENSE
	pinmux_pin_set(portc, 14, PORT_PCR_MUX(kPORT_MuxAsGpio));

	struct device *gpioc = device_get_binding(GPIO_C_LABEL);

	gpio_pin_configure(gpioc, 14, GPIO_DIR_OUT);
	gpio_pin_write(gpioc, 14, 0);
#endif

	return 0;
}

SYS_INIT(hexiwear_k64_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
