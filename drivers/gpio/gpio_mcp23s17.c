/*
 * Copyright (c) 2019 Geanix ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for MCP23S17 SPI-based GPIO driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <misc/util.h>
#include <gpio.h>
#include <spi.h>

#include "gpio_mcp23s17.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
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
static int read_port_regs(struct device *dev, u8_t reg,
			   union mcp23s17_port_data *buf)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	int ret;

	u8_t addr = MCP23S17_ADDR | MCP23S17_READBIT;
	u8_t buffer_tx[4] = { addr, reg, 0, 0 };

	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 4,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};
	rx_buf[0].buf = NULL;
	rx_buf[0].len = 1;
	rx_buf[1].buf = &buf->byte;
	rx_buf[1].len = 1;

	ret = spi_transceive(drv_data->spi, &drv_data->spi_cfg, &tx, &rx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	LOG_DBG("MCP23S17: Read: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X",
		reg, buf->byte[0], (reg + 1), buf->byte[1]);

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
static int write_port_regs(struct device *dev, u8_t reg,
			    union mcp23s17_port_data *buf)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	int ret;

	u8_t addr = MCP23S17_ADDR;
	u8_t buffer_tx[2] = { addr, reg };

	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 2,
		},
		{
			.buf = &buf->byte,
			.len = 2,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2,
	};

	ret = spi_write(drv_data->spi, &drv_data->spi_cfg, &tx);
	if (ret) {
		LOG_DBG("spi_write FAIL %d\n", ret);
		return ret;
	}

	LOG_DBG("MCP23S17: Write: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X",
		reg, buf->byte[0], (reg + 1), buf->byte[1]);
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
static int setup_pin_dir(struct device *dev, u32_t pin, int flags)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data *port = &drv_data->reg_cache.iodir;
	u16_t bit_mask;
	u16_t new_value = 0;
	int ret;

	bit_mask = 1 << pin;

	/* Config 0 == output, 1 == input */
	if ((flags & GPIO_DIR_MASK) == GPIO_INPUT) {
		new_value = 1 << pin;
	}

	port->all &= ~bit_mask;
	port->all |= new_value;

	ret = write_port_regs(dev, REG_IODIR_PORTA, port);

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
static int setup_pin_pullupdown(struct device *dev, u32_t pin, int flags)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data *port;
	u16_t bit_mask;
	u16_t new_value = 0;
	int ret;

	/* Setup pin pull up or pull down */
	port = &drv_data->reg_cache.gppu;
	bit_mask = 1 << pin;

	/* pull down == 0, pull up == 1 */
	if ((flags & GPIO_PUD_MASK) == GPIO_PULL_UP) {
		new_value = 1 << pin;
	} else if ((flags & GPIO_PUD_MASK) == GPIO_PULL_DOWN) {
		return -ENOTSUP;
	}

	port->all &= ~bit_mask;
	port->all |= new_value;

	ret = write_port_regs(dev, REG_GPPU_PORTA, port);

	return ret;
}

static int mcp23s17_config(struct device *dev, int access_op, u32_t pin,
			   int flags)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	int ret;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

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

static int mcp23s17_write(struct device *dev, int access_op, u32_t pin,
			  u32_t value)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data *port = &drv_data->reg_cache.gpio;
	u16_t bit_mask;
	u16_t new_value;
	int ret;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	bit_mask = BIT(pin);

	new_value = (value << pin) & bit_mask;
	new_value &= bit_mask;

	port->all &= ~bit_mask;
	port->all |= new_value;

	ret = write_port_regs(dev, REG_GPIO_PORTA, port);

	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23s17_read(struct device *dev, int access_op,
			      u32_t pin, u32_t *value)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data buf;
	int ret;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = read_port_regs(dev, REG_GPIO_PORTA, &buf);
	if (ret != 0) {
		goto done;
	}

	*value = (buf.all >> pin) & 0x01;

 done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23s17_port_get_raw(struct device *dev, u32_t *value)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data buf;
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

	*value = buf.all;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int mcp23s17_port_set_masked_raw(struct device *dev,
					      u32_t mask, u32_t value)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data buf;
	int ret;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	buf.all = drv_data->reg_cache.gpio.all;
	buf.all = (buf.all & ~mask) | (mask & value);

	ret = write_port_regs(dev, REG_GPIO_PORTA, &buf);
	if (!ret) {
		drv_data->reg_cache.gpio.all = buf.all;
	}

	k_sem_give(&drv_data->lock);

	return ret;
}

static int mcp23s17_port_set_bits_raw(struct device *dev, u32_t mask)
{
	return mcp23s17_port_set_masked_raw(dev, mask, mask);
}

static int mcp23s17_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	return mcp23s17_port_set_masked_raw(dev, mask, 0);
}

static int mcp23s17_port_toggle_bits(struct device *dev, u32_t mask)
{
	struct mcp23s17_drv_data *const drv_data =
	    (struct mcp23s17_drv_data * const)dev->driver_data;
	union mcp23s17_port_data buf;
	int ret;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	buf.all = drv_data->reg_cache.gpio.all;
	buf.all ^= mask;

	ret = write_port_regs(dev, REG_GPIO_PORTA, &buf);
	if (!ret) {
		drv_data->reg_cache.gpio.all = buf.all;
	}

	k_sem_give(&drv_data->lock);

	return ret;
}

