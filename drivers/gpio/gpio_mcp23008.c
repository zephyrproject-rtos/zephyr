/*
 * Copyright (c) 2020 Ioannis Papamanoglou
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <logging/log.h>

#include "gpio_utils.h"

LOG_MODULE_REGISTER(mcp23008, CONFIG_GPIO_LOG_LEVEL);
#define DT_DRV_COMPAT microchip_mcp23008

/* Number of pins supported by the device */
#define NUM_PINS 8

/* Max to select all pins supported on the device. */
#define ALL_PINS ((u8_t)BIT_MASK(NUM_PINS))

/** Cache of the output configuration and data of the pins */
struct gpio_mcp23008_pin_state {
	u8_t irq_enabled;
	u8_t irq_trigger_level;
	u8_t irq_trigger_edge;
	u8_t pull_up;
	u8_t dir;
	u8_t data;
};

/** Runtime driver data */
struct gpio_mcp23008_drv_data {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_data common;
	struct device *i2c_master;
	struct device *device_struct;
	struct gpio_mcp23008_pin_state pin_state;
	struct k_sem lock;
	struct k_work work;
	struct device *irq_gpio_ctrl;
	struct gpio_callback gpio_cb;
	sys_slist_t cbs;
	struct device *reset_gpio_ctrl;
};

/** Configuration data */
struct gpio_mcp23008_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* I2C configuration */
	const char *i2c_master_dev_name;
	u16_t i2c_slave_addr;
	bool i2c_disslw;
	/* Irq line configuration */
	bool irq_enabled;
	const char *irq_gpio_dev_name;
	gpio_pin_t irq_gpio_pin;
	gpio_flags_t irq_gpio_flags;
	bool int_odr;
	bool int_pol;
	/* Reset line configuration */
	bool reset_enabled;
	const char *reset_gpio_dev_name;
	gpio_pin_t reset_gpio_pin;
	gpio_flags_t reset_gpio_flags;
};

/** Register addresses **/
enum {
	MCP23008_REG_IODIR      = 0x00,
	MCP23008_REG_IPOL       = 0x01,
	MCP23008_REG_GPINTEN    = 0x02,
	MCP23008_REG_DEFVAL     = 0x03,
	MCP23008_REG_INTCON     = 0x04,
	MCP23008_REG_IOCON      = 0x05,
	MCP23008_REG_GPPU       = 0x06,
	MCP23008_REG_INTF       = 0x07,
	MCP23008_REG_INTCAP     = 0x08,
	MCP23008_REG_GPIO       = 0x09,
	MCP23008_REG_OLAT       = 0x0A,
};

/** Register bits **/
enum {
	MCP23008_REG_BIT_IOCON_SEQOP    = BIT(5),
	MCP23008_REG_BIT_IOCON_DISSLW   = BIT(4),
	MCP23008_REG_BIT_IOCON_ODR      = BIT(2),
	MCP23008_REG_BIT_IOCON_INTPOL   = BIT(1),
};

/**
 * @brief Set the port output
 *
 * @param dev Device struct of the MCP23008
 * @param mask Mask indicating which pins will be modified.
 * @param value Value to set (0 or 1)
 * @param toggle Mask indicating which pins will be toggled.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_port_write(struct device *dev,
				    gpio_port_pins_t mask,
				    gpio_port_value_t value,
				    gpio_port_value_t toggle)
{
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	struct gpio_mcp23008_drv_data *const drv_data = dev->driver_data;
	u8_t *pin_data = &drv_data->pin_state.data;
	int rc = 0;
	u8_t pin_data_old, reg_gpio;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		rc = -EWOULDBLOCK;
		goto exitns;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/*
	 * Only write changes to pin_state struct,
	 * if they are successfully written to the device.
	 */
	pin_data_old = *pin_data;
	reg_gpio = (u8_t) (((pin_data_old & ~mask) | (value & mask)) ^ toggle);

	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_GPIO, reg_gpio));
	if (rc) {
		LOG_ERR("Failed writing to i2c");
		goto exit;
	}

	*pin_data = reg_gpio;

exit:
	k_sem_give(&drv_data->lock);
exitns:
	if (rc) {
		LOG_ERR("%s: Could not write to port: %d",
			dev->name, rc);
	} else {
		LOG_INF("%s: Wrote val 0x%02x msk 0x%02x: 0x%02x => 0x%02x",
			dev->name, value, mask,
			pin_data_old, *pin_data);
	}
	return rc;
}

