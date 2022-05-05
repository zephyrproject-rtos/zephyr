/*
 * Copyright (c) 2020, Texas Instruments Incorporated
 * Copyright (c) 2020 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <driverlib/cpu.h>

#define GPIO_PORT DT_LABEL(DT_NODELABEL(gpio0))
#define DIO8_PIN  8
#define DIO9_PIN  9
#define DIO10_PIN 10
#define DIO20_PIN 20


/*
 *  ======== CC1352R1_LAUNCHXL_sendExtFlashByte ========
 */
void CC1352R1_LAUNCHXL_sendExtFlashByte(const struct device *dev,
					uint8_t byte)
{
	uint8_t i;

	/* SPI Flash CS */
	gpio_pin_set(dev, DIO20_PIN, 0);

	for (i = 0; i < 8; i++) {
		gpio_pin_set(dev, DIO10_PIN, 0); /* SPI Flash CLK */

		/* SPI Flash MOSI */
		gpio_pin_set(dev, DIO9_PIN, (byte >> (7 - i)) & 0x01);
		gpio_pin_set(dev, DIO10_PIN, 1); /* SPI Flash CLK */

		/*
		 * Waste a few cycles to keep the CLK high for at
		 * least 45% of the period.
		 * 3 cycles per loop: 8 loops @ 48 Mhz = 0.5 us.
		 */
		CPUdelay(8);
	}

	gpio_pin_set(dev, DIO10_PIN, 0);   /* CLK */
	gpio_pin_set(dev, DIO20_PIN, 1);   /* CS */

	/*
	 * Keep CS high at least 40 us
	 * 3 cycles per loop: 700 loops @ 48 Mhz ~= 44 us
	 */
	CPUdelay(700);
}

/*
 *  ======== CC1352R1_LAUNCHXL_wakeUpExtFlash ========
 */
void CC1352R1_LAUNCHXL_wakeUpExtFlash(const struct device *dev)
{
	/*
	 *  To wake up we need to toggle the chip select at
	 *  least 20 ns and ten wait at least 35 us.
	 */

	/* Toggle chip select for ~20ns to wake ext. flash */
	gpio_pin_set(dev, DIO20_PIN, 0);
	/* 3 cycles per loop: 1 loop @ 48 Mhz ~= 62 ns */
	CPUdelay(1);
	gpio_pin_set(dev, DIO20_PIN, 1);
	/* 3 cycles per loop: 560 loops @ 48 Mhz ~= 35 us */
	CPUdelay(560);
}

/*
 *  ======== CC1352R1_LAUNCHXL_shutDownExtFlash ========
 */
void CC1352R1_LAUNCHXL_shutDownExtFlash(void)
{
	const struct device *dev;
	uint8_t extFlashShutdown = 0xB9;

	dev = device_get_binding(GPIO_PORT);
	/* Set SPI Flash CS pin as output */
	gpio_pin_configure(dev, DIO20_PIN, GPIO_OUTPUT);
	/* Set SPI Flash CLK pin as output */
	gpio_pin_configure(dev, DIO10_PIN, GPIO_OUTPUT);
	/* Set SPI Flash MOSI pin as output */
	gpio_pin_configure(dev, DIO9_PIN, GPIO_OUTPUT);
	/* Set SPI Flash MISO pin as input */
	gpio_pin_configure(dev, DIO8_PIN, GPIO_INPUT | GPIO_PULL_DOWN);

	/*
	 *  To be sure we are putting the flash into sleep and not waking it,
	 *  we first have to make a wake up call
	 */
	CC1352R1_LAUNCHXL_wakeUpExtFlash(dev);

	CC1352R1_LAUNCHXL_sendExtFlashByte(dev, extFlashShutdown);
}
