/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_RT1718S_H_
#define ZEPHYR_DRIVERS_GPIO_RT1718S_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#define RT1718S_GPIO_NUM 3

#define RT1718S_REG_ALERT		       0x10
#define RT1718S_REG_ALERT_VENDOR_DEFINED_ALERT BIT(15)

#define RT1718S_REG_ALERT_MASK			    0x12
#define RT1718S_REG_ALERT_MASK_VENDOR_DEFINED_ALERT BIT(15)

#define RT1718S_REG_RT_MASK8	     0xA6
#define RT1718S_REG_RT_MASK8_GPIO1_R BIT(0)
#define RT1718S_REG_RT_MASK8_GPIO2_R BIT(1)
#define RT1718S_REG_RT_MASK8_GPIO3_R BIT(2)
#define RT1718S_REG_RT_MASK8_GPIO1_F BIT(4)
#define RT1718S_REG_RT_MASK8_GPIO2_F BIT(5)
#define RT1718S_REG_RT_MASK8_GPIO3_F BIT(6)

#define RT1718S_REG_RT_INT8	    0xA8
#define RT1718S_REG_RT_INT8_GPIO1_R BIT(0)
#define RT1718S_REG_RT_INT8_GPIO2_R BIT(1)
#define RT1718S_REG_RT_INT8_GPIO3_R BIT(2)
#define RT1718S_REG_RT_INT8_GPIO1_F BIT(4)
#define RT1718S_REG_RT_INT8_GPIO2_F BIT(5)
#define RT1718S_REG_RT_INT8_GPIO3_F BIT(6)
#define RT1718S_GPIO_INT_MASK                                                                      \
	(RT1718S_REG_RT_INT8_GPIO1_R | RT1718S_REG_RT_INT8_GPIO2_R | RT1718S_REG_RT_INT8_GPIO3_R | \
	 RT1718S_REG_RT_INT8_GPIO1_F | RT1718S_REG_RT_INT8_GPIO2_F | RT1718S_REG_RT_INT8_GPIO3_F)

#define RT1718S_REG_RT_ST8	   0xAA
#define RT1718S_REG_RT_ST8_GPIO1_I BIT(0)
#define RT1718S_REG_RT_ST8_GPIO2_I BIT(1)
#define RT1718S_REG_RT_ST8_GPIO3_I BIT(2)

#define RT1718S_REG_GPIO_CTRL(pin) (0xED + pin)
#define RT1718S_REG_GPIO_CTRL_PU   BIT(5)
#define RT1718S_REG_GPIO_CTRL_PD   BIT(4)
#define RT1718S_REG_GPIO_CTRL_OD_N BIT(3)
#define RT1718S_REG_GPIO_CTRL_OE   BIT(2)
#define RT1718S_REG_GPIO_CTRL_O	   BIT(1)
#define RT1718S_REG_GPIO_CTRL_I	   BIT(0)

/* RT1718S chip driver config */
struct rt1718s_config {
	/* I2C device */
	const struct i2c_dt_spec i2c_dev;
	/* Alert GPIO pin */
	const struct gpio_dt_spec irq_gpio;
	/* GPIO port device */
	const struct device *gpio_port_dev;
};

/* RT1718S chip driver data */
struct rt1718s_data {
	/* RT1718S device */
	const struct device *dev;
	/* lock TCPCI registers access */
	struct k_sem lock_tcpci;
	/* Alert pin callback */
	struct gpio_callback gpio_cb;
	/* Alert worker */
	struct k_work alert_worker;
};

/**
 * @brief Read a RT1718S register
 *
 * @param dev RT1718S device
 * @param reg_addr Register address
 * @param val A pointer to a buffer for the data to return
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int rt1718s_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *val)
{
	const struct rt1718s_config *const config = (const struct rt1718s_config *)dev->config;

	return i2c_reg_read_byte_dt(&config->i2c_dev, reg_addr, val);
}

/**
 * @brief Read a sequence of RT1718S registers
 *
 * @param dev RT1718S device
 * @param start_addr The register start address
 * @param buf A pointer to a buffer for the data to return
 * @param num_bytes Number of data to read
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int rt1718s_reg_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
					 uint32_t num_bytes)
{
	const struct rt1718s_config *const config = (const struct rt1718s_config *)dev->config;

	return i2c_burst_read_dt(&config->i2c_dev, start_addr, buf, num_bytes);
}

/**
 * @brief Write a RT1718S register
 *
 * @param dev RT1718S device
 * @param reg_addr Register address
 * @param val Data to write
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int rt1718s_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	const struct rt1718s_config *const config = (const struct rt1718s_config *)dev->config;

	return i2c_reg_write_byte_dt(&config->i2c_dev, reg_addr, val);
}

/**
 * @brief Write a sequence of RT1718S registers
 *
 * @param dev RT1718S device
 * @param start_addr The register start address
 * @param buf A pointer to a buffer for the data to write
 * @param num_bytes Number of data to write
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int rt1718s_reg_burst_write(const struct device *dev, uint8_t start_addr,
					  uint8_t *buf, uint32_t num_bytes)
{
	const struct rt1718s_config *const config = (const struct rt1718s_config *)dev->config;

	return i2c_burst_write_dt(&config->i2c_dev, start_addr, buf, num_bytes);
}

/**
 * @brief Compare data & write a RT1718S register
 *
 * @param dev RT1718S device
 * @param reg_addr Register address
 * @param reg_val Old register data
 * @param new_val New register data
 *
 * @return 0 if successful, otherwise failed.
 */
static inline int rt1718s_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t reg_val,
				     uint8_t new_val)
{
	if (reg_val == new_val)
		return 0;

	return rt1718s_reg_write_byte(dev, reg_addr, new_val);
}

/**
 * @brief Dispatch GPIO port alert
 *
 * @param dev RT1718S device
 */
void rt1718s_gpio_alert_handler(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_GPIO_RT1718S_H_*/
