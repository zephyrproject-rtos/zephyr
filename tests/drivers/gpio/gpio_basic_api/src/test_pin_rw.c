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

/**
 * @addtogroup t_gpio_basic_api
 * @{
 * @defgroup t_gpio_basic_read_write test_gpio_basic_read_write
 * @brief TestPurpose: verify zephyr gpio read and write correctly
 * @}
 */

#include <stdlib.h>
#include "test_gpio.h"

void test_gpio_pin_read_write(void)
{
	struct device *dev = device_get_binding(DEV_NAME);

	/* set PIN_OUT as writer */
	TC_PRINT("device=%s, pin1=%d, pin2=%d\n", DEV_NAME, PIN_OUT, PIN_IN);
	gpio_pin_configure(dev, PIN_OUT, GPIO_DIR_OUT);
	/* set PIN_IN as reader */
	gpio_pin_configure(dev, PIN_IN, GPIO_DIR_IN);
	gpio_pin_disable_callback(dev, PIN_IN);

	uint32_t val_write, val_read = 0;
	int i = 0;

	while (i++ < 32) {
		val_write = rand() % 2;
		assert_true(gpio_pin_write(dev, PIN_OUT, val_write) == 0,
			    "write data fail");
		TC_PRINT("write: %lu\n", val_write);
		k_sleep(100);
		assert_true(gpio_pin_read(dev, PIN_IN, &val_read) == 0,
			    "read data fail");
		TC_PRINT("read: %lu\n", val_read);

		/*= checkpoint: compare write and read value =*/
		assert_true(val_write == val_read,
			    "Inconsistent GPIO read/write value");
	}
}