/**
 * @brief Non-ISR part for handling interrupt on IRQ line
 * @param item Pointer to k_work structure
 */
static void gpio_mcp23008_isr_work_handler(struct k_work *item)
{
	struct gpio_mcp23008_drv_data *const drv_data =
		CONTAINER_OF(item, struct gpio_mcp23008_drv_data, work);
	struct device *const dev = drv_data->device_struct;
	const struct gpio_mcp23008_config *cfg = dev->config_info;
	u8_t intf, intcap;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		goto exitns;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	LOG_DBG("Read INTF");
	rc = (i2c_reg_read_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				MCP23008_REG_INTF, &intf));
	if (rc) {
		LOG_ERR("Failed reading INTF");
		goto exit;
	}

	/*
	 * Reset INT line by reading out intcap register
	 * (only edge triggered irqs)
	 */
	LOG_DBG("Read INTCAP");
	rc = (i2c_reg_read_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				MCP23008_REG_INTCAP, &intcap));
	if (rc) {
		LOG_ERR("Failed reading INTCAP");
		goto exit;
	}

	intf &= drv_data->pin_state.irq_enabled;

	k_sem_give(&drv_data->lock);
	gpio_fire_callbacks(&drv_data->cbs, dev, intf);

	/*
	 * Re-enable Interrupt since it was disabled in ISR.
	 * We can assume that the interrupt was configured,
	 * because else the isr could have never been called
	 */
	if (gpio_pin_interrupt_configure(drv_data->irq_gpio_ctrl,
					 cfg->irq_gpio_pin,
					 GPIO_INT_LEVEL_ACTIVE)) {
		LOG_ERR("Could not re-enable irq pin");
	}
	goto exitns;

exit:
	k_sem_give(&drv_data->lock);
exitns:
	if (rc) {
		LOG_ERR("%s: Could not handle workqueue irq: %d",
			dev->name, rc);
	} else {
		LOG_INF("%s: Handled workqueue irq",
			dev->name);
	}
}

/**
 * @brief Callback handler for interrupt on IRQ line
 *
 * @param dev Pointer to device structure for the driver instance.
 * @param cb The callback structure pointer.
 * @param pins Pins that triggered the interrupt.
 */
static void gpio_mcp23008_irq_callback(struct device *dev,
				       struct gpio_callback *cb,
				       gpio_port_pins_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	struct gpio_mcp23008_drv_data *drv_data =
		CONTAINER_OF(cb, struct gpio_mcp23008_drv_data, gpio_cb);
	struct device *const mcp_dev = drv_data->device_struct;
	const struct gpio_mcp23008_config *cfg = mcp_dev->config_info;

	/*
	 * Disable Interrupt until current interrupt is handled,
	 * to avoid hogging the CPU
	 */
	gpio_pin_interrupt_configure(drv_data->irq_gpio_ctrl, cfg->irq_gpio_pin,
				     GPIO_INT_DISABLE);

	/* Defer work to gpio_mcp23008_isr_work_handler */
	k_work_submit(&drv_data->work);
}

/* GPIO API functions */

static int gpio_mcp23008_pin_configure(struct device *dev,
				       gpio_pin_t pin,
				       gpio_flags_t flags)
{
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	struct gpio_mcp23008_drv_data *const drv_data = dev->driver_data;
	struct gpio_mcp23008_pin_state *const pins = &drv_data->pin_state;
	int rc = 0;
	u8_t reg_iodir, reg_gppu, reg_gpio;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		rc = -EWOULDBLOCK;
		goto exitns;
	}

	rc = ((flags & (GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH)) == 0) ?
	     0 : -ENOTSUP;
	if (rc) {
		LOG_ERR("Drive strength not supported");
		goto exitns;
	}

	rc = ((flags & GPIO_SINGLE_ENDED) == 0) ?
	     0 : -ENOTSUP;
	if (rc) {
		LOG_ERR("Open drain/source not supported");
		goto exitns;
	}

	rc = ((flags & GPIO_PULL_DOWN) == 0) ?
	     0 : -ENOTSUP;
	if (rc) {
		LOG_ERR("Pull-down not supported");
		goto exitns;
	}

	rc = ((flags & (GPIO_INPUT | GPIO_OUTPUT)) != GPIO_DISCONNECTED) ?
	     0 : -ENOTSUP;
	if (rc) {
		LOG_ERR("Disconnected pin not supported");
		goto exitns;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/*
	 * Only write changes to pin_state struct,
	 * if they are successfully written to the device.
	 */
	reg_iodir = pins->dir;
	reg_gppu = pins->pull_up;
	reg_gpio = pins->data;

	WRITE_BIT(reg_gppu, pin, (flags & GPIO_PULL_UP) != 0);
	if ((flags & GPIO_OUTPUT) != 0) {
		reg_iodir &= ~BIT(pin);
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			reg_gpio &= ~BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			reg_gpio |= BIT(pin);
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		reg_iodir |= BIT(pin);
	}

	LOG_DBG("CFG %u 0x%x : DIR 0x%02x PU 0x%02x DAT 0x%02x",
		pin, flags,
		reg_iodir, reg_gppu, reg_gpio);

	LOG_DBG("Write GPPU");
	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_GPPU, reg_gppu));
	if (rc) {
		LOG_ERR("Could not write GPPU");
		goto exit;
	}
	pins->pull_up = reg_gppu;

	LOG_DBG("Write GPIO");
	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_GPIO, reg_gpio));
	if (rc) {
		LOG_ERR("Could not write GPIO");
		goto exit;
	}
	pins->data = reg_gpio;

	LOG_DBG("Write IODIR");
	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_IODIR, reg_iodir));
	if (rc) {
		LOG_ERR("Could not write IODIR");
		goto exit;
	}
	pins->dir = reg_iodir;
