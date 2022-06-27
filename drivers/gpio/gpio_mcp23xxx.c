/*
 *
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for MPC23xxx I2C/SPI-based GPIO driver.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#include "gpio_utils.h"
#include "gpio_mcp23xxx.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp23xxx);

/**
 * @brief Reads given register from mcp23xxx.
 *
 * The registers of the mcp23x0x consist of one 8 bit port.
 * The registers of the mcp23x1x consist of two 8 bit ports.
 *
 * @param dev The mcp23xxx device.
 * @param reg The register to be read.
 * @param buf The buffer to read data to.
 * @return 0 if successful.
 *		   Otherwise <0 will be returned.
 */
static int read_port_regs(const struct device *dev, uint8_t reg, uint16_t *buf)
{
	const struct mcp23xxx_config *config = dev->config;

	if (config->ngpios == 16U) {
		reg *= 2;
	}

	return config->read_fn(dev, reg, buf);
}

/**
 * @brief Writes registers of the mcp23xxx.
 *
 * On the mcp23x08 one 8 bit port will be written.
 * On the mcp23x17 two 8 bit ports will be written.
 *
 * @param dev The mcp23xxx device.
 * @param reg Register to be written.
 * @param buf The new register value.
 *
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int write_port_regs(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct mcp23xxx_config *config = dev->config;

	if (config->ngpios == 16U) {
		reg *= 2;
	}

	return config->write_fn(dev, reg, value);
}

/**
 * @brief Setup the pin direction.
 *
 * @param dev The mcp23xxx device.
 * @param pin The pin number.
 * @param flags Flags of pin or port.
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	uint16_t dir = drv_data->reg_cache.iodir;
	uint16_t output = drv_data->reg_cache.gpio;
	int ret;

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			output |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			output &= ~BIT(pin);
		}
		dir &= ~BIT(pin);
	} else {
		dir |= BIT(pin);
	}

	ret = write_port_regs(dev, REG_GPIO, output);
	if (ret != 0) {
		return ret;
	}

	drv_data->reg_cache.gpio = output;

	ret = write_port_regs(dev, REG_IODIR, dir);
	if (ret == 0) {
		drv_data->reg_cache.iodir = dir;
	}

	return ret;
}

/**
 * @brief Setup pin pull up/pull down.
 *
 * @param dev The mcp23xxx device.
 * @param pin The pin number.
 * @param flags Flags of pin or port.
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int setup_pin_pull(const struct device *dev, uint32_t pin, int flags)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	uint16_t port;
	int ret;

	port = drv_data->reg_cache.gppu;

	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	WRITE_BIT(port, pin, (flags & GPIO_PULL_UP) != 0);

	ret = write_port_regs(dev, REG_GPPU, port);
	if (ret == 0) {
		drv_data->reg_cache.gppu = port;
	}

	return ret;
}

static int mcp23xxx_pin_cfg(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		ret = -ENOTSUP;
		goto done;
	}

	ret = setup_pin_dir(dev, pin, flags);
	if (ret < 0) {
		LOG_ERR("Error setting pin direction (%d)", ret);
		goto done;
	}

	ret = setup_pin_pull(dev, pin, flags);
	if (ret < 0) {
		LOG_ERR("Error setting pin pull up/pull down (%d)", ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23xxx_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	uint16_t buf;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = read_port_regs(dev, REG_GPIO, &buf);
	if (ret == 0) {
		*value = buf;
	}

	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23xxx_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	uint16_t buf;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	buf = drv_data->reg_cache.gpio;
	buf = (buf & ~mask) | (mask & value);

	ret = write_port_regs(dev, REG_GPIO, buf);
	if (ret == 0) {
		drv_data->reg_cache.gpio = buf;
	}

	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23xxx_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return mcp23xxx_port_set_masked_raw(dev, mask, mask);
}

static int mcp23xxx_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return mcp23xxx_port_set_masked_raw(dev, mask, 0);
}

static int mcp23xxx_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	uint16_t buf;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	buf = drv_data->reg_cache.gpio;
	buf ^= mask;

	ret = write_port_regs(dev, REG_GPIO, buf);
	if (ret == 0) {
		drv_data->reg_cache.gpio = buf;
	}

	k_sem_give(&drv_data->lock);

	return ret;
}

static int mcp23xxx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

const struct gpio_driver_api gpio_mcp23xxx_api_table = {
	.pin_configure = mcp23xxx_pin_cfg,
	.port_get_raw = mcp23xxx_port_get_raw,
	.port_set_masked_raw = mcp23xxx_port_set_masked_raw,
	.port_set_bits_raw = mcp23xxx_port_set_bits_raw,
	.port_clear_bits_raw = mcp23xxx_port_clear_bits_raw,
	.port_toggle_bits = mcp23xxx_port_toggle_bits,
	.pin_interrupt_configure = mcp23xxx_pin_interrupt_configure,
};

/**
 * @brief Initialization function of MCP23XXX
 *
 * @param dev Device struct.
 * @return 0 if successful. Otherwise <0 is returned.
 */
int gpio_mcp23xxx_init(const struct device *dev)
{
	const struct mcp23xxx_config *config = dev->config;
	struct mcp23xxx_drv_data *drv_data = dev->data;
	int err;

	if (config->ngpios != 8U && config->ngpios != 16U) {
		LOG_ERR("Invalid value ngpios=%u. Expected 8 or 16!", config->ngpios);
		return -EINVAL;
	}

	err = config->bus_fn(dev);
	if (err < 0) {
		return err;
	}

	k_sem_init(&drv_data->lock, 1, 1);

	return 0;
}
