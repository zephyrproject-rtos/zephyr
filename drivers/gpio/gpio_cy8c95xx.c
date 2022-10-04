/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cypress_cy8c95xx_gpio_port

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cy8c95xx, CONFIG_GPIO_LOG_LEVEL);

#include "gpio_utils.h"

/** Cache of the output configuration and data of the pins. */
struct cy8c95xx_pin_state {
	uint8_t dir;
	uint8_t data_out;
	uint8_t pull_up;
	uint8_t pull_down;
};

/** Runtime driver data */
struct cy8c95xx_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct cy8c95xx_pin_state pin_state;
	struct k_sem *lock;
};

/** Configuration data */
struct cy8c95xx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *i2c_master;
	uint16_t i2c_slave_addr;
	uint8_t port_num;
};

#define CY8C95XX_REG_INPUT_DATA0                0x00
#define CY8C95XX_REG_OUTPUT_DATA0               0x08
#define CY8C95XX_REG_PORT_SELECT                0x18
#define CY8C95XX_REG_DIR                        0x1C
#define CY8C95XX_REG_PULL_UP                    0x1D
#define CY8C95XX_REG_PULL_DOWN                  0x1E
#define CY8C95XX_REG_ID                         0x2E

static int write_pin_state(const struct cy8c95xx_config *cfg,
			   struct cy8c95xx_drv_data *drv_data,
			   struct cy8c95xx_pin_state *pins)
{
	int rc;

	rc = i2c_reg_write_byte(cfg->i2c_master, cfg->i2c_slave_addr,
				CY8C95XX_REG_OUTPUT_DATA0 + cfg->port_num, pins->data_out);
	if (rc) {
		return rc;
	}

	rc = i2c_reg_write_byte(cfg->i2c_master, cfg->i2c_slave_addr,
				CY8C95XX_REG_PORT_SELECT, cfg->port_num);
	if (rc) {
		return rc;
	}

	rc = i2c_reg_write_byte(cfg->i2c_master, cfg->i2c_slave_addr,
				CY8C95XX_REG_DIR, pins->dir);
	if (rc) {
		return rc;
	}

	rc = i2c_reg_write_byte(cfg->i2c_master, cfg->i2c_slave_addr,
				CY8C95XX_REG_PULL_UP, pins->pull_up);
	if (rc) {
		return rc;
	}

	rc = i2c_reg_write_byte(cfg->i2c_master, cfg->i2c_slave_addr,
				CY8C95XX_REG_PULL_DOWN, pins->pull_down);

	return rc;
}

static int cy8c95xx_config(const struct device *dev,
			   gpio_pin_t pin,
			   gpio_flags_t flags)
{
	const struct cy8c95xx_config *cfg = dev->config;
	struct cy8c95xx_drv_data *drv_data = dev->data;
	struct cy8c95xx_pin_state *pins = &drv_data->pin_state;
	int rc = 0;
	uint32_t io_flags;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Open-drain not implemented */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	WRITE_BIT(pins->pull_up, pin, (flags & GPIO_PULL_UP) != 0U);
	WRITE_BIT(pins->pull_down, pin, (flags & GPIO_PULL_DOWN) != 0U);

	/* Disconnected pin not implemented*/
	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if (io_flags == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	k_sem_take(drv_data->lock, K_FOREVER);

	if ((flags & GPIO_OUTPUT) != 0) {
		pins->dir &= ~BIT(pin);
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			pins->data_out &= ~BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			pins->data_out |= BIT(pin);
		}
	} else {
		pins->dir |= BIT(pin);
	}

	LOG_DBG("CFG %u %x : DIR %04x ; DAT %04x",
		pin, flags, pins->dir, pins->data_out);

	rc = write_pin_state(cfg, drv_data, pins);

	k_sem_give(drv_data->lock);
	return rc;
}

static int port_get(const struct device *dev,
		    gpio_port_value_t *value)
{
	const struct cy8c95xx_config *cfg = dev->config;
	struct cy8c95xx_drv_data *drv_data = dev->data;
	uint8_t pin_data = 0;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(drv_data->lock, K_FOREVER);

	rc = i2c_reg_read_byte(cfg->i2c_master, cfg->i2c_slave_addr,
			       CY8C95XX_REG_INPUT_DATA0 + cfg->port_num, &pin_data);

	if (rc == 0) {
		*value = pin_data;
	}

	k_sem_give(drv_data->lock);
	return rc;
}

