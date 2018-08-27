/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_gpio_basic_api
 * @{
 * @defgroup t_gpio_basic_read_write test_gpio_basic_read_write
 * @brief TestPurpose: verify zephyr gpio read and write correctly
 * @}
 */

#include <inttypes.h>

#include "test_gpio.h"

void test_gpio_pin_read_write(void)
{
	struct device *dev = device_get_binding(DEV_NAME);

	/* set PIN_OUT as writer */
#if CONFIG_PINCTRL
	TC_PRINT("device=%s, pin1=%d, pin2=%d\n", DEV_NAME,
		 GPIO_PORT_PIN_IDX(PIN_OUT), GPIO_PORT_PIN_IDX(PIN_IN));
#else
	TC_PRINT("device=%s, pin1=%d, pin2=%d\n", DEV_NAME, PIN_OUT, PIN_IN);
#endif
	zassert_true(gpio_pin_configure(dev, PIN_OUT, GPIO_DIR_OUT) == 0,
		     "configure pin out fail");
	/* set PIN_IN as reader */
	zassert_true(gpio_pin_configure(dev, PIN_IN, GPIO_DIR_IN) == 0,
		     "configure pin in fail");
	zassert_true(gpio_pin_disable_callback(dev, PIN_IN) == 0,
		     "disable pin in callback fail");

	u32_t val_write, val_read = 0;
	int i = 0;

	while (i++ < 32) {
		val_write = sys_rand32_get() / 3 % 2;
		zassert_true(gpio_pin_write(dev, PIN_OUT, val_write) == 0,
			    "write data fail");
		TC_PRINT("write: %" PRIu32 "\n", val_write);
		k_sleep(100);
		zassert_true(gpio_pin_read(dev, PIN_IN, &val_read) == 0,
			    "read data fail");
		TC_PRINT("read: %" PRIu32 "\n", val_read);

		/*= checkpoint: compare write and read value =*/
		zassert_true(val_write == val_read,
			    "Inconsistent GPIO read/write value");
	}
}
