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

#define MCP23XXX_RESET_TIME_US 2

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
	uint8_t data[2] = {0};
	size_t len = 1;
	int ret;

	if (config->ngpios == 16U) {
		reg *= 2;
		len = 2;
	}

	ret = config->read_fn(dev, reg, data, len);
	if (ret == 0) {
		*buf = sys_get_le16(data);
	}

	return ret;
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
	uint8_t data[2];
	size_t len = 1;

	if (config->ngpios == 16U) {
		reg *= 2;
		len = 2;
	}

	sys_put_le16(value, data);

	return config->write_fn(dev, reg, data, len);
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
 * @brief Read the interrupt status registers in one bus transaction.
 *
 * INTF, INTCAP and GPIO are consecutive registers (with IOCON.BANK = 0), so the
 * whole interrupt state can be captured atomically from the device's point of
 * view. Reading INTCAP clears the pending interrupt, so this must always read
 * INTCAP before GPIO: the datasheet warns that reading GPIO first while another
 * interrupt is pending loses the captured value.
 *
 * @param dev The mcp23xxx device.
 * @param intf Receives INTF.
 * @param intcap Receives INTCAP.
 * @param gpio If not NULL, receives GPIO as well.
 *
 * @return 0 if successful. Otherwise <0 will be returned.
 */
static int read_int_regs(const struct device *dev, uint16_t *intf, uint16_t *intcap, uint16_t *gpio)
{
	const struct mcp23xxx_config *config = dev->config;
	size_t nports = (config->ngpios == 16U) ? 2 : 1;
	size_t nregs = (gpio != NULL) ? 3 : 2;
	uint8_t data[MCP23XXX_MAX_BURST] = {0};
	int ret;

	ret = config->read_fn(dev, REG_INTF * nports, data, nregs * nports);
	if (ret != 0) {
		return ret;
	}

	if (nports == 2) {
		*intf = sys_get_le16(&data[0]);
		*intcap = sys_get_le16(&data[2]);
		if (gpio != NULL) {
			*gpio = sys_get_le16(&data[4]);
		}
	} else {
		*intf = data[0];
		*intcap = data[1];
		if (gpio != NULL) {
			*gpio = data[2];
		}
	}

	return 0;
}

/**
 * @brief Turn raw INTF/INTCAP contents into the set of pins whose interrupt fired.
 *
 * The mcp23xxx only knows "any change" and "differs from DEFVAL" interrupts,
 * so single-edge interrupts are filtered here using the captured pin state.
 * Must be called with the driver lock held.
 */
static uint16_t filter_int_pins(const struct device *dev, uint16_t intf, uint16_t intcap)
{
	struct mcp23xxx_drv_data *drv_data = dev->data;
	uint16_t level_ints = drv_data->reg_cache.gpinten & drv_data->reg_cache.intcon;

	return intf & (level_ints | (intcap & drv_data->rising_edge_ints) |
		       (~intcap & drv_data->falling_edge_ints));
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
	const struct mcp23xxx_config *config = dev->config;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Validate drive mode flags for output pins only.
	 * The MCP23xxx hardware has a fixed drive mode per chip variant:
	 * - MCP23x08/x17: Push-pull outputs only
	 * - MCP23x09/x18: Open-drain outputs only
	 * Input pins don't have a drive mode, so skip validation for them.
	 */
	if (flags & GPIO_OUTPUT) {
		if ((bool)(flags & GPIO_SINGLE_ENDED) != config->is_open_drain ||
		    (bool)(flags & GPIO_LINE_OPEN_DRAIN) != config->is_open_drain) {
			ret = -ENOTSUP;
			goto done;
		}
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
	const struct mcp23xxx_config *config = dev->config;
	uint16_t buf;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	if (config->gpio_int.port && drv_data->reg_cache.gpinten != 0) {
		/* Reading GPIO clears any pending interrupt and discards INTCAP.
		 * Capture both alongside the port value so the event is not lost
		 * if the interrupt handler has not run yet, and let the handler
		 * deliver it.
		 */
		uint16_t intf;
		uint16_t intcap;

		ret = read_int_regs(dev, &intf, &intcap, &buf);
		if (ret == 0 && intf != 0) {
			drv_data->pending_ints |= filter_int_pins(dev, intf, intcap);
			k_work_submit(&drv_data->work);
		}
	} else {
		ret = read_port_regs(dev, REG_GPIO, &buf);
	}

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
		drv_data->pending_ints &= ~BIT(pin);
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
		default:
			ret = -EINVAL;
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
		default:
			ret = -EINVAL;
			goto done;
		}
		break;
	}

	if ((gpinten & ~drv_data->reg_cache.gpinten) != 0) {
		/* Interrupt-on-change compares against the pin value seen at the
		 * last GPIO or INTCAP read, which may be long stale. Refresh it so
		 * that enabling does not fire for a change that predates the
		 * enable, keeping any interrupt the read consumes for the handler.
		 */
		uint16_t intf;
		uint16_t intcap;
		uint16_t gpio;

		ret = read_int_regs(dev, &intf, &intcap, &gpio);
		if (ret != 0) {
			goto done;
		}
		if (intf != 0) {
			drv_data->pending_ints |= filter_int_pins(dev, intf, intcap);
			k_work_submit(&drv_data->work);
		}
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
	const struct mcp23xxx_config *config = dev->config;
	uint16_t intf;
	uint16_t intcap;
	uint16_t pins;
	int ret;

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Reading INTCAP acknowledges the interrupt */
	ret = read_int_regs(dev, &intf, &intcap, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to read INTF/INTCAP (%d)", ret);
		k_sem_give(&drv_data->lock);
		return;
	}

	pins = drv_data->pending_ints | filter_int_pins(dev, intf, intcap);
	drv_data->pending_ints = 0;

	k_sem_give(&drv_data->lock);

	if (pins != 0) {
		gpio_fire_callbacks(&drv_data->callbacks, dev, pins);
	} else {
		/* Not an error: a level interrupt that the callback disabled, or an
		 * edge interrupt filtered out for the other edge, ends up here.
		 */
		LOG_DBG("No interrupt pending");
	}

	/* The host side interrupt is edge triggered, but a DEFVAL compare (level)
	 * interrupt keeps INT asserted for as long as the mismatch exists, so no
	 * new edge is ever produced. Re-run while the line is still active rather
	 * than relying on the brief INT pulse the chip emits when INTCAP is read
	 * during a level interrupt. Submitting instead of looping lets other work
	 * items run in between.
	 */
	if (gpio_pin_get_dt(&config->gpio_int) > 0 ||
	    (config->gpio_intb.port && gpio_pin_get_dt(&config->gpio_intb) > 0)) {
		k_work_submit(work);
	}
}

