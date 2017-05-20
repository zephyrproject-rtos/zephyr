/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <gpio.h>
#include <fsl_port.h>

#include "board.h"

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

	/* Red, green, blue LEDs */
	pinmux_pin_set(portc,  RED_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc,  BLUE_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd,  GREEN_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Haptic feedback motor */
	pinmux_pin_set(portb,  HAPTIC_MOTOR_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));


#ifdef CONFIG_GPIO

#ifdef CONFIG_GPIO_MCUX_PORTA_NAME
#if defined(CONFIG_MAX30101)
	struct device *gpioa = device_get_binding(CONFIG_GPIO_MCUX_PORTA_NAME);
#endif
#endif

#ifdef CONFIG_GPIO_MCUX_PORTB_NAME
#if defined(CONFIG_HAPTIC_FEEDBACK) || defined(CONFIG_I2C_0)
	struct device *gpiob = device_get_binding(CONFIG_GPIO_MCUX_PORTB_NAME);
#endif
#endif

#ifdef CONFIG_GPIO_MCUX_PORTC_NAME
	struct device *gpioc = device_get_binding(CONFIG_GPIO_MCUX_PORTC_NAME);
#endif

#ifdef CONFIG_GPIO_MCUX_PORTD_NAME
	struct device *gpiod = device_get_binding(CONFIG_GPIO_MCUX_PORTD_NAME);
#endif

	/* These GPIO lines are hardwired inside the hexiwear device.
	 * There's little point in not setting these up properly, except
	 * that I suppose this is a handful of cycles spent at initialization
	 * time that might not need to be spent quite yet.
	 */
#ifdef CONFIG_GPIO_MCUX_PORTC_NAME
	gpio_pin_configure(gpioc, RED_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpioc, RED_GPIO_PIN, 1);

	gpio_pin_configure(gpioc, BLUE_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpioc, BLUE_GPIO_PIN, 1);
#endif

#ifdef CONFIG_GPIO_MCUX_PORTD_NAME
	gpio_pin_configure(gpiod, GREEN_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpiod, GREEN_GPIO_PIN, 1);
#endif

#ifdef CONFIG_HAPTIC_FEEDBACK
	gpio_pin_configure(gpiob, HAPTIC_MOTOR_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpiob, HAPTIC_MOTOR_PIN, 0);
#endif

#endif /* CONFIG_GPIO */

#ifdef CONFIG_HEXIWEAR_DOCKING_STATION
	/* these are not set up here with a gpio_pin_configure, because the
	 * docking station has BOTH LEDs and buttons for each of these
	 * pins.  It's really up to the developer to configure them based
	 * on the specific needs of that application.
	 */
	pinmux_pin_set(porta, CLICK1_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, CLICK2_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, CLICK3_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));

	pinmux_pin_set(portb, CLICK1_AN_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, CLICK1_PWM_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, CLICK1_INT_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));

	pinmux_pin_set(portb, CLICK2_AN_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, CLICK2_PWM_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, CLICK2_INT_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));

	pinmux_pin_set(portb, CLICK3_AN_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, CLICK3_PWM_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, CLICK3_INT_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#ifdef CONFIG_SPI_0
	/* SPI0 SCK, MOSI, MISO */
	pinmux_pin_set(portc, SPI_0_SCK_PIN,  PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, SPI_0_MOSI_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, SPI_0_MISO_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));
	/* Click slot Chip-select lines - SPI0_PCS0-2 */
	pinmux_pin_set(portc, CLICK1_CS_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, CLICK2_CS_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, CLICK3_CS_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#ifdef CONFIG_SPI_1
	/* SPI1 SCK, MOSI, MISO */
	pinmux_pin_set(portd, SPI_1_SCK_PIN,  PORT_PCR_MUX(kPORT_MuxAlt7));
	pinmux_pin_set(portd, SPI_1_MOSI_PIN, PORT_PCR_MUX(kPORT_MuxAlt7));
	pinmux_pin_set(portd, SPI_1_MISO_PIN, PORT_PCR_MUX(kPORT_MuxAlt7));

	/* SPI1 SPI1_PCSx */
	pinmux_pin_set(portd, 4, PORT_PCR_MUX(kPORT_MuxAlt7));
#endif

#ifdef CONFIG_SPI_2
	/* SPI2 SCK, MOSI, MISO */
	pinmux_pin_set(portd, SPI_2_SCK_PIN,  PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, SPI_2_MOSI_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, SPI_2_MISO_PIN, PORT_PCR_MUX(kPORT_MuxAlt2));

	/* SPI2 SPI2_PCSx */
	pinmux_pin_set(portb, 20, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if CONFIG_I2C_0
	/* I2C0 SCL, SDA - heart rate, light, humidity */
	pinmux_pin_set(portb,  0, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portb,  1, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);

	/* 3V3B_EN */
	pinmux_pin_set(portb, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));

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
	pinmux_pin_set(porta, MAX30101_GPIO_PIN, PORT_PCR_MUX(kPORT_MuxAsGpio));


	gpio_pin_configure(gpioa, MAX30101_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpioa, MAX30101_GPIO_PIN, 1);
#endif

	return 0;
}

SYS_INIT(hexiwear_k64_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
