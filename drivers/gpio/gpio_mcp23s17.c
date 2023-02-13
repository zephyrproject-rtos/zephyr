/*
 * Copyright (c) 2020 Geanix ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp23s17

/**
 * @file Driver for MCP23S17 SPI-based GPIO driver.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_mcp23s17.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp23s17);

/**
 * @brief Read both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, read the pair of port 0 and port 1.
 *
 * @param dev Device struct of the MCP23S17.
 * @param reg Register to read (the PORTA of the pair of registers).
 * @param buf Buffer to read data into.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_port_regs(const struct device *dev, uint8_t reg,
			  uint16_t *buf)
{
	const struct mcp23s17_config *config = dev->config;
	int ret;
	uint16_t port_data;

	uint8_t addr = MCP23S17_ADDR | MCP23S17_READBIT;
	uint8_t buffer_tx[4] = { addr, reg, 0, 0 };

	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 4,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 2
		},
		{
			.buf = (uint8_t *)&port_data,
			.len = 2
		}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(&config->bus, &tx, &rx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	*buf = sys_le16_to_cpu(port_data);

	LOG_DBG("MCP23S17: Read: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X",
		reg, (*buf & 0xFF), (reg + 1), (*buf >> 8));

	return 0;
}

/**
 * @brief Write both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, write the pair of port 0 and port 1.
 *
 * @param dev Device struct of the MCP23S17.
 * @param reg Register to write into (the PORTA of the pair of registers).
 * @param buf Buffer to write data from.
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_port_regs(const struct device *dev, uint8_t reg,
			   uint16_t value)
{
	const struct mcp23s17_config *config = dev->config;
	int ret;
	uint16_t port_data;

	LOG_DBG("MCP23S17: Write: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X",
		reg, (value & 0xFF), (reg + 1), (value >> 8));

	port_data = sys_cpu_to_le16(value);

	uint8_t addr = MCP23S17_ADDR;
	uint8_t buffer_tx[2] = { addr, reg };

	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 2,
		},
		{
			.buf = (uint8_t *)&port_data,
			.len = 2,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	ret = spi_write_dt(&config->bus, &tx);
	if (ret) {
		LOG_DBG("spi_write FAIL %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the MCP23S17
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct mcp23s17_drv_data *drv_data = dev->data;
	uint16_t *dir = &drv_data->reg_cache.iodir;
	uint16_t *output = &drv_data->reg_cache.gpio;
	int ret;

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			*output |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			*output &= ~BIT(pin);
		}
		*dir &= ~BIT(pin);
	} else {
		*dir |= BIT(pin);
	}

	ret = write_port_regs(dev, REG_GPIO_PORTA, *output);
	if (ret != 0) {
		return ret;
	}

	ret = write_port_regs(dev, REG_IODIR_PORTA, *dir);

	return ret;
}

/**
 * @brief Setup the pin pull up/pull down status
 *
 * @param dev Device struct of the MCP23S17
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_pullupdown(const struct device *dev, uint32_t pin,
				int flags)
{
	struct mcp23s17_drv_data *drv_data = dev->data;
	uint16_t port;
	int ret;

	/* Setup pin pull up or pull down */
	port = drv_data->reg_cache.gppu;

	/* pull down == 0, pull up == 1 */
	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	WRITE_BIT(port, pin, (flags & GPIO_PULL_UP) != 0U);

	ret = write_port_regs(dev, REG_GPPU_PORTA, port);
	if (ret == 0) {
		drv_data->reg_cache.gppu = port;
	}

	return ret;
}

