/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP451_TMP451_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP451_TMP451_H_

#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

/* Device Identification */
#define TMP451_ID 0x55

/* Conversion Time Max */
#define TMP451_CONVERSION_TIME_MS 34

/* Temperature Resolution: 1 count = 0.0625 deg C (1/16th degree) */
#define TMP451_TEMP_RESOLUTION 16                                 /* counts per deg C */
#define TMP451_TEMP_SCALE      (1000000 / TMP451_TEMP_RESOLUTION) /* micro-deg C per count */

/* Extended Range Offset */
#define TMP451_EXTENDED_RANGE_DEGREES 64
#define TMP451_EXTENDED_RANGE_OFFSET  (TMP451_EXTENDED_RANGE_DEGREES * TMP451_TEMP_RESOLUTION)

/* Config Reg Bits */
#define TMP451_CONFIG_MASK1  BIT(7)
#define TMP451_CONFIG_SD     BIT(6)
#define TMP451_CONFIG_THERM2 BIT(5)
#define TMP451_CONFIG_RANGE  BIT(2)

/* Status Reg Bits */
#define TMP451_STATUS_LHIGH BIT(6)
#define TMP451_STATUS_LLOW  BIT(5)
#define TMP451_STATUS_RHIGH BIT(4)
#define TMP451_STATUS_RLOW  BIT(3)
#define TMP451_STATUS_OPEN  BIT(2)

/* Status Alert Groups */
#define TMP451_STATUS_LOCAL_ALERT  (TMP451_STATUS_LHIGH | TMP451_STATUS_LLOW)
#define TMP451_STATUS_REMOTE_ALERT (TMP451_STATUS_RHIGH | TMP451_STATUS_RLOW)

/* CONAL Reg Bits */
#define TMP451_CONAL_MASK (BIT(3) | BIT(2) | BIT(1))
#define TMP451_CONAL_1    0
#define TMP451_CONAL_2    BIT(1)
#define TMP451_CONAL_3    (BIT(2) | BIT(1))
#define TMP451_CONAL_4    (BIT(3) | BIT(2) | BIT(1))

/* Register Addresses */
#define TMP451_REG_LT_H           0x00
#define TMP451_REG_RT_H           0x01
#define TMP451_REG_STATUS         0x02
#define TMP451_REG_CONFIG_READ    0x03
#define TMP451_REG_CR_READ        0x04
#define TMP451_REG_LTHL_READ      0x05
#define TMP451_REG_LTLL_READ      0x06
#define TMP451_REG_RTHL_H_READ    0x07
#define TMP451_REG_RTLL_H_READ    0x08
#define TMP451_REG_CONFIG_WRITE   0x09
#define TMP451_REG_CR_WRITE       0x0A
#define TMP451_REG_LTHL_WRITE     0x0B
#define TMP451_REG_LTLL_WRITE     0x0C
#define TMP451_REG_RTHL_H_WRITE   0x0D
#define TMP451_REG_RTLL_H_WRITE   0x0E
#define TMP451_REG_ONE_SHOT_START 0x0F
#define TMP451_REG_RT_L           0x10
#define TMP451_REG_RTO_H          0x11
#define TMP451_REG_RTO_L          0x12
#define TMP451_REG_RTHL_L         0x13
#define TMP451_REG_RTLL_L         0x14
#define TMP451_REG_LT_L           0x15
#define TMP451_REG_RTH            0x19
#define TMP451_REG_LTH            0x20
#define TMP451_REG_HYS            0x21
#define TMP451_REG_CONAL          0x22
#define TMP451_REG_NC             0x23
#define TMP451_REG_DF             0x24
#define TMP451_REG_ID             0xFE

struct tmp451_config {
	const struct i2c_dt_spec i2c;
	int8_t ideality_factor;
	uint8_t hysteresis;
	int32_t remote_offset;
	int32_t conversion_rate;
	uint8_t digital_filter;
	int16_t local_therm_limit;
	int16_t remote_therm_limit;
	bool therm2_mode;
#ifdef CONFIG_TMP451_TRIGGER
	const struct gpio_dt_spec alert_gpio;
	uint8_t consecutive_alerts;
#endif /* CONFIG_TMP451_TRIGGER */
};

struct tmp451_data {
	uint16_t local_raw;
	uint16_t remote_raw;
	bool one_shot_mode;
#ifdef CONFIG_TMP451_TRIGGER
	const struct device *dev;
	struct gpio_callback trigger_cb;
	sensor_trigger_handler_t trigger_handler_local;
	sensor_trigger_handler_t trigger_handler_remote;
	const struct sensor_trigger *trigger_local;
	const struct sensor_trigger *trigger_remote;
#if defined(CONFIG_TMP451_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TMP451_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_TMP451_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_TMP451_TRIGGER */
};

#ifdef CONFIG_TMP451_TRIGGER
int tmp451_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int tmp451_init_interrupt(const struct device *dev);
#endif

int tmp451_reg_update(const struct tmp451_config *cfg, uint8_t read_reg, uint8_t write_reg,
		      uint8_t mask, uint8_t val);

#endif /* ZEPHYR_DRIVERS_SENSOR_TMP451_TMP451_H_ */
