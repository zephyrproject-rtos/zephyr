/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (C) 2020, Riedo Networks Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9536

/**
 * @file Driver for PCA9536 I2C-based GPIO driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include "gpio_utils.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_pca9536);

#define REG_INPUT_PORT		0x00
#define REG_OUTPUT_PORT		0x01
#define REG_POL_INV_PORT	0x02
#define REG_CONF_PORT		0x03

/** Configuration data */
struct gpio_pca9536_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	/** The master I2C device's name */
	const char * const i2c_master_dev_name;

	/** The slave address of the chip */
	uint16_t i2c_slave_addr;

	uint8_t capabilities;
};

/** Runtime driver data */
struct gpio_pca9536_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	/** Master I2C device */
	const struct device *i2c_master;

	struct {
		uint8_t output;
		uint8_t dir;
	} reg_cache;

	struct k_sem lock;
};

/**
 * @brief Read both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, read the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCA9536.
 * @param reg Register to read (the PORT0 of the pair of registers).
 * @param buf Buffer to read data into.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_port_regs(const struct device *dev, uint8_t reg, uint8_t *buf)
{
	const struct gpio_pca9536_config * const config = dev->config;
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	const struct device * const i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	uint8_t port_data;
	int ret;

	ret = i2c_burst_read(i2c_master, i2c_addr, reg,
			     (uint8_t *)&port_data, sizeof(port_data));
	if (ret != 0) {
		LOG_ERR("PCA9536[0x%X]: error reading register 0x%X (%d)",
			i2c_addr, reg, ret);
		return ret;
	}

	*buf = sys_le16_to_cpu(port_data);

	LOG_DBG("PCA9536[0x%X]: Read: REG[0x%X] = 0x%X",
		i2c_addr, reg, (*buf & 0xFF));

	return 0;
}

/**
 * @brief Write both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, write the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCA9536.
 * @param reg Register to write into (the PORT0 of the pair of registers).
 * @param cache Pointer to the cache to be updated after successful write.
 * @param value New value to set.
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_port_regs(const struct device *dev, uint8_t reg,
			   uint8_t *cache, uint8_t value)
{
	const struct gpio_pca9536_config * const config = dev->config;
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	const struct device * const i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	uint8_t port_data;
	int ret;

	LOG_DBG("PCA9536[0x%X]: Write: REG[0x%X] = 0x%X",
		i2c_addr, reg, (value & 0xFF));

	port_data = (value);

	ret = i2c_burst_write(i2c_master, i2c_addr, reg,
			      (uint8_t *)&port_data, sizeof(port_data));
	if (ret == 0) {
		*cache = value;
	} else {
		LOG_ERR("PCA9536[0x%X]: error writing to register 0x%X (%d)",
			i2c_addr, reg, ret);
	}

	return ret;
}

static inline
int update_output_regs(const struct device *dev, uint8_t value)
{
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;

	return write_port_regs(dev, REG_OUTPUT_PORT,
			       &drv_data->reg_cache.output, value);
}

static inline
int update_direction_regs(const struct device *dev, uint8_t value)
{
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;

	return write_port_regs(dev, REG_CONF_PORT,
			       &drv_data->reg_cache.dir, value);
}

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the PCA9536
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	uint8_t reg_dir = drv_data->reg_cache.dir;
	uint8_t reg_out = drv_data->reg_cache.output;
	int ret;

	__ASSERT(pin < 4, "Only 4 pin are supported on PCA9536");

	/* For each pin, 0 == output, 1 == input */
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			reg_out |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			reg_out &= ~BIT(pin);
		}
		reg_dir &= ~BIT(pin);
	} else {
		reg_dir |= BIT(pin);
	}

	ret = update_output_regs(dev, reg_out);
	if (ret != 0) {
		return ret;
	}

	ret = update_direction_regs(dev, reg_dir);

	return ret;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct of the PCA9536
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pca9536_config(const struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	int ret;
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;

#if (CONFIG_GPIO_LOG_LEVEL >= LOG_LEVEL_DEBUG)
	const struct gpio_pca9536_config * const config = dev->config;
	uint16_t i2c_addr = config->i2c_slave_addr;
#endif

	/* Does not support disconnected pin */
	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	/* Open-drain support is per port, not per pin.
	 * So can't really support the API as-is.
	 */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = setup_pin_dir(dev, pin, flags);
	if (ret) {
		LOG_ERR("PCA9536[0x%X]: error setting pin direction (%d)",
			i2c_addr, ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca9536_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	uint8_t buf;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = read_port_regs(dev, REG_INPUT_PORT, &buf);
	if (ret != 0) {
		goto done;
	}

	*value = buf;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca9536_port_set_masked_raw(const struct device *dev,
					    uint32_t mask, uint32_t value)
{
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	uint8_t reg_out;
	int ret;

	__ASSERT(!((mask & 0xFFFFFFF0) || (value & 0xFFFFFFF0)), "Only 4 pin are supported on PCA9536");

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out = (reg_out & ~mask) | (mask & value);

	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

static inline
int gpio_pca9536_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return gpio_pca9536_port_set_masked_raw(dev, mask, mask);
}

static inline
int gpio_pca9536_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return gpio_pca9536_port_set_masked_raw(dev, mask, 0);
}

static int gpio_pca9536_port_toggle_bits(const struct device *dev,
					 uint32_t mask)
{
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	uint8_t reg_out;
	int ret;

	__ASSERT(!(mask & 0xFFFFFFF0), "Only 4 pin are supported on PCA9536");

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out ^= mask;

	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int gpio_pca9536_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api gpio_pca9536_drv_api_funcs = {
	.pin_configure = gpio_pca9536_config,
	.port_get_raw = gpio_pca9536_port_get_raw,
	.port_set_masked_raw = gpio_pca9536_port_set_masked_raw,
	.port_set_bits_raw = gpio_pca9536_port_set_bits_raw,
	.port_clear_bits_raw = gpio_pca9536_port_clear_bits_raw,
	.port_toggle_bits = gpio_pca9536_port_toggle_bits,
	.pin_interrupt_configure = gpio_pca9536_pin_interrupt_configure,
};

/**
 * @brief Initialization function of PCA9536
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_pca9536_init(const struct device *dev)
{
	const struct gpio_pca9536_config * const config = dev->config;
	struct gpio_pca9536_drv_data * const drv_data =
		(struct gpio_pca9536_drv_data * const)dev->data;
	const struct device *i2c_master;

	/* Find out the device struct of the I2C master */
	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	if (!i2c_master) {
		return -EINVAL;
	}
	drv_data->i2c_master = i2c_master;

	k_sem_init(&drv_data->lock, 1, 1);

	return 0;
}

#define GPIO_PCA9536_DEVICE_INSTANCE(inst)				\
static const struct gpio_pca9536_config gpio_pca9536_##inst##_cfg = {	\
	.common = {							\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),	\
	},								\
	.i2c_master_dev_name = DT_INST_BUS_LABEL(inst),			\
	.i2c_slave_addr = DT_INST_REG_ADDR(inst),			\
	.capabilities = 0,						\
};									\
									\
static struct gpio_pca9536_drv_data gpio_pca9536_##inst##_drvdata = {	\
	.reg_cache.output = 0xFF,					\
	.reg_cache.dir = 0xFF,						\
};									\
									\
DEVICE_AND_API_INIT(gpio_pca9536_##inst,				\
	DT_INST_LABEL(inst),						\
	gpio_pca9536_init,						\
	&gpio_pca9536_##inst##_drvdata,					\
	&gpio_pca9536_##inst##_cfg,					\
	POST_KERNEL, CONFIG_GPIO_PCA9536_INIT_PRIORITY,			\
	&gpio_pca9536_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PCA9536_DEVICE_INSTANCE)
