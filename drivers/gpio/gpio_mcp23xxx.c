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
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_mcp23xxx.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp23xxx);

#define MCP23XXX_RESET_TIME_US 1

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
 * @brief Writes to the IOCON register of the mcp23xxx.
 *
 * IOCON is the only register that is not 16 bits wide on 16-pin devices; instead, it is mirrored in
 * two adjacent memory locations. Because the underlying `write_fn` always does a 16-bit write for
 * 16-pin devices, make sure we write the same value to both IOCON locations.
 *
 * @param dev The mcp23xxx device.
 * @param value the IOCON value to write
 *
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int write_iocon(const struct device *dev, uint8_t value)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;

	uint16_t extended_value = value | (value << 8);
	int ret = write_port_regs(dev, REG_IOCON, extended_value);

	if (ret == 0) {
		drv_data->reg_cache.iocon = extended_value;
	}

	return ret;
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
	struct mcp23xxx_drv_data *drv_data = dev->data;
	const struct mcp23xxx_config *config = dev->config;

	if (!config->gpio_int.port) {
		return -ENOTSUP;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	uint16_t gpinten = drv_data->reg_cache.gpinten;
	uint16_t defval = drv_data->reg_cache.defval;
	uint16_t intcon = drv_data->reg_cache.intcon;

	int ret;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		gpinten &= ~BIT(pin);
		break;

	case GPIO_INT_MODE_LEVEL:
		gpinten |= BIT(pin);
		intcon |= BIT(pin);

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			defval |= BIT(pin);
			break;
		case GPIO_INT_TRIG_HIGH:
			defval &= ~BIT(pin);
			break;
		case GPIO_INT_TRIG_BOTH:
			/* can't happen */
			ret = -ENOTSUP;
			goto done;
		}
		break;

	case GPIO_INT_MODE_EDGE:
		gpinten |= BIT(pin);
		intcon &= ~BIT(pin);

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			drv_data->rising_edge_ints &= ~BIT(pin);
			drv_data->falling_edge_ints |= BIT(pin);
			break;
		case GPIO_INT_TRIG_HIGH:
			drv_data->rising_edge_ints |= BIT(pin);
			drv_data->falling_edge_ints &= ~BIT(pin);
			break;
		case GPIO_INT_TRIG_BOTH:
			drv_data->rising_edge_ints |= BIT(pin);
			drv_data->falling_edge_ints |= BIT(pin);
			break;
		}
		break;
	}

	ret = write_port_regs(dev, REG_GPINTEN, gpinten);
	if (ret != 0) {
		goto done;
	}
	drv_data->reg_cache.gpinten = gpinten;

	ret = write_port_regs(dev, REG_DEFVAL, defval);
	if (ret != 0) {
		goto done;
	}
	drv_data->reg_cache.defval = defval;

	ret = write_port_regs(dev, REG_INTCON, intcon);
	if (ret != 0) {
		goto done;
	}
	drv_data->reg_cache.intcon = intcon;

done:
	k_sem_give(&drv_data->lock);

	return ret;
}

static int mcp23xxx_manage_callback(const struct device *dev, struct gpio_callback *callback,
				    bool set)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	const struct mcp23xxx_config *config = dev->config;

	if (!config->gpio_int.port) {
		return -ENOTSUP;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	int ret = gpio_manage_callback(&drv_data->callbacks, callback, set);

	k_sem_give(&drv_data->lock);

	return ret;
}

static void mcp23xxx_work_handler(struct k_work *work)
{
	struct mcp23xxx_drv_data *drv_data = CONTAINER_OF(work, struct mcp23xxx_drv_data, work);
	const struct device *dev = drv_data->dev;

	int ret;

	k_sem_take(&drv_data->lock, K_FOREVER);

	uint16_t intf;

	ret = read_port_regs(dev, REG_INTF, &intf);
	if (ret != 0) {
		LOG_ERR("Failed to read INTF");
		goto done;
	}

	if (!intf) {
		/* Probable cause: REG_GPIO was read from somewhere else before the interrupt
		 * handler had a chance to run
		 */
		LOG_ERR("Spurious interrupt");
		goto done;
	}

	uint16_t intcap;

	/* Read INTCAP to acknowledge the interrupt */
	ret = read_port_regs(dev, REG_INTCAP, &intcap);
	if (ret != 0) {
		LOG_ERR("Failed to read INTCAP");
		goto done;
	}

	/* mcp23xxx does not support single-edge interrupts in hardware, filter them out manually */
	uint16_t level_ints = drv_data->reg_cache.gpinten & drv_data->reg_cache.intcon;

	intf &= level_ints | (intcap & drv_data->rising_edge_ints) |
		(~intcap & drv_data->falling_edge_ints);

	gpio_fire_callbacks(&drv_data->callbacks, dev, intf);
done:
	k_sem_give(&drv_data->lock);
}

static void mcp23xxx_int_gpio_handler(const struct device *port, struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	struct mcp23xxx_drv_data *drv_data =
		CONTAINER_OF(cb, struct mcp23xxx_drv_data, int_gpio_cb);

	k_work_submit(&drv_data->work);
}

const struct gpio_driver_api gpio_mcp23xxx_api_table = {
	.pin_configure = mcp23xxx_pin_cfg,
	.port_get_raw = mcp23xxx_port_get_raw,
	.port_set_masked_raw = mcp23xxx_port_set_masked_raw,
	.port_set_bits_raw = mcp23xxx_port_set_bits_raw,
	.port_clear_bits_raw = mcp23xxx_port_clear_bits_raw,
	.port_toggle_bits = mcp23xxx_port_toggle_bits,
	.pin_interrupt_configure = mcp23xxx_pin_interrupt_configure,
	.manage_callback = mcp23xxx_manage_callback,
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

	k_sem_init(&drv_data->lock, 0, 1);

	/* If the RESET line is available, pulse it. */
	if (config->gpio_reset.port) {
		err = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (err != 0) {
			LOG_ERR("Failed to configure RESET line: %d", err);
			return -EIO;
		}

		k_usleep(MCP23XXX_RESET_TIME_US);

		err = gpio_pin_set_dt(&config->gpio_reset, 0);
		if (err != 0) {
			LOG_ERR("Failed to deactivate RESET line: %d", err);
			return -EIO;
		}
	}

	/* If the INT line is available, configure the callback for it. */
	if (config->gpio_int.port) {
		if (config->ngpios == 16) {
			/* send both ports' interrupts through one IRQ pin */
			err = write_iocon(dev, REG_IOCON_MIRROR);

			if (err != 0) {
				LOG_ERR("Failed to enable mirrored IRQ pins: %d", err);
				return -EIO;
			}
		}

		if (!device_is_ready(config->gpio_int.port)) {
			LOG_ERR("INT port is not ready");
			return -ENODEV;
		}

		drv_data->dev = dev;
		k_work_init(&drv_data->work, mcp23xxx_work_handler);

		err = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("Failed to configure INT line: %d", err);
			return -EIO;
		}

		gpio_init_callback(&drv_data->int_gpio_cb, mcp23xxx_int_gpio_handler,
				   BIT(config->gpio_int.pin));
		err = gpio_add_callback(config->gpio_int.port, &drv_data->int_gpio_cb);
		if (err != 0) {
			LOG_ERR("Failed to add INT callback: %d", err);
			return -EIO;
		}

		err = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (err != 0) {
			LOG_ERR("Failed to configure INT interrupt: %d", err);
			return -EIO;
		}
	}

	k_sem_give(&drv_data->lock);

	return 0;
}
