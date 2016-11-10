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

#ifndef __SENSOR_HTS221_H__
#define __SENSOR_HTS221_H__

#include <device.h>
#include <misc/util.h>
#include <stdint.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "HTS221"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>

#define HTS221_I2C_ADDR			0x5F
#define HTS221_AUTOINCREMENT_ADDR	BIT(7)

#define HTS221_REG_WHO_AM_I		0x0F
#define HTS221_CHIP_ID			0xBC

#define HTS221_REG_CTRL1		0x20
#define HTS221_PD_BIT			BIT(7)
#define HTS221_BDU_BIT			BIT(3)
#define HTS221_ODR_SHIFT		0

#define HTS221_REG_CTRL3		0x22
#define HTS221_DRDY_EN			BIT(2)

#define HTS221_REG_DATA_START		0x28
#define HTS221_REG_CONVERSION_START	0x30

static const char * const hts221_odr_strings[] = {
	"1", "7", "12.5"
};

struct hts221_data {
	struct device *i2c;
	int16_t rh_sample;
	int16_t t_sample;

	uint8_t h0_rh_x2;
	uint8_t h1_rh_x2;
	uint16_t t0_degc_x8;
	uint16_t t1_degc_x8;
	int16_t h0_t0_out;
	int16_t h1_t0_out;
	int16_t t0_out;
	int16_t t1_out;

#ifdef CONFIG_HTS221_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	char __stack thread_stack[CONFIG_HTS221_THREAD_STACK_SIZE];
	struct k_sem gpio_sem;
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_HTS221_TRIGGER */
};

#ifdef CONFIG_HTS221_TRIGGER
int hts221_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int hts221_init_interrupt(struct device *dev);
#endif

#endif /* __SENSOR_HTS221__ */