static int port_write(const struct device *dev,
		      gpio_port_pins_t mask,
		      gpio_port_value_t value,
		      gpio_port_value_t toggle)
{
	const struct cy8c95xx_config *cfg = dev->config;
	struct cy8c95xx_drv_data *drv_data = dev->data;
	uint8_t *outp = &drv_data->pin_state.data_out;
	uint8_t out = ((*outp & ~mask) | (value & mask)) ^ toggle;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(drv_data->lock, K_FOREVER);

	int rc = i2c_reg_write_byte(cfg->i2c_master, cfg->i2c_slave_addr,
				    CY8C95XX_REG_OUTPUT_DATA0 + cfg->port_num, out);

	if (rc == 0) {
		*outp = out;
	}
	k_sem_give(drv_data->lock);

	LOG_DBG("write msk %04x val %04x tog %04x => %04x: %d",
		mask, value, toggle, out, rc);

	return rc;
}

static int port_set_masked(const struct device *dev,
			   gpio_port_pins_t mask,
			   gpio_port_value_t value)
{
	return port_write(dev, mask, value, 0);
}

static int port_set_bits(const struct device *dev,
			 gpio_port_pins_t pins)
{
	return port_write(dev, pins, pins, 0);
}

static int port_clear_bits(const struct device *dev,
			   gpio_port_pins_t pins)
{
	return port_write(dev, pins, 0, 0);
}

static int port_toggle_bits(const struct device *dev,
			    gpio_port_pins_t pins)
{
	return port_write(dev, 0, 0, pins);
}

static int pin_interrupt_configure(const struct device *dev,
				   gpio_pin_t pin,
				   enum gpio_int_mode mode,
				   enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

/**
 * @brief Initialization function of CY8C95XX
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int cy8c95xx_init(const struct device *dev)
{
	const struct cy8c95xx_config *cfg = dev->config;
	struct cy8c95xx_drv_data *drv_data = dev->data;
	int rc;
	uint8_t data = 0;

	k_sem_take(drv_data->lock, K_FOREVER);

	if (!device_is_ready(cfg->i2c_master)) {
		LOG_ERR("%s is not ready", cfg->i2c_master->name);
		rc = -ENODEV;
		goto out;
	}

	rc = i2c_reg_read_byte(cfg->i2c_master, cfg->i2c_slave_addr,
			  CY8C95XX_REG_ID, &data);
	if (rc) {
		goto out;
	}
	LOG_DBG("cy8c95xx device ID %02X", data & 0xF0);
	if ((data & 0xF0) != 0x20) {
		LOG_WRN("driver only support [0-2] ports operations");
	}

	/* Reset state mediated by initial configuration */
	drv_data->pin_state = (struct cy8c95xx_pin_state) {
		.dir = 0xFF,
		.data_out = 0xFF,
		.pull_up = 0xFF,
		.pull_down = 0x00,
	};
	rc = write_pin_state(cfg, drv_data, &drv_data->pin_state);
out:
	if (rc != 0) {
		LOG_ERR("%s init failed: %d", dev->name, rc);
	} else {
		LOG_DBG("%s init ok", dev->name);
	}
	k_sem_give(drv_data->lock);
	return rc;
}

static const struct gpio_driver_api api_table = {
	.pin_configure = cy8c95xx_config,
	.port_get_raw = port_get,
	.port_set_masked_raw = port_set_masked,
	.port_set_bits_raw = port_set_bits,
	.port_clear_bits_raw = port_clear_bits,
	.port_toggle_bits = port_toggle_bits,
	.pin_interrupt_configure = pin_interrupt_configure,
};

static struct k_sem cy8c95xx_lock = Z_SEM_INITIALIZER(cy8c95xx_lock, 1, 1);

#define GPIO_PORT_INIT(idx) \
static const struct cy8c95xx_config cy8c95xx_##idx##_cfg = { \
	.common = { \
		.port_pin_mask = 0xFF, \
	}, \
	.i2c_master = DEVICE_DT_GET(DT_BUS(DT_INST(0, cypress_cy8c95xx_gpio))), \
	.i2c_slave_addr = DT_REG_ADDR_BY_IDX(DT_INST(0, cypress_cy8c95xx_gpio), 0), \
	.port_num = DT_INST_REG_ADDR(idx), \
}; \
static struct cy8c95xx_drv_data cy8c95xx_##idx##_drvdata = { \
	.lock = &cy8c95xx_lock, \
}; \
DEVICE_DT_INST_DEFINE(idx, cy8c95xx_init, NULL, \
				&cy8c95xx_##idx##_drvdata, &cy8c95xx_##idx##_cfg, \
				POST_KERNEL, CONFIG_GPIO_CY8C95XX_INIT_PRIORITY, \
				&api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PORT_INIT)
