/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_OMRON_D7S_D7S_H_
#define ZEPHYR_DRIVERS_SENSOR_OMRON_D7S_D7S_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/d7s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define D7S_REG_STATE         0x1000U
#define D7S_REG_AXIS_STATE    0x1001U
#define D7S_REG_EVENT         0x1002U
#define D7S_REG_MODE          0x1003U
#define D7S_REG_CTRL          0x1004U
#define D7S_REG_CLEAR_COMMAND 0x1005U

/* Earthquake records, newest first, D7S_RECORD_SIZE bytes each. */
#define D7S_REG_LATEST(n) (0x3000U + ((n) << 8))

#define D7S_RECORD_T_AVE 6U
#define D7S_RECORD_SI    8U
#define D7S_RECORD_PGA   10U
#define D7S_RECORD_SIZE  12U

#define D7S_STATE_MASK     GENMASK(2, 0)
#define D7S_EVENT_MASK     GENMASK(3, 0)
#define D7S_CLEAR_MASK     GENMASK(3, 0)
#define D7S_MODE_MASK      GENMASK(2, 0)
#define D7S_CTRL_AXIS_MASK GENMASK(6, 4)
#define D7S_CTRL_THRESHOLD BIT(3)

#define D7S_MODE_NORMAL          0x01U
#define D7S_MODE_INITIAL_INSTALL 0x02U
#define D7S_MODE_SELF_DIAG       0x04U

struct d7s_config {
	struct i2c_dt_spec i2c;
	uint8_t axis_mode;
	uint8_t shutoff_threshold;
#ifdef CONFIG_D7S_TRIGGER
	struct gpio_dt_spec int1;
	struct gpio_dt_spec int2;
#endif
};

struct d7s_data {
	uint8_t record[D7S_RECORD_SIZE];

#ifdef CONFIG_D7S_TRIGGER
	const struct device *dev;

	struct gpio_callback int1_cb;
	struct gpio_callback int2_cb;

	const struct sensor_trigger *threshold_trigger;
	sensor_trigger_handler_t threshold_handler;
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t drdy_handler;

	atomic_t pending;

#if defined(CONFIG_D7S_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_D7S_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#elif defined(CONFIG_D7S_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_D7S_TRIGGER */
};

int d7s_read_reg(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len);
int d7s_write_reg(const struct device *dev, uint16_t reg, uint8_t val);

#ifdef CONFIG_D7S_TRIGGER
#define D7S_PENDING_INT1 BIT(0)
#define D7S_PENDING_INT2 BIT(1)

int d7s_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		    sensor_trigger_handler_t handler);
int d7s_trigger_init(const struct device *dev);
#endif

#ifdef CONFIG_EMUL
struct emul;

void d7s_emul_set_record(const struct emul *target, uint8_t index, uint16_t si, uint16_t pga,
			 int16_t temp);
void d7s_emul_set_event(const struct emul *target, uint8_t flags);
void d7s_emul_set_state(const struct emul *target, uint8_t state);
uint8_t d7s_emul_get_ctrl(const struct emul *target);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_OMRON_D7S_D7S_H_ */
