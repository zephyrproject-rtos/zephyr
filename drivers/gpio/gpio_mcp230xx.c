/*
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for MPC230xx I2C-based GPIO driver.
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include "gpio_utils.h"
#include "gpio_mcp230xx.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp230xx);

/**
 * @brief Reads given register from mcp230xx.
 *
 * The registers of the mcp23008 consist of one 8 bit port.
 * The registers of the mcp23017 consist of two 8 bit ports.
 *
 * @param dev The mcp230xx device.
 * @param reg The register to be read.
 * @param buf The buffer to read data to.
 * @return 0 if successful.
 *		   Otherwise <0 will be returned.
 */
static int read_port_regs(const struct device *dev, uint8_t reg, uint16_t *buf)
{
	const struct mcp230xx_config *config = dev->config;
	uint16_t port_data = 0;
	int ret;

	uint8_t nread = (config->ngpios == 8) ? 1 : 2;

	ret = i2c_burst_read(config->i2c.bus, config->i2c.addr, reg, (uint8_t *)&port_data, nread);
	if (ret) {
		LOG_ERR("i2c_read failed!");
		return ret;
	}

	*buf = sys_le16_to_cpu(port_data);

	return 0;
}

/**
 * @brief Writes registers of the mcp230xx.
 *
 * On the mcp23008 one 8 bit port will be written.
 * On the mcp23017 two 8 bit ports will be written.
 *
 * @param dev The mcp230xx device.
 * @param reg Register to be written.
 * @param buf The new register value.
 *
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int write_port_regs(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct mcp230xx_config *config = dev->config;
	int ret;

	uint8_t nwrite = (config->ngpios == 8) ? 1 : 2;
	uint16_t port_data = sys_cpu_to_le16(value);

	ret = i2c_burst_write(config->i2c.bus, config->i2c.addr, reg, (uint8_t *)&port_data,
			      nwrite);
	if (ret) {
		LOG_ERR("i2c_write failed!");
		return ret;
	}

	return 0;
}

/**
 * @brief Setup the pin direction.
 *
 * @param dev The mcp230xx device.
 * @param pin The pin number.
 * @param flags Flags of pin or port.
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;
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
 * @param dev The mcp230xx device.
 * @param pin The pin number.
 * @param flags Flags of pin or port.
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int setup_pin_pull(const struct device *dev, uint32_t pin, int flags)
{
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;
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

static int mcp230xx_pin_cfg(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;
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
	if (ret) {
		LOG_ERR("Error setting pin direction (%d)", ret);
		goto done;
	}

	ret = setup_pin_pull(dev, pin, flags);
	if (ret) {
		LOG_ERR("Error setting pin pull up/pull down (%d)", ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp230xx_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;
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

static int mcp230xx_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;
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

static int mcp230xx_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return mcp230xx_port_set_masked_raw(dev, mask, mask);
}

static int mcp230xx_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return mcp230xx_port_set_masked_raw(dev, mask, 0);
}

static int mcp230xx_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;
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

static int mcp230xx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api api_table = {
	.pin_configure = mcp230xx_pin_cfg,
	.port_get_raw = mcp230xx_port_get_raw,
	.port_set_masked_raw = mcp230xx_port_set_masked_raw,
	.port_set_bits_raw = mcp230xx_port_set_bits_raw,
	.port_clear_bits_raw = mcp230xx_port_clear_bits_raw,
	.port_toggle_bits = mcp230xx_port_toggle_bits,
	.pin_interrupt_configure = mcp230xx_pin_interrupt_configure,
};

/**
 * @brief Initialization function of MCP230XX
 *
 * @param dev Device struct.
 * @return 0 if successful. Otherwise <0 is returned.
 */
static int mcp230xx_init(const struct device *dev)
{
	const struct mcp230xx_config *config = dev->config;
	struct mcp230xx_drv_data *drv_data = (struct mcp230xx_drv_data *)dev->data;

	if (config->ngpios != 8 && config->ngpios != 16) {
		LOG_ERR("Invalid value ngpios=%u. Expected 8 or 16!", config->ngpios);
		return -EINVAL;
	}

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("%s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	k_sem_init(&drv_data->lock, 1, 1);

	return 0;
}

#define MCP230XX_INIT(inst)                                                                        \
	static struct mcp230xx_config mcp230xx_##inst##_config = {     \
		.config = {					       \
			.port_pin_mask =			       \
				GPIO_PORT_PIN_MASK_FROM_DT_INST(inst), \
		},						       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),		   \
		.ngpios =  DT_INST_PROP(inst, ngpios),		       \
	};                           \
\
	static struct mcp230xx_drv_data mcp230xx_##inst##_drvdata = {                              \
		/* Default for registers according to datasheet */                                 \
		.reg_cache.iodir = 0xFFFF, .reg_cache.ipol = 0x0,   .reg_cache.gpinten = 0x0,      \
		.reg_cache.defval = 0x0,   .reg_cache.intcon = 0x0, .reg_cache.iocon = 0x0,        \
		.reg_cache.gppu = 0x0,	   .reg_cache.intf = 0x0,   .reg_cache.intcap = 0x0,       \
		.reg_cache.gpio = 0x0,	   .reg_cache.olat = 0x0,                                  \
	};                                                                                         \
\
	/* This has to init after SPI master */                                                    \
	DEVICE_DT_INST_DEFINE(inst, mcp230xx_init, NULL, &mcp230xx_##inst##_drvdata,               \
			      &mcp230xx_##inst##_config, POST_KERNEL,                              \
			      CONFIG_GPIO_MCP230XX_INIT_PRIORITY, &api_table);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23008
DT_INST_FOREACH_STATUS_OKAY(MCP230XX_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23017
DT_INST_FOREACH_STATUS_OKAY(MCP230XX_INIT)