static void mcp23xxx_int_gpio_handler(const struct device *port, struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	struct mcp23xxx_drv_data *drv_data =
		CONTAINER_OF(cb, struct mcp23xxx_drv_data, int_gpio_cb);

	k_work_submit(&drv_data->work);
}

static void mcp23xxx_intb_gpio_handler(const struct device *port, struct gpio_callback *cb,
				       gpio_port_pins_t pins)
{
	struct mcp23xxx_drv_data *drv_data =
		CONTAINER_OF(cb, struct mcp23xxx_drv_data, intb_gpio_cb);

	k_work_submit(&drv_data->work);
}

/**
 * @brief Connect one INT line to the given callback.
 */
static int setup_int_line(const struct gpio_dt_spec *spec, struct gpio_callback *cb,
			  gpio_callback_handler_t handler)
{
	int err;

	if (!gpio_is_ready_dt(spec)) {
		LOG_ERR("INT port is not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Failed to configure INT line: %d", err);
		return -EIO;
	}

	gpio_init_callback(cb, handler, BIT(spec->pin));
	err = gpio_add_callback(spec->port, cb);
	if (err != 0) {
		LOG_ERR("Failed to add INT callback: %d", err);
		return -EIO;
	}

	err = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		LOG_ERR("Failed to configure INT interrupt: %d", err);
		return -EIO;
	}

	return 0;
}

DEVICE_API(gpio, gpio_mcp23xxx_api_table) = {
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
		uint8_t iocon = 0;

		if (config->gpio_intb.port) {
			if (config->ngpios != 16) {
				LOG_ERR("Only the 16 pin parts have two INT lines");
				return -EINVAL;
			}
			if ((config->gpio_int.dt_flags & GPIO_ACTIVE_LOW) !=
			    (config->gpio_intb.dt_flags & GPIO_ACTIVE_LOW)) {
				LOG_ERR("INTA and INTB must have the same polarity");
				return -EINVAL;
			}
		} else if (config->ngpios == 16) {
			/* send both ports' interrupts through one IRQ pin */
			iocon |= REG_IOCON_MIRROR;
		}

		if (config->is_open_drain) {
			/* The MCP23x09/x18 default to clearing the interrupt on a GPIO
			 * read; this driver acknowledges by reading INTCAP.
			 */
			iocon |= REG_IOCON_INTCC;
		}

		if (config->int_open_drain) {
			iocon |= REG_IOCON_ODR;
		} else if ((config->gpio_int.dt_flags & GPIO_ACTIVE_LOW) == 0) {
			iocon |= REG_IOCON_INTPOL;
		}

		err = write_iocon(dev, iocon);
		if (err != 0) {
			LOG_ERR("Failed to configure IOCON: %d", err);
			return -EIO;
		}

		drv_data->dev = dev;
		k_work_init(&drv_data->work, mcp23xxx_work_handler);

		err = setup_int_line(&config->gpio_int, &drv_data->int_gpio_cb,
				     mcp23xxx_int_gpio_handler);
		if (err != 0) {
			return err;
		}

		if (config->gpio_intb.port) {
			err = setup_int_line(&config->gpio_intb, &drv_data->intb_gpio_cb,
					     mcp23xxx_intb_gpio_handler);
			if (err != 0) {
				return err;
			}
		}
	}

	k_sem_give(&drv_data->lock);

	return 0;
}