exit:
	k_sem_give(&drv_data->lock);
exitns:
	if (rc) {
		LOG_ERR("%s: Configure failed %d", dev->name, rc);
	} else {
		LOG_INF("%s: Configure success", dev->name);
	}
	return rc;
}

/** @brief Set physical level of output pins in a port.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Mask indicating which pins will be modified.
 * @param value Value assigned to the output pins.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_port_set_masked_raw(struct device *dev,
					     gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	return gpio_mcp23008_port_write(dev, mask, value, 0);
}

/**
 * @brief Set physical level of selected output pins to high.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_port_set_bits_raw(struct device *dev,
					   gpio_port_pins_t pins)
{
	return gpio_mcp23008_port_write(dev, pins, pins, 0);
}

/**
 * @brief Set physical level of selected output pins to low.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_port_clear_bits_raw(struct device *dev,
					     gpio_port_pins_t pins)
{
	return gpio_mcp23008_port_write(dev, pins, 0, 0);
}

/**
 * @brief Toggle level of selected output pins.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_port_toggle_bits(struct device *dev,
					  gpio_port_pins_t pins)
{
	return gpio_mcp23008_port_write(dev, 0, 0, pins);
}

/**
 * @brief Read the pin or port data
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Value of input pin(s)
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_port_get_raw(struct device *dev,
				      gpio_port_value_t *value)
{
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	struct gpio_mcp23008_drv_data *const drv_data = dev->driver_data;
	u8_t pin_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		rc = -EWOULDBLOCK;
		goto exitns;
	}
	k_sem_take(&drv_data->lock, K_FOREVER);

	rc = (i2c_reg_read_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				MCP23008_REG_GPIO, &pin_data));
	if (rc) {
		LOG_ERR("Could not read from i2c GPIO");
		goto exit;
	}
	*value = pin_data;

exit:
	k_sem_give(&drv_data->lock);
exitns:
	if (rc) {
		LOG_ERR("%s: Could not get pin data from port: %d",
			dev->name, rc);
	} else {
		LOG_INF("%s: Read from port: 0x%x",
			dev->name, pin_data);
	}
	return rc;
}

/**
 * @brief Add/remove an application callback.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @param set Determines whether callback is set or removed
 *
 * @retval 0 If successful, negative errno code on failure.
 * @retval -EINVAL If INT line of device is not configured.
 */
static int gpio_mcp23008_manage_callback(struct device *dev,
					 struct gpio_callback *callback,
					 bool set)
{
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	struct gpio_mcp23008_drv_data *drv_data = dev->driver_data;
	int rc;

	rc = ((cfg->irq_enabled) != false) ? 0 : -EINVAL;
	if (rc) {
		LOG_ERR("Can't setup cb when the INT line isn't configured");
		goto exit;
	}

	rc = (gpio_manage_callback(&drv_data->cbs, callback, set));
	if (rc) {
		LOG_ERR("Adding/Removing to/from callback list failed");
		goto exit;
	}
exit:
	if (rc) {
		LOG_ERR("%s Error %s callback: %d",
			dev->name, set ? "adding" : "removing", rc);
	} else {
		LOG_INF("%s Successfully %s callback",
			dev->name, set ? "added" : "removed");
	}
	return rc;
}