static int mcp23s17_config(const struct device *dev,
			   gpio_pin_t pin, gpio_flags_t flags)
{
	struct mcp23s17_drv_data *drv_data = dev->data;
	int ret;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	if ((flags & GPIO_OPEN_DRAIN) != 0U) {
		ret = -ENOTSUP;
		goto done;
	}

	ret = setup_pin_dir(dev, pin, flags);
	if (ret) {
		LOG_ERR("MCP23S17: error setting pin direction (%d)", ret);
		goto done;
	}

	ret = setup_pin_pullupdown(dev, pin, flags);
	if (ret) {
		LOG_ERR("MCP23S17: error setting pin pull up/down (%d)", ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23s17_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct mcp23s17_drv_data *drv_data = dev->data;
	uint16_t buf;
	int ret;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = read_port_regs(dev, REG_GPIO_PORTA, &buf);
	if (ret != 0) {
		goto done;
	}

	*value = buf;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23s17_port_set_masked_raw(const struct device *dev,
					uint32_t mask, uint32_t value)
{
	struct mcp23s17_drv_data *drv_data = dev->data;
	uint16_t buf;
	int ret;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	buf = drv_data->reg_cache.gpio;
	buf = (buf & ~mask) | (mask & value);

	ret = write_port_regs(dev, REG_GPIO_PORTA, buf);
	if (ret == 0) {
		drv_data->reg_cache.gpio = buf;
	}

	k_sem_give(&drv_data->lock);

	return ret;
}

static int mcp23s17_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return mcp23s17_port_set_masked_raw(dev, mask, mask);
}

static int mcp23s17_port_clear_bits_raw(const struct device *dev,
					uint32_t mask)
{
	return mcp23s17_port_set_masked_raw(dev, mask, 0);
}

static int mcp23s17_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	struct mcp23s17_drv_data *const drv_data =
		(struct mcp23s17_drv_data *const)dev->data;
	uint16_t buf;
	int ret;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	buf = drv_data->reg_cache.gpio;
	buf ^= mask;

	ret = write_port_regs(dev, REG_GPIO_PORTA, buf);
	if (ret == 0) {
		drv_data->reg_cache.gpio = buf;
	}

	k_sem_give(&drv_data->lock);

	return ret;
}

static int mcp23s17_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api api_table = {
	.pin_configure = mcp23s17_config,
	.port_get_raw = mcp23s17_port_get_raw,
	.port_set_masked_raw = mcp23s17_port_set_masked_raw,
	.port_set_bits_raw = mcp23s17_port_set_bits_raw,
	.port_clear_bits_raw = mcp23s17_port_clear_bits_raw,
	.port_toggle_bits = mcp23s17_port_toggle_bits,
	.pin_interrupt_configure = mcp23s17_pin_interrupt_configure,
};

/**
 * @brief Initialization function of MCP23S17
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int mcp23s17_init(const struct device *dev)
{
	const struct mcp23s17_config *config = dev->config;
	struct mcp23s17_drv_data *drv_data = dev->data;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	k_sem_init(&drv_data->lock, 1, 1);

	return 0;
}

#define MCP23S17_INIT(inst)						 \
	static const struct mcp23s17_config mcp23s17_##inst##_config = { \
		.common = {						 \
			.port_pin_mask =				 \
				GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),	 \
		},							 \
		.bus = SPI_DT_SPEC_INST_GET(				 \
			inst,						 \
			SPI_OP_MODE_MASTER | SPI_MODE_CPOL |		 \
			SPI_MODE_CPHA | SPI_WORD_SET(8), 0),		 \
	};								 \
									 \
	static struct mcp23s17_drv_data mcp23s17_##inst##_drvdata = {	 \
		/* Default for registers according to datasheet */	 \
		.reg_cache.iodir = 0xFFFF,				 \
		.reg_cache.ipol = 0x0,					 \
		.reg_cache.gpinten = 0x0,				 \
		.reg_cache.defval = 0x0,				 \
		.reg_cache.intcon = 0x0,				 \
		.reg_cache.iocon = 0x0,					 \
		.reg_cache.gppu = 0x0,					 \
		.reg_cache.intf = 0x0,					 \
		.reg_cache.intcap = 0x0,				 \
		.reg_cache.gpio = 0x0,					 \
		.reg_cache.olat = 0x0,					 \
	};								 \
									 \
	/* This has to init after SPI master */				 \
	DEVICE_DT_INST_DEFINE(inst, mcp23s17_init,			 \
			      NULL,					 \
			      &mcp23s17_##inst##_drvdata,		 \
			      &mcp23s17_##inst##_config,		 \
			      POST_KERNEL,				 \
			      CONFIG_GPIO_MCP23S17_INIT_PRIORITY,	 \
			      &api_table);

DT_INST_FOREACH_STATUS_OKAY(MCP23S17_INIT)
