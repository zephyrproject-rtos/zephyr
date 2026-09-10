/*
 * Copyright (c) 2024 Gustavo Silva
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ENS160_ENS160_H_
#define ZEPHYR_DRIVERS_SENSOR_ENS160_ENS160_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

/* Registers */
#define ENS160_REG_PART_ID       0x00
#define ENS160_REG_OPMODE        0x10
#define ENS160_REG_CONFIG        0x11
#define ENS160_REG_COMMAND       0x12
#define ENS160_REG_TEMP_IN       0x13
#define ENS160_REG_RH_IN         0x15
#define ENS160_REG_DEVICE_STATUS 0x20
#define ENS160_REG_DATA_AQI      0x21
#define ENS160_REG_DATA_TVOC     0x22
#define ENS160_REG_DATA_ECO2     0x24
#define ENS160_REG_DATA_T        0x30
#define ENS160_REG_DATA_RH       0x32
#define ENS160_REG_DATA_MISR     0x38
#define ENS160_REG_GPR_WRITE0    0x40
#define ENS160_REG_GPR_WRITE1    0x41
#define ENS160_REG_GPR_WRITE2    0x42
#define ENS160_REG_GPR_WRITE3    0x43
#define ENS160_REG_GPR_WRITE4    0x44
#define ENS160_REG_GPR_WRITE5    0x45
#define ENS160_REG_GPR_WRITE6    0x46
#define ENS160_REG_GPR_WRITE7    0x47
#define ENS160_REG_GPR_READ0     0x48
#define ENS160_REG_GPR_READ1     0x49
#define ENS160_REG_GPR_READ2     0x4A
#define ENS160_REG_GPR_READ3     0x4B
#define ENS160_REG_GPR_READ4     0x4C
#define ENS160_REG_GPR_READ5     0x4D
#define ENS160_REG_GPR_READ6     0x4E
#define ENS160_REG_GPR_READ7     0x4F

#define ENS160_PART_ID 0x160

#define ENS160_TIMEOUT_US      (1000000U)
#define ENS160_BOOTING_TIME_MS (10U)

/* Operation modes */
#define ENS160_OPMODE_DEEP_SLEEP 0x00
#define ENS160_OPMODE_IDLE       0x01
#define ENS160_OPMODE_STANDARD   0x02
#define ENS160_OPMODE_RESET      0xF0

/* Device status */
#define ENS160_STATUS_STATER        BIT(6)
#define ENS160_STATUS_VALIDITY_FLAG GENMASK(3, 2)
#define ENS160_STATUS_NEWDAT        BIT(1)

#define ENS160_STATUS_NORMAL   0x00
#define ENS160_STATUS_WARM_UP  0x01
#define ENS160_STATUS_START_UP 0x02
#define ENS160_STATUS_INVALID  0x03

/* Commands */
#define ENS160_COMMAND_NOP        0x00
#define ENS160_COMMAND_GET_APPVER 0x0E
#define ENS160_COMMAND_CLRGPR     0xCC

/* Config register fields */
#define ENS160_CONFIG_INTPOL  BIT(6)
#define ENS160_CONFIG_INT_CFG BIT(5)
#define ENS160_CONFIG_INTGPR  BIT(3)
#define ENS160_CONFIG_INTDAT  BIT(1)
#define ENS160_CONFIG_INTEN   BIT(0)

#define ENS160_DATA_AQI_UBA GENMASK(2, 0)

struct ens160_config {
	int (*bus_init)(const struct device *dev);
	const union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		struct spi_dt_spec spi;
#endif
	};
#ifdef CONFIG_ENS160_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

struct ens160_transfer_function {
	int (*read_reg)(const struct device *dev, uint8_t reg, uint8_t *val);
	int (*read_data)(const struct device *dev, uint8_t reg, uint8_t *data, size_t len);
	int (*write_reg)(const struct device *dev, uint8_t reg, uint8_t val);
	int (*write_data)(const struct device *dev, uint8_t reg, uint8_t *data, size_t len);
};

struct ens160_data {
	uint16_t eco2;
	uint16_t tvoc;
	uint8_t aqi;
	const struct ens160_transfer_function *tf;
#ifdef CONFIG_ENS160_TRIGGER
	struct gpio_callback gpio_cb;
	const struct device *dev;

	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
#if defined(CONFIG_ENS160_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ENS160_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ENS160_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ENS160_TRIGGER */
};

#ifdef CONFIG_ENS160_TRIGGER
int ens160_trigger_set(const struct device *dev, const struct sensor_trigger *trigg,
		       sensor_trigger_handler_t handler);

int ens160_init_interrupt(const struct device *dev);
#endif

int ens160_i2c_init(const struct device *dev);
int ens160_spi_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ENS160_ENS160_H_ */
