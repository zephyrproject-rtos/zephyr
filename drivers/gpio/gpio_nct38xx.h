/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_NCT38XX_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_NCT38XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

/* NCT38XX controller register */
#define NCT38XX_REG_ALERT      0x10
#define NCT38XX_REG_ALERT_MASK 0x12

#define NCT38XX_REG_GPIO_DATA_IN(n)     (0xC0 + ((n) * 8))
#define NCT38XX_REG_GPIO_DATA_OUT(n)    (0xC1 + ((n) * 8))
#define NCT38XX_REG_GPIO_DIR(n)         (0xC2 + ((n) * 8))
#define NCT38XX_REG_GPIO_OD_SEL(n)      (0xC3 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_RISE(n)  (0xC4 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_FALL(n)  (0xC5 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_LEVEL(n) (0xC6 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_MASK(n)  (0xC7 + ((n) * 8))
#define NCT38XX_REG_MUX_CONTROL         0xD0
#define NCT38XX_REG_GPIO_ALERT_STAT(n)  (0xD4 + (n))

/* NCT38XX controller register field */
#define NCT38XX_REG_ALERT_VENDOR_DEFINDED_ALERT      15
#define NCT38XX_REG_ALERT_MASK_VENDOR_DEFINDED_ALERT 15

/* Driver config */
struct gpio_nct38xx_config {
	/* I2C device */
	const struct i2c_dt_spec i2c_dev;
	/* GPIO ports */
	const struct device **sub_gpio_dev;
	uint8_t sub_gpio_port_num;
	/* Alert handler */
	const struct device *alert_dev;
};

/* Driver data */
struct gpio_nct38xx_data {
	/* NCT38XX device */
	const struct device *dev;
};

/**
 * @brief Read a NCT38XX register
 *
 * @param dev NCT38XX device
 * @param reg_addr Register address
 * @param val A pointer to a buffer for the data to return
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int nct38xx_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *val)
{
	const struct gpio_nct38xx_config *const config =
		(const struct gpio_nct38xx_config *)dev->config;
	return i2c_reg_read_byte_dt(&config->i2c_dev, reg_addr, val);
}

/**
 * @brief Read a sequence of NCT38XX registers
 *
 * @param dev NCT38XX device
 * @param start_addr The register start address
 * @param buf A pointer to a buffer for the data to return
 * @param num_bytes Number of data to read
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int nct38xx_reg_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
					 uint32_t num_bytes)
{
	const struct gpio_nct38xx_config *const config =
		(const struct gpio_nct38xx_config *)dev->config;
	return i2c_burst_read_dt(&config->i2c_dev, start_addr, buf, num_bytes);
}

/**
 * @brief Write a NCT38XX register
 *
 * @param dev NCT38XX device
 * @param reg_addr Register address
 * @param val Data to write
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int nct38xx_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	const struct gpio_nct38xx_config *const config =
		(const struct gpio_nct38xx_config *)dev->config;
	return i2c_reg_write_byte_dt(&config->i2c_dev, reg_addr, val);
}

/**
 * @brief Write a sequence of NCT38XX registers
 *
 * @param dev NCT38XX device
 * @param start_addr The register start address
 * @param buf A pointer to a buffer for the data to write
 * @param num_bytes Number of data to write
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int nct38xx_reg_burst_write(const struct device *dev, uint8_t start_addr,
					  uint8_t *buf, uint32_t num_bytes)
{
	const struct gpio_nct38xx_config *const config =
		(const struct gpio_nct38xx_config *)dev->config;
	return i2c_burst_write_dt(&config->i2c_dev, start_addr, buf, num_bytes);
}

/**
 * @brief Compare data & write a NCT38XX register
 *
 * @param dev NCT38XX device
 * @param reg_addr Register address
 * @param reg_val Old register data
 * @param new_val New register data
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int nct38xx_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t reg_val,
				     uint8_t new_val)
{
	if (reg_val == new_val)
		return 0;

	return nct38xx_reg_write_byte(dev, reg_addr, new_val);
}

/**
 * @brief Dispatch GPIO port ISR
 *
 * @param dev GPIO port device
 * @return 0 if successful, otherwise failed.
 */
int gpio_nct38xx_dispatch_port_isr(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_NCT38XX_H_*/
