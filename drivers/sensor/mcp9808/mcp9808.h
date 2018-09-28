/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MCP9808_MCP9808_H_
#define ZEPHYR_DRIVERS_SENSOR_MCP9808_MCP9808_H_

#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <sensor.h>
#include <misc/util.h>
#include <gpio.h>

#define MCP9808_REG_CONFIG		0x01
#define MCP9808_REG_UPPER_LIMIT		0x02
#define MCP9808_REG_LOWER_LIMIT		0x03
#define MCP9808_REG_CRITICAL		0x04
#define MCP9808_REG_TEMP_AMB		0x05

#define MCP9808_ALERT_INT		BIT(0)
#define MCP9808_ALERT_CNT		BIT(3)
#define MCP9808_INT_CLEAR		BIT(5)

#define MCP9808_SIGN_BIT		BIT(12)
#define MCP9808_TEMP_INT_MASK		0x0ff0
#define MCP9808_TEMP_INT_SHIFT		4
#define MCP9808_TEMP_FRAC_MASK		0x000f

#define MCP9808_TEMP_MAX		0xffc

struct mcp9808_data {
	struct device *i2c_master;
	u16_t i2c_slave_addr;

	u16_t reg_val;

	struct gpio_callback gpio_cb;

#ifdef CONFIG_MCP9808_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD
	struct k_work work;
	struct device *dev;
#endif

#ifdef CONFIG_MCP9808_TRIGGER
	struct sensor_trigger trig;
	sensor_trigger_handler_t trigger_handler;
#endif
};

int mcp9808_reg_read(struct mcp9808_data *data, u8_t reg, u16_t *val);

#ifdef CONFIG_MCP9808_TRIGGER
int mcp9808_attr_set(struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);
int mcp9808_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
void mcp9808_setup_interrupt(struct device *dev);
#else
static inline int mcp9808_attr_set(struct device *dev,
				   enum sensor_channel chan,
				   enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	return -ENOTSUP;
}

static inline int mcp9808_trigger_set(struct device *dev,
				      const struct sensor_trigger *trig,
				      sensor_trigger_handler_t handler)
{
	return -ENOTSUP;
}

static void mcp9808_setup_interrupt(struct device *dev)
{
}
#endif /* CONFIG_MCP9808_TRIGGER */

#define SYS_LOG_DOMAIN "MCP9808"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* ZEPHYR_DRIVERS_SENSOR_MCP9808_MCP9808_H_ */
