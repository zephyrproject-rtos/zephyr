/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/gpio/gpio_cmsdk_ahb.h>

/**
 * @brief Pinmux driver for ARM MPS2 AN385 Board
 *
 * The ARM MPS2 AN385 Board has 4 GPIO controllers. These controllers
 * are responsible for pin muxing, input/output, pull-up, etc.
 *
 * All GPIO controller pins are exposed via the following sequence of pin
 * numbers:
 *   Pins  0 -  15 are for GPIO0
 *   Pins 16 -  31 are for GPIO1
 *   Pins 32 -  47 are for GPIO2
 *   Pins 48 -  51 are for GPIO3
 *
 * For the GPIO controllers configuration ARM MPS2 AN385 Board follows the
 * Arduino compliant pin out.
 */

#define CMSDK_AHB_GPIO0_DEV \
	((volatile struct gpio_cmsdk_ahb *)DT_REG_ADDR(DT_NODELABEL(gpio0)))
#define CMSDK_AHB_GPIO1_DEV \
	((volatile struct gpio_cmsdk_ahb *)DT_REG_ADDR(DT_NODELABEL(gpio1)))
#define CMSDK_AHB_GPIO2_DEV \
	((volatile struct gpio_cmsdk_ahb *)DT_REG_ADDR(DT_NODELABEL(gpio2)))
#define CMSDK_AHB_GPIO3_DEV \
	((volatile struct gpio_cmsdk_ahb *)DT_REG_ADDR(DT_NODELABEL(gpio3)))

/*
 * This is the mapping from the ARM MPS2 AN385 Board pins to GPIO
 * controllers.
 *
 * D0 : EXT_0
 * D1 : EXT_4
 * D2 : EXT_2
 * D3 : EXT_3
 * D4 : EXT_1
 * D5 : EXT_6
 * D6 : EXT_7
 * D7 : EXT_8
 * D8 : EXT_9
 * D9 : EXT_10
 * D10 : EXT_12
 * D11 : EXT_13
 * D12 : EXT_14
 * D13 : EXT_11
 * D14 : EXT_15
 * D15 : EXT_5
 * D16 : EXT_16
 * D17 : EXT_17
 * D18 : EXT_18
 * D19 : EXT_19
 * D20 : EXT_20
 * D21 : EXT_21
 * D22 : EXT_22
 * D23 : EXT_23
 * D24 : EXT_24
 * D25 : EXT_25
 * D26 : EXT_26
 * D27 : EXT_30
 * D28 : EXT_28
 * D29 : EXT_29
 * D30 : EXT_27
 * D31 : EXT_32
 * D32 : EXT_33
 * D33 : EXT_34
 * D34 : EXT_35
 * D35 : EXT_36
 * D36 : EXT_38
 * D37 : EXT_39
 * D38 : EXT_40
 * D39 : EXT_44
 * D40 : EXT_41
 * D41 : EXT_31
 * D42 : EXT_37
 * D43 : EXT_42
 * D44 : EXT_43
 * D45 : EXT_45
 * D46 : EXT_46
 * D47 : EXT_47
 * D48 : EXT_48
 * D49 : EXT_49
 * D50 : EXT_50
 * D51 : EXT_51
 *
 * UART_3_RX : D0
 * UART_3_TX : D1
 * SPI_3_CS : D10
 * SPI_3_MOSI : D11
 * SPI_3_MISO : D12
 * SPI_3_SCLK : D13
 * I2C_3_SDA : D14
 * I2C_3_SCL : D15
 * UART_4_RX : D26
 * UART_4_TX : D30
 * SPI_4_CS : D36
 * SPI_4_MOSI : D37
 * SPI_4_MISO : D38
 * SPI_4_SCK : D39
 * I2C_4_SDA : D40
 * I2C_4_SCL : D41
 *
 */
static void arm_mps2_pinmux_defaults(void)
{
	uint32_t gpio_0 = 0U;
	uint32_t gpio_1 = 0U;
	uint32_t gpio_2 = 0U;

	/* Set GPIO Alternate Functions */

	gpio_0 = (1<<0)   /* Shield 0 UART 3 RXD */
	       | (1<<4)   /* Shield 0 UART 3 TXD */
	       | (1<<5)   /* Shield 0 I2C SCL SBCON2 */
	       | (1<<15)  /* Shield 0 I2C SDA SBCON2 */
	       | (1<<11)  /* Shield 0 SPI 3 SCK */
	       | (1<<12)  /* Shield 0 SPI 3 SS */
	       | (1<<13)  /* Shield 0 SPI 3 MOSI */
	       | (1<<14); /* Shield 0 SPI 3 MISO */

	CMSDK_AHB_GPIO0_DEV->altfuncset = gpio_0;

	gpio_1 = (1<<10) /* Shield 1 UART 4 RXD */
	       | (1<<14) /* Shield 1 UART 4 TXD */
	       | (1<<15) /* Shield 1 I2C SCL SBCON3 */
	       | (1<<0)  /* ADC SPI 2 SS */
	       | (1<<1)  /* ADC SPI 2 MISO */
	       | (1<<2)  /* ADC SPI 2 MOSI */
	       | (1<<3)  /* ADC SPI 2 SCK */
	       | (1<<5)  /* USER BUTTON 0 */
	       | (1<<6); /* USER BUTTON 1 */

	CMSDK_AHB_GPIO1_DEV->altfuncset = gpio_1;

	gpio_2 = (1<<9)   /* Shield 1 I2C SDA SBCON3 */
	       | (1<<6)   /* Shield 1 SPI 4 SS */
	       | (1<<7)   /* Shield 1 SPI 4 MOSI */
	       | (1<<8)   /* Shield 1 SPI 4 MISO */
	       | (1<<12); /* Shield 1 SPI 4 SCK */

	CMSDK_AHB_GPIO2_DEV->altfuncset = gpio_2;
}

static int arm_mps2_pinmux_init(const struct device *port)
{
	ARG_UNUSED(port);

	arm_mps2_pinmux_defaults();

	return 0;
}

SYS_INIT(arm_mps2_pinmux_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
