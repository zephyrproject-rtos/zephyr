/*
 * Copyright (c) 2018 Aapo Vienamo
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <gpio.h>
#include <i2c.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sx1509b, CONFIG_GPIO_LOG_LEVEL);

/** Cache of the output configuration and data of the pins. */
struct sx1509b_pin_state {
	u16_t input_disable;    /* 0x00 */
	u16_t long_slew;        /* 0x02 */
	u16_t low_drive;        /* 0x04 */
	u16_t pull_up;          /* 0x06 */
	u16_t pull_down;        /* 0x08 */
	u16_t open_drain;       /* 0x0A */
	u16_t polarity;         /* 0x0C */
	u16_t dir;              /* 0x0E */
	u16_t data;             /* 0x10 */
};

/** Runtime driver data */
struct sx1509b_drv_data {
	struct gpio_driver_data common;
	struct device *i2c_master;
	struct sx1509b_pin_state pin_state;
	struct k_sem lock;
};

/** Configuration data */
struct sx1509b_config {
	const char *i2c_master_dev_name;
	u16_t i2c_slave_addr;
};

/* General configuration register addresses */
enum {
	/* TODO: Add rest of the regs */
	SX1509B_REG_CLOCK       = 0x1e,
	SX1509B_REG_RESET       = 0x7d,
};

/* Magic values for softreset */
enum {
	SX1509B_REG_RESET_MAGIC0        = 0x12,
	SX1509B_REG_RESET_MAGIC1        = 0x34,
};

/* Register bits for SX1509B_REG_CLOCK */
enum {
	SX1509B_REG_CLOCK_FOSC_OFF      = 0 << 5,
	SX1509B_REG_CLOCK_FOSC_EXT      = 1 << 5,
	SX1509B_REG_CLOCK_FOSC_INT_2MHZ = 2 << 5,
};

/* Pin configuration register addresses */
enum {
	SX1509B_REG_INPUT_DISABLE       = 0x00,
	SX1509B_REG_PULL_UP             = 0x06,
	SX1509B_REG_PULL_DOWN           = 0x08,
	SX1509B_REG_OPEN_DRAIN          = 0x0a,
	SX1509B_REG_DIR                 = 0x0e,
	SX1509B_REG_DATA                = 0x10,
	SX1509B_REG_LED_DRIVER_ENABLE   = 0x20,
};

/**
 * @brief Write a big-endian word to an internal address of an I2C slave.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_word_be(struct device *dev, u16_t dev_addr,
					u8_t reg_addr, u16_t value)
{
	u8_t tx_buf[3] = { reg_addr, value >> 8, value & 0xff };

	return i2c_write(dev, tx_buf, 3, dev_addr);
}

static int config(struct device *dev,
		  int access_op, /* unused */
		  u32_t pin,
		  int flags)
{
	const struct sx1509b_config *cfg = dev->config->config_info;
	struct sx1509b_drv_data *drv_data = dev->driver_data;
	struct sx1509b_pin_state *pins = &drv_data->pin_state;

	struct {
		u8_t reg;
		struct sx1509b_pin_state pins;
	} __packed outbuf;
	int rc = 0;
	u16_t bit = BIT(pin);
	bool data_first = false;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Zephyr currently defines drive strength support based on
	 * the behavior and capabilities of the Nordic GPIO
	 * peripheral: strength defaults to low but can be set high,
	 * and is controlled independently for output levels.
	 *
	 * SX150x defaults to high strength, and does not support
	 * different strengths for different levels.
	 *
	 * Until something more general is available reject any
	 * attempt to set a non-default drive strength.
	 *
	 * This code is fragile because it has to assume the flags
	 * we're checking are the non-zero valued ones.  Switching to
	 * mask-and-compare would be more robust, but requires adding
	 * more defines.
	 */
	if ((flags & (GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH)) != 0) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	pins->open_drain &= ~bit;
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
			pins->open_drain |= bit;
		} else {
			/* Open source not supported */
			rc = -ENOTSUP;
			goto out;
		}
	}

	switch (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
	default:
		rc = -EINVAL;
		goto out;
	case 0:
		pins->pull_up &= ~bit;
		pins->pull_down &= ~bit;
		break;
	case GPIO_PULL_UP:
		pins->pull_up |= bit;
		pins->pull_down &= ~bit;
		break;
	case GPIO_PULL_DOWN:
		pins->pull_up &= ~bit;
		pins->pull_down |= bit;
		break;
	}

	if ((flags & GPIO_INPUT) != 0) {
		pins->input_disable &= ~bit;
	} else {
		pins->input_disable |= bit;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		pins->dir &= ~bit;
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			pins->data &= ~bit;
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			pins->data |= bit;
		}
	} else {
		pins->dir |= bit;
	}

	WRITE_BIT(drv_data->common.invert, pin, (flags & GPIO_ACTIVE_LOW) != 0);

	outbuf.reg = SX1509B_REG_INPUT_DISABLE;
	outbuf.pins.input_disable = sys_cpu_to_be16(pins->input_disable);
	outbuf.pins.long_slew = sys_cpu_to_be16(pins->long_slew);
	outbuf.pins.low_drive = sys_cpu_to_be16(pins->low_drive);
	outbuf.pins.pull_up = sys_cpu_to_be16(pins->pull_up);
	outbuf.pins.pull_down = sys_cpu_to_be16(pins->pull_down);
	outbuf.pins.open_drain = sys_cpu_to_be16(pins->open_drain);
	outbuf.pins.polarity = sys_cpu_to_be16(pins->polarity);
	outbuf.pins.dir = sys_cpu_to_be16(pins->dir);
	outbuf.pins.data = sys_cpu_to_be16(pins->data);

	LOG_DBG("CFG %u %x : ID %04x ; PU %04x ; PD %04x ; DIR %04x ; DAT %04x",
		pin, flags,
		pins->input_disable, pins->pull_up, pins->pull_down,
		pins->dir, pins->data);
	if (data_first) {
		rc = i2c_reg_write_word_be(drv_data->i2c_master,
					   cfg->i2c_slave_addr,
					   SX1509B_REG_DATA, pins->data);
		if (rc == 0) {
			rc = i2c_write(drv_data->i2c_master,
				       &outbuf.reg,
				       sizeof(outbuf) - sizeof(pins->data),
				       cfg->i2c_slave_addr);
		}
	} else {
		rc = i2c_write(drv_data->i2c_master,
			       &outbuf.reg,
			       sizeof(outbuf),
			       cfg->i2c_slave_addr);
	}