/**
 * @brief Configure pin interrupt.
 *
 * @param dev Pointer to device structure for the driver instance.
 * @param pin Pin number.
 * @param flags Interrupt configuration flags as defined by GPIO_INT_*.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL  Invalid argument.
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_pin_interrupt_configure(struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	struct gpio_mcp23008_drv_data *const drv_data = dev->driver_data;
	int rc = 0;
	u8_t reg_gpinten, reg_defval, reg_intcon;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		rc = -EWOULDBLOCK;
		goto exitns;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/*
	 * Only write changes to pin_state struct,
	 * if they are successfully written to the device.
	 */
	reg_gpinten = drv_data->pin_state.irq_enabled;
	reg_defval = drv_data->pin_state.irq_trigger_level;
	reg_intcon = ~drv_data->pin_state.irq_trigger_edge;

	LOG_DBG("Configure int pin %d (state: EN 0x%02x LVL 0x%02x EDG 0x%02x)",
		pin, reg_gpinten, reg_defval, reg_intcon);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		reg_gpinten &= ~BIT(pin);
		break;
	case GPIO_INT_MODE_LEVEL:
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			reg_defval &= ~BIT(pin);
			break;
		case GPIO_INT_TRIG_HIGH:
			reg_defval |= BIT(pin);
			break;
		default:
			rc = (-ENOTSUP);
			if (rc) {
				LOG_ERR("No support");
				goto exit;
			}
		}
		reg_intcon |= BIT(pin);
		reg_gpinten |= BIT(pin);
		break;
	case GPIO_INT_MODE_EDGE:
		rc = ((trig) == GPIO_INT_TRIG_BOTH) ? 0 : -ENOTSUP;
		if (rc) {
			LOG_ERR("Only triggering on both edges supported");
			goto exit;
		}
		reg_intcon &= ~BIT(pin);
		reg_gpinten |= BIT(pin);
		break;
	default:
		rc = (-ENOTSUP);
		if (rc) {
			LOG_ERR("No support");
			goto exit;
		}
	}

	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_GPINTEN, reg_gpinten));
	if (rc) {
		LOG_ERR("Could not write to i2c GPINTEN");
		goto exit;
	}
	drv_data->pin_state.irq_enabled = reg_gpinten;

	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_DEFVAL, reg_defval));
	if (rc) {
		LOG_ERR("Could not write to i2c DEFVAL");
		goto exit;
	}
	drv_data->pin_state.irq_trigger_level = reg_defval;

	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_INTCON, reg_intcon));
	if (rc) {
		LOG_ERR("Could not write to i2c INTCON");
		goto exit;
	}
	drv_data->pin_state.irq_trigger_edge = ~reg_intcon;

exit:
	k_sem_give(&drv_data->lock);
exitns:
	if (rc) {
		LOG_ERR("%s: Error configuring interrupt on pin %d: %d",
			dev->name, pin, rc);
	} else {
		LOG_INF("%s: Successfully configured interrupt on pin %d",
			dev->name, pin);
		LOG_DBG("%s: pin %d new state: EN 0x%02x LVL 0x%02x EDG 0x%02x",
			dev->name,
			pin,
			drv_data->pin_state.irq_enabled,
			drv_data->pin_state.irq_trigger_level,
			drv_data->pin_state.irq_trigger_edge);
	}
	return rc;
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval status != 0 if at least one gpio interrupt is pending.
 * @retval 0 if no gpio interrupt is pending or an error occurred.
 */
static u32_t gpio_mcp23008_get_pending_int(struct device *dev)
{
	const struct gpio_mcp23008_drv_data *const drv_data = dev->driver_data;
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	int rc = 0;
	u8_t intf = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		rc = -EWOULDBLOCK;
		goto exit;
	}

	rc = (i2c_reg_read_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				MCP23008_REG_INTF, &intf));
	if (rc) {
		LOG_ERR("Failed reading INTF");
		goto exit;
	}

exit:
	if (rc) {
		LOG_ERR("%s: Failed getting pending int: %d",
			dev->name, rc);
	} else {
		LOG_INF("%s: Successfully read pending int: %x",
			dev->name, intf);
	}
	return intf;
}

/* GPIO API functions end*/

/* Device initialization */

