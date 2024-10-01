/*
 * Copyright (c) 2024 Vitrolife A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MFD_AD559X_H_
#define ZEPHYR_DRIVERS_MFD_AD559X_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DT_DRV_COMPAT adi_ad559x

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#define AD559X_GPIO_READBACK_EN         BIT(10)
#define AD559X_LDAC_READBACK_EN         BIT(6)
#define AD559X_REG_SOFTWARE_RESET       0x0FU
#define AD559X_SOFTWARE_RESET_MAGIC_VAL 0x5AC
#define AD559X_REG_VAL_MASK             0x3FF
#define AD559X_REG_RESET_VAL_MASK       0x7FF
#define AD559X_REG_SHIFT_VAL            11
#define AD559X_REG_READBACK_SHIFT_VAL   2

struct mfd_ad559x_transfer_function {
	int (*read_raw)(const struct device *dev, uint8_t *val, size_t len);
	int (*write_raw)(const struct device *dev, uint8_t *val, size_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg, uint8_t reg_data, uint16_t *val);
	int (*write_reg)(const struct device *dev, uint8_t reg, uint16_t val);
};

struct mfd_ad559x_config {
	struct gpio_dt_spec reset_gpio;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
	int (*bus_init)(const struct device *dev);
	bool has_pointer_byte_map;
};

struct mfd_ad559x_data {
	const struct mfd_ad559x_transfer_function *transfer_function;
};

int mfd_ad559x_i2c_init(const struct device *dev);
int mfd_ad559x_spi_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MFD_AD559X_H_*/