out:
	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_get(struct device *dev,
		    gpio_port_value_t *value)
{
	const struct sx1509b_config *cfg = dev->config->config_info;
	struct sx1509b_drv_data *drv_data = dev->driver_data;
	u16_t pin_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	u8_t cmd = SX1509B_REG_DATA;

	rc = i2c_write_read(drv_data->i2c_master, cfg->i2c_slave_addr,
			    &cmd, sizeof(cmd),
			    &pin_data, sizeof(pin_data));
	LOG_DBG("read %04x got %d", sys_be16_to_cpu(pin_data), rc);
	if (rc != 0) {
		goto out;
	}

	*value = sys_be16_to_cpu(pin_data);

out:
	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_write_locked(struct device *dev,
			     gpio_port_pins_t mask,
			     gpio_port_value_t value)
{
	const struct sx1509b_config *cfg = dev->config->config_info;
	struct sx1509b_drv_data *drv_data = dev->driver_data;
	u16_t *outp = &drv_data->pin_state.data;
	u16_t out = *outp;
	int rc = 0;

	out = (out & ~mask) | (u16_t)(value & mask);
	rc = i2c_reg_write_word_be(drv_data->i2c_master, cfg->i2c_slave_addr,
				   SX1509B_REG_DATA, out);
	LOG_DBG("write %04x msk %04x val %04x => %04x: %d", *outp, mask, value, out, rc);
	if (rc == 0) {
		*outp = out;
	}

	return rc;
}

static int port_set_masked(struct device *dev,
			   gpio_port_pins_t mask,
			   gpio_port_value_t value)
{
	struct sx1509b_drv_data *drv_data = dev->driver_data;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (mask & ~(gpio_port_pins_t)0xFFFF) {
		return -EINVAL;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	int rc = port_write_locked(dev, mask, value);

	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_set_bits(struct device *dev,
			 gpio_port_pins_t pins)
{
	return port_set_masked(dev, pins, pins);
}

static int port_clear_bits(struct device *dev,
			   gpio_port_pins_t pins)
{
	return port_set_masked(dev, pins, 0);
}

static int port_toggle_bits(struct device *dev,
			    gpio_port_pins_t pins)
{
	struct sx1509b_drv_data *drv_data = dev->driver_data;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pins & ~(gpio_port_pins_t)0xFFFF) {
		return -EINVAL;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	int rc = port_write_locked(dev, pins,
				   drv_data->pin_state.data ^ pins);

	k_sem_give(&drv_data->lock);

	return rc;
}

static int pin_interrupt_configure(struct device *dev,
				   unsigned int pin,
				   unsigned int flags)
{
	return -ENOTSUP;
}

static int sx1509b_write(struct device *dev, int access_op, u32_t pin,
			 u32_t value)
{
	const struct sx1509b_config *cfg = dev->config->config_info;
	struct sx1509b_drv_data *drv_data = dev->driver_data;
	u16_t *pin_data = &drv_data->pin_state.data;
	int ret = 0;

	k_sem_take(&drv_data->lock, K_FOREVER);

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		if (value) {
			*pin_data |= BIT(pin);
		} else {
			*pin_data &= ~BIT(pin);
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		*pin_data = value;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	ret = i2c_reg_write_word_be(drv_data->i2c_master, cfg->i2c_slave_addr,
				    SX1509B_REG_DATA, *pin_data);
out:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int sx1509b_read(struct device *dev, int access_op, u32_t pin,
			u32_t *value)
{
	const struct sx1509b_config *cfg = dev->config->config_info;
	struct sx1509b_drv_data *drv_data = dev->driver_data;
	u16_t pin_data;
	int ret;

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = i2c_burst_read(drv_data->i2c_master, cfg->i2c_slave_addr,
			     SX1509B_REG_DATA, (u8_t *)&pin_data,
			     sizeof(pin_data));
	if (ret) {
		goto out;
	}

	pin_data = sys_be16_to_cpu(pin_data);

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = !!(pin_data & (BIT(pin)));
		break;
	case GPIO_ACCESS_BY_PORT:
		*value = pin_data;
		break;
	default:
		ret = -ENOTSUP;
	}

out:
	k_sem_give(&drv_data->lock);
	return ret;
}

/**
 * @brief Initialization function of SX1509B
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int sx1509b_init(struct device *dev)
{
	const struct sx1509b_config *cfg = dev->config->config_info;
	struct sx1509b_drv_data *drv_data = dev->driver_data;
	int rc;

	drv_data->i2c_master = device_get_binding(cfg->i2c_master_dev_name);
	if (!drv_data->i2c_master) {
		LOG_ERR("%s: no bus %s", dev->config->name,
			cfg->i2c_master_dev_name);
		rc = -EINVAL;
		goto out;
	}

	/* Reset state */
	drv_data->pin_state = (struct sx1509b_pin_state) {
		.dir = (u16_t)-1,
		.data = (u16_t)-1,
	};

	rc = i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				SX1509B_REG_RESET, SX1509B_REG_RESET_MAGIC0);
	if (rc != 0) {
		LOG_ERR("%s: reset m0 failed: %d\n", dev->config->name, rc);
		goto out;
	}
	rc = i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				SX1509B_REG_RESET, SX1509B_REG_RESET_MAGIC1);
	if (rc != 0) {
		goto out;
	}

	k_sleep(K_MSEC(3));

	rc = i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				SX1509B_REG_CLOCK,
				SX1509B_REG_CLOCK_FOSC_INT_2MHZ);
	if (rc != 0) {
		goto out;
	}

out:
	if (rc != 0) {
		LOG_ERR("%s init failed: %d", dev->config->name, rc);
	} else {
		LOG_INF("%s init ok", dev->config->name);
	}
	k_sem_give(&drv_data->lock);
	return rc;
}

static const struct gpio_driver_api api_table = {
	.config = config,
	.write = sx1509b_write,
	.read = sx1509b_read,
	.port_get_raw = port_get,
	.port_set_masked_raw = port_set_masked,
	.port_set_bits_raw = port_set_bits,
	.port_clear_bits_raw = port_clear_bits,
	.port_toggle_bits = port_toggle_bits,
	.pin_interrupt_configure = pin_interrupt_configure,
};

static const struct sx1509b_config sx1509b_cfg = {
	.i2c_master_dev_name = DT_INST_0_SEMTECH_SX1509B_BUS_NAME,
	.i2c_slave_addr = DT_INST_0_SEMTECH_SX1509B_BASE_ADDRESS,
};

static struct sx1509b_drv_data sx1509b_drvdata = {
	.lock = Z_SEM_INITIALIZER(sx1509b_drvdata.lock, 1, 1),
};

DEVICE_AND_API_INIT(sx1509b, DT_INST_0_SEMTECH_SX1509B_LABEL,
		    sx1509b_init, &sx1509b_drvdata, &sx1509b_cfg,
		    POST_KERNEL, CONFIG_GPIO_SX1509B_INIT_PRIORITY,
		    &api_table);