static const struct gpio_driver_api gpio_mcp23008_drv_api_funcs = {
	.pin_configure = gpio_mcp23008_pin_configure,
	.port_get_raw = gpio_mcp23008_port_get_raw,
	.port_set_masked_raw = gpio_mcp23008_port_set_masked_raw,
	.port_set_bits_raw = gpio_mcp23008_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mcp23008_port_clear_bits_raw,
	.port_toggle_bits = gpio_mcp23008_port_toggle_bits,
	.pin_interrupt_configure = gpio_mcp23008_pin_interrupt_configure,
	.manage_callback = gpio_mcp23008_manage_callback,
	.get_pending_int = gpio_mcp23008_get_pending_int,
};

/**
 * @brief Initialization function of MCP23008
 *
 * @param dev Device struct
 * @return 0 If successful.
 * @retval -ENOTSUP If any of the configuration options is not supported
 * @retval -EINVAL  Invalid argument (gpio controller of irq and reset line).
 * @retval -EIO I/O error when accessing the external GPIO chip over I2C.
 * @retval -EWOULDBLOCK if operation would block.
 */
static int gpio_mcp23008_init(struct device *dev)
{
	const struct gpio_mcp23008_config *const cfg = dev->config_info;
	struct gpio_mcp23008_drv_data *const drv_data = dev->driver_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		rc = -EWOULDBLOCK;
		goto exitns;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);
	drv_data->device_struct = dev;

	rc = (((drv_data->i2c_master =
			device_get_binding(cfg->i2c_master_dev_name))) != 0) ? 0
			: -EINVAL;
	if (rc) {
		LOG_ERR("%s: not access i2c bus %s",
			dev->name, cfg->i2c_master_dev_name);
		goto exit;
	}

	/* Reset the device over the reset line */
	if (cfg->reset_enabled) {
		rc = ((drv_data->reset_gpio_ctrl =
			       device_get_binding(cfg->reset_gpio_dev_name))
		      != NULL) ? 0 : -EINVAL;
		if (rc) {
			LOG_ERR("Failed to get pointer to RESET device: %s",
				cfg->reset_gpio_dev_name);
			goto exit;
		}

		LOG_DBG("Config reset gpio");
		rc = (gpio_pin_configure(drv_data->reset_gpio_ctrl,
					 cfg->reset_gpio_pin,
					 GPIO_OUTPUT | cfg->reset_gpio_flags));
		if (rc) {
			LOG_ERR("Could not configure RESET gpio %d",
				cfg->reset_gpio_pin);
			goto exit;
		}

		LOG_DBG("Set reset");
		rc = (gpio_pin_set(drv_data->reset_gpio_ctrl,
				   cfg->reset_gpio_pin, 1));
		if (rc) {
			LOG_ERR("Could not set reset");
			goto exit;
		}
		/*
		 * Minimum pulse width for reset signal is 1us
		 * Wait 10us to be sure
		 */
		k_busy_wait(10);
		LOG_DBG("Clear reset");
		rc = (gpio_pin_set(drv_data->reset_gpio_ctrl,
				   cfg->reset_gpio_pin, 0));
		if (rc) {
			LOG_ERR("Could not clear reset");
			goto exit;
		}
		/* Wait for device to be active */
		k_busy_wait(1);
	} else {
		/*
		 * Not sure whether setting all registers to their default state
		 * is enough to gain a clean reset state.
		 */
		rc = (-ENOTSUP);
		if (rc) {
			LOG_ERR("Software reset not supported");
			goto exit;
		}
	}

	/* Set configuration taking the open drain into account */
	LOG_DBG("Write IOCON");
	rc = (i2c_reg_write_byte(drv_data->i2c_master, cfg->i2c_slave_addr,
				 MCP23008_REG_IOCON,
				 MCP23008_REG_BIT_IOCON_SEQOP
				 | (cfg->int_odr ?
				    MCP23008_REG_BIT_IOCON_ODR : 0)
				 | (cfg->int_pol ?
				    MCP23008_REG_BIT_IOCON_INTPOL : 0)
				 | (cfg->i2c_disslw ?
				    MCP23008_REG_BIT_IOCON_DISSLW : 0)));
	if (rc) {
		LOG_ERR("Could not configure IOCON");
		goto exit;
	}

	/* Setup the interrupt line and enable the callback. */
	if (cfg->irq_enabled) {
		k_work_init(&drv_data->work, gpio_mcp23008_isr_work_handler);

		rc = ((drv_data->irq_gpio_ctrl =
			       device_get_binding(cfg->irq_gpio_dev_name))
		      != NULL) ? 0 : -EINVAL;
		if (rc) {
			LOG_ERR("Failed to get pointer to IRQ device: %s",
				cfg->irq_gpio_dev_name);
			goto exit;
		}

		LOG_DBG("Config irq pin");
		rc = (gpio_pin_configure(drv_data->irq_gpio_ctrl,
					 cfg->irq_gpio_pin,
					 GPIO_INPUT | cfg->irq_gpio_flags));
		if (rc) {
			LOG_ERR("Could not configure IRQ gpio %d",
				cfg->irq_gpio_pin);
			goto exit;
		}

		rc = (gpio_pin_interrupt_configure(drv_data->irq_gpio_ctrl,
						   cfg->irq_gpio_pin,
						   GPIO_INT_LEVEL_ACTIVE));
		if (rc) {
			LOG_ERR("Could not configure interrupt on IRQ gpio %d",
				cfg->irq_gpio_pin);
			goto exit;
		}

		gpio_init_callback(&drv_data->gpio_cb,
				   gpio_mcp23008_irq_callback,
				   BIT(cfg->irq_gpio_pin));

		LOG_DBG("Config irq callback");
		rc = (gpio_add_callback(drv_data->irq_gpio_ctrl,
					&drv_data->gpio_cb));
		if (rc) {
			LOG_ERR("Could not add gpio irq callback");
			goto exit;
		}
	}

	drv_data->pin_state = (struct gpio_mcp23008_pin_state) {
		.pull_up = 0x00,
		.dir = ALL_PINS,
		.data = 0x00,
		.irq_enabled = 0x00,
		.irq_trigger_edge = ALL_PINS,
		.irq_trigger_level = 0x00,
	};
