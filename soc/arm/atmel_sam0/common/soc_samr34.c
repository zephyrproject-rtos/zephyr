/*
 * Copyright (c) 2021 Argentum Systems Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#if !(ATMEL_SAM0_DT_SERCOM_CHECK(4, atmel_sam0_spi) && CONFIG_SPI_SAM0)
/* When the radio is not in use, it's important that #CS is set high, to avoid
 * unexpected behavior and increased current consumption... see Chapter 10 of
 * DS70005356C. We also hold the radio in reset.
 */
static int soc_pinconf_init(const struct device *dev)
{
	const struct device *const portb = DEVICE_DT_GET(DT_NODELABEL(portb));

	ARG_UNUSED(dev);

	if (!device_is_ready(portb)) {
		return -ENODEV;
	}

	gpio_pin_configure(portb, 31, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(portb, 15, GPIO_OUTPUT_LOW);

	return 0;
}

SYS_INIT(soc_pinconf_init, PRE_KERNEL_2, 0);
#endif