static int mcp23s17_pin_interrupt_configure(struct device *dev,
					    unsigned int pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static const struct gpio_driver_api api_table = {
	.config = mcp23s17_config,
	.write = mcp23s17_write,
	.read = mcp23s17_read,
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
static int mcp23s17_init(struct device *dev)
{
	const struct mcp23s17_config * const config =
		dev->config->config_info;
	struct mcp23s17_drv_data *const drv_data =
		(struct mcp23s17_drv_data * const)dev->driver_data;

	drv_data->spi = device_get_binding((char *)config->spi_dev_name);
	if (!drv_data->spi) {
		LOG_DBG("Unable to get SPI device");
		return -ENODEV;
	}

	if (config->cs_dev) {
		/* handle SPI CS thru GPIO if it is the case */
		drv_data->mcp23s17_cs_ctrl.gpio_dev =
			device_get_binding(config->cs_dev);
		if (!drv_data->mcp23s17_cs_ctrl.gpio_dev) {
			LOG_ERR("Unable to get GPIO SPI CS device");
			return -ENODEV;
		}

		drv_data->mcp23s17_cs_ctrl.gpio_pin = config->cs_pin;
		drv_data->mcp23s17_cs_ctrl.delay = 0;

		drv_data->spi_cfg.cs = &drv_data->mcp23s17_cs_ctrl;

		LOG_DBG("SPI GPIO CS configured on %s:%u",
				config->cs_dev, config->cs_pin);
	}

	drv_data->spi_cfg.frequency = config->freq;
	drv_data->spi_cfg.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE);
	drv_data->spi_cfg.slave = config->slave;

	k_sem_init(&drv_data->lock, 1, 1);

	dev->driver_api = &api_table;

	return 0;
}

/* Initialization for MCP23S17_0 */
#ifdef DT_INST_0_MICROCHIP_MCP23S17
static struct mcp23s17_config mcp23s17_0_config = {
	.spi_dev_name = DT_INST_0_MICROCHIP_MCP23S17_BUS_NAME,
	.slave = DT_INST_0_MICROCHIP_MCP23S17_BASE_ADDRESS,
	.freq = DT_INST_0_MICROCHIP_MCP23S17_SPI_MAX_FREQUENCY,
#ifdef DT_INST_0_MICROCHIP_MCP23S17_CS_GPIOS_CONTROLLER
	.cs_dev = DT_INST_0_MICROCHIP_MCP23S17_CS_GPIOS_CONTROLLER,
#endif
#ifdef DT_INST_0_MICROCHIP_MCP23S17_CS_GPIOS_PIN
	.cs_pin = DT_INST_0_MICROCHIP_MCP23S17_CS_GPIOS_PIN,
#endif
};

static struct mcp23s17_drv_data mcp23s17_0_drvdata = {
	/* Default for registers according to datasheet */
	.reg_cache.iodir = { .all = 0xFFFF },
	.reg_cache.ipol = { .all = 0x0 },
	.reg_cache.gpinten = { .all = 0x0 },
	.reg_cache.defval = { .all = 0x0 },
	.reg_cache.intcon = { .all = 0x0 },
	.reg_cache.iocon = { .all = 0x0 },
	.reg_cache.gppu = { .all = 0x0 },
	.reg_cache.intf = { .all = 0x0 },
	.reg_cache.intcap = { .all = 0x0 },
	.reg_cache.gpio = { .all = 0x0 },
	.reg_cache.olat = { .all = 0x0 },
};

/* This has to init after SPI master */
DEVICE_AND_API_INIT(mcp23s17_0, DT_INST_0_MICROCHIP_MCP23S17_LABEL,
	    mcp23s17_init, &mcp23s17_0_drvdata, &mcp23s17_0_config,
	    POST_KERNEL, CONFIG_GPIO_MCP23S17_INIT_PRIORITY,
	    &api_table);
#endif	/* DT_INST_0_MICROCHIP_MCP23S17 */

/* Initialization for MCP23S17_1 */
#ifdef DT_INST_1_MICROCHIP_MCP23S17
static struct mcp23s17_config mcp23s17_1_config = {
	.spi_dev_name = DT_INST_1_MICROCHIP_MCP23S17_BUS_NAME,
	.slave = DT_INST_1_MICROCHIP_MCP23S17_BASE_ADDRESS,
	.freq = DT_INST_1_MICROCHIP_MCP23S17_SPI_MAX_FREQUENCY,
#ifdef DT_INST_1_MICROCHIP_MCP23S17_CS_GPIOS_CONTROLLER
	.cs_dev = DT_INST_1_MICROCHIP_MCP23S17_CS_GPIOS_CONTROLLER,
#endif
#ifdef DT_INST_1_MICROCHIP_MCP23S17_CS_GPIOS_PIN
	.cs_pin = DT_INST_1_MICROCHIP_MCP23S17_CS_GPIOS_PIN,
#endif
};

static struct mcp23s17_drv_data mcp23s17_1_drvdata = {
	/* Default for registers according to datasheet */
	.reg_cache.iodir = { .all = 0xFFFF },
	.reg_cache.ipol = { .all = 0x0 },
	.reg_cache.gpinten = { .all = 0x0 },
	.reg_cache.defval = { .all = 0x0 },
	.reg_cache.intcon = { .all = 0x0 },
	.reg_cache.iocon = { .all = 0x0 },
	.reg_cache.gppu = { .all = 0x0 },
	.reg_cache.intf = { .all = 0x0 },
	.reg_cache.intcap = { .all = 0x0 },
	.reg_cache.gpio = { .all = 0x0 },
	.reg_cache.olat = { .all = 0x0 },
};

/* This has to init after SPI master */
DEVICE_AND_API_INIT(mcp23s17_1, DT_INST_1_MICROCHIP_MCP23S17_LABEL,
	    mcp23s17_init, &mcp23s17_1_drvdata, &mcp23s17_1_config,
	    POST_KERNEL, CONFIG_GPIO_MCP23S17_INIT_PRIORITY,
	    &api_table);
#endif	/* DT_INST_0_MICROCHIP_MCP23S17 */
