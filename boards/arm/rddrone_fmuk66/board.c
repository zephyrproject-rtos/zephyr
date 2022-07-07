/*
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr/device.h>
#include <drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rddrone_fmuk66_board_init);

/* Initialization chain of rddrone fmuk66 board requires some delays before on board sensors
 * could be accessed after power up. In particular bme680 and bmm150 sensors require,
 * respectively 2ms and 1ms power on delay. In order to avoid delays sum, common delay is
 * introduced in the board start up file. Below asserts ensure correct initialization order:
 * on board regulators, board init (this), sensors init.
 */
#if defined(CONFIG_REGULATOR_FIXED)
BUILD_ASSERT(CONFIG_RDDRONE_FMUK66_INIT_PRIORITY > CONFIG_REGULATOR_FIXED_INIT_PRIORITY,
	"CONFIG_RDDRONE_FMUK66_INIT_PRIORITY must be higher than CONFIG_REGULATOR_FIXED_INIT_PRIORITY");
#endif

#if defined(CONFIG_SENSOR)
BUILD_ASSERT(CONFIG_RDDRONE_FMUK66_INIT_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY,
	"CONFIG_RDDRONE_FMUK66_INIT_PRIORITY must be less than CONFIG_SENSOR_INIT_PRIORITY");
#endif



static int setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *gpiob = DEVICE_DT_GET(DT_NODELABEL(gpiob));

        /* PTB8 or gpiob 8th pin is responsible for enabling 3.3V for sensors
         * on rddrone fmuk66 */
        gpio_pin_configure(gpiob, 8, GPIO_OUTPUT_HIGH);

	if (IS_ENABLED(CONFIG_SENSOR)) {
		/* Initialization chain of rddrone fmuk66 board requires some delays before on board
		 * sensors could be accessed after power up. In particular bme680 and bmm150
		 * sensors require, 2ms and 1ms power on delay respectively. In order not to sum
		 * delays, common delay is introduced in the board start up file. This code is
		 * executed after sensors are powered up and before their initialization.
		 * It's ensured by build asserts at the beginning of this file.
		 */
		k_msleep(2);
	}

	return 0;
}

SYS_INIT(setup, POST_KERNEL, CONFIG_RDDRONE_FMUK66_INIT_PRIORITY);