exit:
	k_sem_give(&drv_data->lock);
exitns:
	if (rc) {
		LOG_ERR("%s: Init failed: %d", dev->name, rc);
	} else {
		LOG_INF("%s: Init ok", dev->name);
	}
	return rc;
}


#define MCP23008_INIT(inst)						      \
	static struct gpio_mcp23008_drv_data gpio_mcp23008_drvdata_##inst = { \
		.lock = Z_SEM_INITIALIZER(gpio_mcp23008_drvdata_##inst.lock,  \
					  1, 1),			      \
	};								      \
									      \
	static const struct gpio_mcp23008_config gpio_mcp23008_cfg_##inst = { \
		.common = {						      \
			.port_pin_mask =				      \
				GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)	      \
		},							      \
		.i2c_master_dev_name = DT_INST_BUS_LABEL(inst),		      \
		.i2c_slave_addr = DT_INST_REG_ADDR(inst),		      \
		.int_odr = DT_INST_PROP(inst, int_odr),			      \
		.int_pol = DT_INST_PROP(inst, int_pol),			      \
		.i2c_disslw = DT_INST_PROP(inst, i2c_disslw),		      \
		/*
		 * Only check for irq gpio config if irq detection
		 * on irq pin is enabled
		 */							       \
		IF_ENABLED(DT_INST_PROP(inst, irq_enable),		       \
			   (.irq_enabled = true,			       \
			    .irq_gpio_dev_name =			       \
				    DT_INST_GPIO_LABEL(inst, irq_gpios),       \
			    .irq_gpio_pin = DT_INST_GPIO_PIN(inst, irq_gpios), \
			    .irq_gpio_flags =				       \
				    DT_INST_GPIO_FLAGS(inst, irq_gpios),)      \
			   )						       \
		/*
		 * Only check for reset gpio config if reset
		 * on reset pin is enabled
		 */							     \
		IF_ENABLED(DT_INST_PROP(inst, reset_enable),		     \
			   (.reset_enabled = true,			     \
			    .reset_gpio_dev_name =			     \
				    DT_INST_GPIO_LABEL(inst, reset_gpios),   \
			    .reset_gpio_pin =				     \
				    DT_INST_GPIO_PIN(inst, reset_gpios),     \
			    .reset_gpio_flags =				     \
				    DT_INST_GPIO_FLAGS(inst, reset_gpios),)  \
			   )						     \
	};								     \
									     \
	DEVICE_AND_API_INIT(gpio_mcp23008,				     \
			    DT_INST_LABEL(inst),			     \
			    gpio_mcp23008_init,				     \
			    &gpio_mcp23008_drvdata_##inst,		     \
			    &gpio_mcp23008_cfg_##inst,			     \
			    POST_KERNEL, CONFIG_GPIO_MCP23008_INIT_PRIORITY, \
			    &gpio_mcp23008_drv_api_funcs);


DT_INST_FOREACH_STATUS_OKAY(MCP23008_INIT)
