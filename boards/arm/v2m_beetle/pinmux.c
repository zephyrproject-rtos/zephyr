/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <pinmux.h>
#include <soc.h>
#include <sys_io.h>
#include <gpio/gpio_cmsdk_ahb.h>

#include "pinmux/pinmux.h"

/**
 * @brief Pinmux driver for ARM V2M Beetle Board
 *
 * The ARM V2M Beetle Board has 4 GPIO controllers. These controllers
 * are responsible for pin muxing, input/output, pull-up, etc.
 *
 * The GPIO controllers 2 and 3 are reserved and therefore not exposed by
 * this driver.
 *
 * All GPIO controller exposed pins are exposed via the following sequence of
 * pin numbers:
 *   Pins  0 -  15 are for GPIO0
 *   Pins 16 -  31 are for GPIO1
 *
 * For the exposed GPIO controllers ARM V2M Beetle Board follows the Arduino
 * compliant pin out.
 */

#define CMSDK_AHB_GPIO0_DEV \
	((volatile struct gpio_cmsdk_ahb *)CMSDK_AHB_GPIO0)
#define CMSDK_AHB_GPIO1_DEV \
	((volatile struct gpio_cmsdk_ahb *)CMSDK_AHB_GPIO1)

/*
 * This is the mapping from the ARM V2M Beetle Board pins to GPIO
 * controllers.
 *
 * D0 : P0_0
 * D1 : P0_1
 * D2 : P0_2
 * D3 : P0_3
 * D4 : P0_4
 * D5 : P0_5
 * D6 : P0_6
 * D7 : P0_7
 * D8 : P0_8
 * D9 : P0_9
 * D10 : P0_10
 * D11 : P0_11
 * D12 : P0_12
 * D13 : P0_13
 * D14 : P0_14
 * D15 : P0_15
 * D16 : P1_0
 * D17 : P1_1
 * D18 : P1_2
 * D19 : P1_3
 * D20 : P1_4
 * D21 : P1_5
 * D22 : P1_6
 * D23 : P1_7
 * D24 : P1_8
 * D25 : P1_9
 * D26 : P1_10
 * D27 : P1_11
 * D28 : P1_12
 * D29 : P1_13
 * D30 : P1_14
 * D31 : P1_15
 *
 * UART_0_RX : D0
 * UART_0_TX : D1
 * SPI_0_CS : D10
 * SPI_0_MOSI : D11
 * SPI_0_MISO : D12
 * SPI_0_SCLK : D13
 * I2C_0_SCL : D14
 * I2C_0_SDA : D15
 * UART_1_RX : D16
 * UART_1_TX : D17
 * SPI_1_CS : D18
 * SPI_1_MOSI : D19
 * SPI_1_MISO : D20
 * SPI_1_SCK : D21
 * I2C_1_SDA : D22
 * I2C_1_SCL : D23
 *
 */
static void arm_v2m_beetle_pinmux_defaults(void)
{
	u32_t gpio_0 = 0;
	u32_t gpio_1 = 0;

	/* Set GPIO Alternate Functions */

	gpio_0 = (1<<0); /* Sheild 0 UART 0 RXD */
	gpio_0 |= (1<<1); /* Sheild 0 UART 0 TXD */
	gpio_0 |= (1<<14); /* Sheild 0 I2C SDA SBCON2 */
	gpio_0 |= (1<<15); /* Sheild 0 I2C SCL SBCON2 */
	gpio_0 |= (1<<10); /* Sheild 0 SPI_3 nCS */
	gpio_0 |= (1<<11); /* Sheild 0 SPI_3 MOSI */
	gpio_0 |= (1<<12); /* Sheild 0 SPI_3 MISO */
	gpio_0 |= (1<<13); /* Sheild 0 SPI_3 SCK */

	CMSDK_AHB_GPIO0_DEV->altfuncset = gpio_0;

	gpio_1 = (1<<0); /* UART 1 RXD */
	gpio_1 |= (1<<1); /* UART 1 TXD */
	gpio_1 |= (1<<6); /* Sheild 1 I2C SDA */
	gpio_1 |= (1<<7); /* Sheild 1 I2C SCL */
	gpio_1 |= (1<<2); /* ADC SPI_1 nCS */
	gpio_1 |= (1<<3); /* ADC SPI_1 MOSI */
	gpio_1 |= (1<<4); /* ADC SPI_1 MISO */
	gpio_1 |= (1<<5); /* ADC SPI_1 SCK */

	gpio_1 |= (1<<8); /* QSPI CS 2 */
	gpio_1 |= (1<<9); /* QSPI CS 1 */
	gpio_1 |= (1<<10); /* QSPI IO 0 */
	gpio_1 |= (1<<11); /* QSPI IO 1 */
	gpio_1 |= (1<<12); /* QSPI IO 2 */
	gpio_1 |= (1<<13); /* QSPI IO 3 */
	gpio_1 |= (1<<14); /* QSPI SCK */

	CMSDK_AHB_GPIO1_DEV->altfuncset = gpio_1;

	/* Set the ARD_PWR_EN GPIO1[15] as an output */
	CMSDK_AHB_GPIO1_DEV->outenableset |= (0x1 << 15);
	/* Set on 3v3 (for ARDUINO HDR compliancy) */
	CMSDK_AHB_GPIO1_DEV->data |= (0x1 << 15);
}

static int arm_v2m_beetle_pinmux_init(struct device *port)
{
	ARG_UNUSED(port);

	arm_v2m_beetle_pinmux_defaults();

	return 0;
}

SYS_INIT(arm_v2m_beetle_pinmux_init, PRE_KERNEL_1,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
