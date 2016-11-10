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

#ifndef __SENSOR_SX9500_H__
#define __SENSOR_SX9500_H__

#include <stdint.h>
#include <device.h>

#define SX9500_REG_IRQ_SRC		0x00
#define SX9500_REG_STAT			0x01
#define SX9500_REG_IRQ_MSK		0x03

#define SX9500_REG_PROX_CTRL0		0x06
#define SX9500_REG_PROX_CTRL1		0x07

/* These are used both in the IRQ_SRC register, to identify which
 * interrupt occur, and in the IRQ_MSK register, to enable specific
 * interrupts.
 */
#define SX9500_CONV_DONE_IRQ		(1 << 3)
#define SX9500_NEAR_FAR_IRQ		((1 << 5) | (1 << 6))

struct sx9500_data {
	struct device *i2c_master;
	uint16_t i2c_slave_addr;
	uint8_t prox_stat;

	struct gpio_callback gpio_cb;

#ifdef CONFIG_SX9500_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_SX9500_TRIGGER_GLOBAL_THREAD
	struct k_work work;
	struct device *dev;
#endif

#ifdef CONFIG_SX9500_TRIGGER
	struct sensor_trigger trigger_drdy;
	struct sensor_trigger trigger_near_far;

	sensor_trigger_handler_t handler_drdy;
	sensor_trigger_handler_t handler_near_far;
#endif
};

#ifdef CONFIG_SX9500_TRIGGER
int sx9500_setup_interrupt(struct device *dev);
int sx9500_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
#else
static inline int sx9500_setup_interrupt(struct device *dev)
{
	return 0;
}
#endif

#define SYS_LOG_DOMAIN "SX9500"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <misc/sys_log.h>
#endif /* __SENSOR_SX9500_H__ */
