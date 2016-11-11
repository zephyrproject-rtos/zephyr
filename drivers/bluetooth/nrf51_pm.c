/* nrf51-pm.c Power Management for nrf51 chip */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <gpio.h>
#include <uart.h>

#include <bluetooth/log.h>
#include <errno.h>

#define NBLE_SWDIO_PIN	6
#define NBLE_RESET_PIN	NBLE_SWDIO_PIN
#define NBLE_BTWAKE_PIN 5

static struct device *nrf51_gpio;

int nrf51_wakeup(void)
{
	return gpio_pin_write(nrf51_gpio, NBLE_BTWAKE_PIN, 1);
}

int nrf51_allow_sleep(void)
{
	return gpio_pin_write(nrf51_gpio, NBLE_BTWAKE_PIN, 0);
}

int nrf51_init(struct device *dev)
{
	uint8_t c;
	int ret;

	nrf51_gpio = device_get_binding("GPIO_0");
	if (!nrf51_gpio) {
		BT_ERR("Cannot find GPIO_0");
		return -ENODEV;
	}

	ret = gpio_pin_configure(nrf51_gpio, NBLE_RESET_PIN, GPIO_DIR_OUT);
	if (ret) {
		BT_ERR("Error configuring pin %d", NBLE_RESET_PIN);
		return -ENODEV;
	}

	/* Reset hold time is 0.2us (normal) or 100us (SWD debug) */
	ret = gpio_pin_write(nrf51_gpio, NBLE_RESET_PIN, 0);
	if (ret) {
		BT_ERR("Error pin write %d", NBLE_RESET_PIN);
		return -EINVAL;
	}

	/* Drain the fifo */
	while (uart_fifo_read(dev, &c, 1)) {
		continue;
	}

	/**
	 * NBLE reset is achieved by asserting low the SWDIO pin.
	 * However, the BLE Core chip can be in SWD debug mode,
	 * and NRF_POWER->RESET = 0 due to, other constraints: therefore,
	 * this reset might not work everytime, especially after
	 * flashing or debugging.
	 */

	/* sleep 1ms depending on context */
	k_sleep(1);

	ret = gpio_pin_write(nrf51_gpio, NBLE_RESET_PIN, 1);
	if (ret) {
		BT_ERR("Error pin write %d", NBLE_RESET_PIN);
		return -EINVAL;
	}

	/* Set back GPIO to input to avoid interfering with external debugger */
	ret = gpio_pin_configure(nrf51_gpio, NBLE_RESET_PIN, GPIO_DIR_IN);
	if (ret) {
		BT_ERR("Error configuring pin %d", NBLE_RESET_PIN);
		return -ENODEV;
	}

	ret = gpio_pin_configure(nrf51_gpio, NBLE_BTWAKE_PIN, GPIO_DIR_OUT);
	if (ret) {
		BT_ERR("Error configuring pin %d", NBLE_BTWAKE_PIN);
		return -ENODEV;
	}

	return nrf51_wakeup();
}
