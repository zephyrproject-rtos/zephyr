/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1X_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1X_H_

#include <device.h>
#include <misc/util.h>
#include <zephyr/types.h>
#include <gpio.h>

#include "vl53l1_api.h"

/* All the values used in this driver are coming from ST datasheet and examples.
 * It can be found here:
 *   http://www.st.com/en/embedded-software/stsw-img007.html
 */
#define VL53L1X_REG_WHO_AM_I   0x010F
#define VL53L1X_CHIP_ID        0xEACC
#define VL53L1X_SETUP_SIGNAL_LIMIT         (0.1*65536)
#define VL53L1X_SETUP_SIGMA_LIMIT          (60*65536)
#define VL53L1X_SETUP_MAX_TIME_FOR_RANGING     33000
#define VL53L1X_SETUP_PRE_RANGE_VCSEL_PERIOD   18
#define VL53L1X_SETUP_FINAL_RANGE_VCSEL_PERIOD 14

struct vl53l1x_data {
	struct device *i2c;
	VL53L1_Dev_t vl53l1x;
	VL53L1_RangingMeasurementData_t RangingMeasurementData;
#ifdef CONFIG_VL53L1X_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_VL53L1X_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_VL53L1X_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_VL53L1X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_VL53L1X_TRIGGER */
};

#ifdef CONFIG_VL53L1X_TRIGGER
int vl53l1x_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int vl53l1x_init_interrupt(struct device *dev);
#endif


#endif /* __SENSOR_VL53L1X__ */
