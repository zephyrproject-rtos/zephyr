/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2020 Norbit ODM AS
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca95xx

/**
 * @file Driver for PCA95XX and PCAL95XX I2C-based GPIO driver.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_pca95xx);

#if CONFIG_LITTLE_ENDIAN
#define LOW_BYTE_LE16_IDX	0
#define HIGH_BYTE_LE16_IDX	1
#else
#define LOW_BYTE_LE16_IDX	1
#define HIGH_BYTE_LE16_IDX	0
#endif

/* Register definitions */
#define REG_INPUT_PORT0			0x00
#define REG_INPUT_PORT1			0x01
#define REG_OUTPUT_PORT0		0x02
#define REG_OUTPUT_PORT1		0x03
#define REG_POL_INV_PORT0		0x04
#define REG_POL_INV_PORT1		0x05
#define REG_CONF_PORT0			0x06
#define REG_CONF_PORT1			0x07
#define REG_OUT_DRV_STRENGTH_PORT0_L	0x40
#define REG_OUT_DRV_STRENGTH_PORT0_H	0x41
#define REG_OUT_DRV_STRENGTH_PORT1_L	0x42
#define REG_OUT_DRV_STRENGTH_PORT1_H	0x43
#define REG_INPUT_LATCH_PORT0		0x44
#define REG_INPUT_LATCH_PORT1		0x45
#define REG_PUD_EN_PORT0		0x46
#define REG_PUD_EN_PORT1		0x47
#define REG_PUD_SEL_PORT0		0x48
#define REG_PUD_SEL_PORT1		0x49
#define REG_INT_MASK_PORT0		0x4A
#define REG_INT_MASK_PORT1		0x4B
#define REG_INT_STATUS_PORT0		0x4C
#define REG_INT_STATUS_PORT1		0x4D
#define REG_OUTPUT_PORT_CONF		0x4F

/* Driver flags */
#define PCA_HAS_PUD			BIT(0)
#define PCA_HAS_INTERRUPT		BIT(1)
#define PCA_HAS_INTERRUPT_MASK_REG	BIT(2)

/** Configuration data */
struct gpio_pca95xx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	struct i2c_dt_spec bus;
	uint8_t capabilities;
#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
	struct gpio_dt_spec int_gpio;
#endif
};

/** Runtime driver data */
struct gpio_pca95xx_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	struct {
		uint16_t input;
		uint16_t output;
		uint16_t dir;
		uint16_t pud_en;
		uint16_t pud_sel;
		uint16_t int_mask;
		uint16_t input_latch;
	} reg_cache;

	struct k_sem lock;

#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
	/* Self-reference to the driver instance */
	const struct device *instance;

	/* port ISR callback routine address */
	sys_slist_t callbacks;

	/* interrupt triggering pin masks */
	struct {
		uint16_t edge_rising;
		uint16_t edge_falling;
		uint16_t level_high;
		uint16_t level_low;
	} interrupts;

	struct gpio_callback gpio_callback;

	struct k_work interrupt_worker;

	bool interrupt_active;
#endif
};

static int read_port_reg(const struct device *dev, uint8_t reg, uint8_t pin,
			 uint16_t *cache, uint16_t *buf)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	uint8_t b_buf;
	int ret;

	if (pin >= 8) {
		reg++;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, reg, &b_buf);
	if (ret != 0) {
		LOG_ERR("PCA95XX[0x%X]: error reading register 0x%X (%d)",
			config->bus.addr, reg, ret);
		return ret;
	}

	if (pin < 8) {
		((uint8_t *)cache)[LOW_BYTE_LE16_IDX] = b_buf;
	} else {
		((uint8_t *)cache)[HIGH_BYTE_LE16_IDX] = b_buf;
	}

	*buf = *cache;

	LOG_DBG("PCA95XX[0x%X]: Read: REG[0x%X] = 0x%X",
		config->bus.addr, reg, b_buf);

	return 0;
}

/**
 * @brief Read both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, read the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCA95XX.
 * @param reg Register to read (the PORT0 of the pair of registers).
 * @param cache Pointer to the cache to be updated after successful read.
 * @param buf Buffer to read data into.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_port_regs(const struct device *dev, uint8_t reg,
			  uint16_t *cache, uint16_t *buf)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	uint16_t port_data, value;
	int ret;

	ret = i2c_burst_read_dt(&config->bus, reg, (uint8_t *)&port_data,
				sizeof(port_data));
	if (ret != 0) {
		LOG_ERR("PCA95XX[0x%X]: error reading register 0x%X (%d)",
			config->bus.addr, reg, ret);
		return ret;
	}

	value = sys_le16_to_cpu(port_data);
	*cache = value;
	*buf = value;

	LOG_DBG("PCA95XX[0x%X]: Read: REG[0x%X] = 0x%X, REG[0x%X] = 0x%X",
		config->bus.addr, reg, (*buf & 0xFF), (reg + 1), (*buf >> 8));

	return 0;
}


static int write_port_reg(const struct device *dev, uint8_t reg, uint8_t pin,
			  uint16_t *cache, uint16_t value)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	uint8_t buf[2];
	int ret;

	if (pin < 8) {
		buf[1] = value;
	} else {
		buf[1] = value >> 8;
		reg++;
	}
	buf[0] = reg;

	LOG_DBG("PCA95XX[0x%X]: Write: REG[0x%X] = 0x%X", config->bus.addr,
			reg, buf[1]);

	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret == 0) {
		*cache = value;
	} else {
		LOG_ERR("PCA95XX[0x%X]: error writing to register 0x%X "
			"(%d)", config->bus.addr, reg, ret);
	}

	return ret;
}

/**
 * @brief Write both port 0 and port 1 registers of certain register function.
 *
 * Given the register in reg, write the pair of port 0 and port 1.
 *
 * @param dev Device struct of the PCA95XX.
 * @param reg Register to write into (the PORT0 of the pair of registers).
 * @param cache Pointer to the cache to be updated after successful write.
 * @param value New value to set.
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_port_regs(const struct device *dev, uint8_t reg,
			   uint16_t *cache, uint16_t value)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	uint8_t buf[3];
	int ret;

	LOG_DBG("PCA95XX[0x%X]: Write: REG[0x%X] = 0x%X, REG[0x%X] = "
		"0x%X", config->bus.addr, reg, (value & 0xFF),
		(reg + 1), (value >> 8));

	buf[0] = reg;
	sys_put_le16(value, &buf[1]);

	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret == 0) {
		*cache = value;
	} else {
		LOG_ERR("PCA95XX[0x%X]: error writing to register 0x%X "
			"(%d)", config->bus.addr, reg, ret);
	}

	return ret;
}

static inline int update_input_reg(const struct device *dev, uint8_t pin,
				   uint16_t *buf)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return read_port_reg(dev, REG_INPUT_PORT0, pin,
			     &drv_data->reg_cache.input, buf);
}

static inline int update_input_regs(const struct device *dev, uint16_t *buf)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return read_port_regs(dev, REG_INPUT_PORT0,
			      &drv_data->reg_cache.input, buf);
}

static inline int update_output_reg(const struct device *dev, uint8_t pin,
				    uint16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return write_port_reg(dev, REG_OUTPUT_PORT0, pin,
			      &drv_data->reg_cache.output, value);
}

static inline int update_output_regs(const struct device *dev, uint16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return write_port_regs(dev, REG_OUTPUT_PORT0,
			       &drv_data->reg_cache.output, value);
}

static inline int update_direction_reg(const struct device *dev, uint8_t pin,
					uint16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return write_port_reg(dev, REG_CONF_PORT0, pin,
			      &drv_data->reg_cache.dir, value);
}

static inline int update_pul_sel_reg(const struct device *dev, uint8_t pin,
					uint16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return write_port_reg(dev, REG_PUD_SEL_PORT0, pin,
			      &drv_data->reg_cache.pud_sel, value);
}

static inline int update_pul_en_reg(const struct device *dev, uint8_t pin,
					uint8_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	return write_port_reg(dev, REG_PUD_EN_PORT0, pin,
			      &drv_data->reg_cache.pud_en, value);
}

#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
static inline int update_int_mask_reg(const struct device *dev, uint8_t pin,
					uint16_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	/* If the interrupt mask is present, so is the input latch */
	write_port_reg(dev, REG_INPUT_LATCH_PORT0, pin, &drv_data->reg_cache.input_latch, ~value);

	return write_port_reg(dev, REG_INT_MASK_PORT0, pin,
			      &drv_data->reg_cache.int_mask, value);
}
#endif /* CONFIG_GPIO_PCA95XX_INTERRUPT */

/**
 * @brief Setup the pin direction (input or output)
 *
 * @param dev Device struct of the PCA95XX
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;
	uint16_t reg_dir = drv_data->reg_cache.dir;
	uint16_t reg_out = drv_data->reg_cache.output;
	int ret;

	/* For each pin, 0 == output, 1 == input */
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			reg_out |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			reg_out &= ~BIT(pin);
		}
		ret = update_output_reg(dev, pin, reg_out);
		if (ret != 0) {
			return ret;
		}
		reg_dir &= ~BIT(pin);
	} else {
		reg_dir |= BIT(pin);
	}

	ret = update_direction_reg(dev, pin, reg_dir);

	return ret;
}

/**
 * @brief Setup the pin pull up/pull down status
 *
 * @param dev Device struct of the PCA95XX
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int setup_pin_pullupdown(const struct device *dev, uint32_t pin,
				int flags)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;
	uint16_t reg_pud;
	int ret;

	if ((config->capabilities & PCA_HAS_PUD) == 0) {
		/* Chip does not support pull up/pull down */
		if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
			return -ENOTSUP;
		}

		/* If both GPIO_PULL_UP and GPIO_PULL_DOWN are not set,
		 * we should disable them in hardware. But need to skip
		 * if the chip does not support pull up/pull down.
		 */
		return 0;
	}

	/* If disabling pull up/down, there is no need to set the selection
	 * register. Just go straight to disabling.
	 */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		/* Setup pin pull up or pull down */
		reg_pud = drv_data->reg_cache.pud_sel;

		/* pull down == 0, pull up == 1 */
		WRITE_BIT(reg_pud, pin, (flags & GPIO_PULL_UP) != 0U);

		ret = update_pul_sel_reg(dev, pin, reg_pud);
		if (ret) {
			return ret;
		}
	}

	/* enable/disable pull up/down */
	reg_pud = drv_data->reg_cache.pud_en;

	WRITE_BIT(reg_pud, pin,
		  (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U);

	ret = update_pul_en_reg(dev, pin, reg_pud);

	return ret;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct of the PCA95XX
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_pca95xx_config(const struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	int ret;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

#if (CONFIG_GPIO_LOG_LEVEL >= LOG_LEVEL_DEBUG)
	const struct gpio_pca95xx_config * const config = dev->config;
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
		LOG_ERR("PCA95XX[0x%X]: error setting pin direction (%d)",
			config->bus.addr, ret);
		goto done;
	}

	ret = setup_pin_pullupdown(dev, pin, flags);
	if (ret) {
		LOG_ERR("PCA95XX[0x%X]: error setting pin pull up/down "
			"(%d)", config->bus.addr, ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca95xx_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;
	uint16_t buf;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = update_input_regs(dev, &buf);
	if (ret != 0) {
		goto done;
	}

	*value = buf;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca95xx_port_set_masked_raw(const struct device *dev,
					      uint32_t mask, uint32_t value)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;
	uint16_t reg_out;
	int ret;

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

static int gpio_pca95xx_port_set_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	return gpio_pca95xx_port_set_masked_raw(dev, mask, mask);
}

static int gpio_pca95xx_port_clear_bits_raw(const struct device *dev,
					    uint32_t mask)
{
	return gpio_pca95xx_port_set_masked_raw(dev, mask, 0);
}

static int gpio_pca95xx_port_toggle_bits(const struct device *dev,
					 uint32_t mask)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;
	uint16_t reg_out;
	int ret;

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

#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
static void get_triggered_it(struct gpio_pca95xx_drv_data *drv_data,
		uint16_t *trig_edge, uint16_t *trig_level)
{
	uint16_t input_cache, changed_pins, input_new;
	int ret;

	input_cache = drv_data->reg_cache.input;
	ret = update_input_regs(drv_data->instance, &input_new);
	if (ret == 0) {
		changed_pins = (input_cache ^ input_new);

		*trig_edge |= (changed_pins & input_new &
			     drv_data->interrupts.edge_rising);
		*trig_edge |= (changed_pins & input_cache &
			      drv_data->interrupts.edge_falling);
		*trig_level |= (input_new & drv_data->interrupts.level_high);
		*trig_level |= (~input_new & drv_data->interrupts.level_low);
	}
}

static void gpio_pca95xx_interrupt_worker(struct k_work *work)
{
	struct gpio_pca95xx_drv_data * const drv_data = CONTAINER_OF(
		work, struct gpio_pca95xx_drv_data, interrupt_worker);
	const struct gpio_pca95xx_config * const config = drv_data->instance->config;
	uint16_t trig_edge = 0, trig_level = 0;
	uint32_t triggered_int = 0;

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Note: PCA Interrupt status is cleared by reading inputs */
	if (config->capabilities & PCA_HAS_INTERRUPT_MASK_REG) {
		/* gpio latched read values, to be compared to cached ones */
		get_triggered_it(drv_data, &trig_edge, &trig_level);
	}
	/* gpio unlatched read values, in case signal has flipped again */
	get_triggered_it(drv_data, &trig_edge, &trig_level);

	triggered_int = (trig_edge | trig_level);

	k_sem_give(&drv_data->lock);

	if (triggered_int != 0) {
		gpio_fire_callbacks(&drv_data->callbacks, drv_data->instance,
				    triggered_int);
	}

	/* Emulate level triggering */
	if (trig_level != 0) {
		/* Reschedule worker */
		k_work_submit(&drv_data->interrupt_worker);
	}
}

static void gpio_pca95xx_interrupt_callback(const struct device *dev,
					    struct gpio_callback *cb,
					    gpio_port_pins_t pins)
{
	struct gpio_pca95xx_drv_data * const drv_data =
		CONTAINER_OF(cb, struct gpio_pca95xx_drv_data, gpio_callback);

	ARG_UNUSED(pins);

	/* Cannot read PCA95xx registers from ISR context, queue worker */
	k_work_submit(&drv_data->interrupt_worker);
}

static int gpio_pca95xx_pin_interrupt_configure(const struct device *dev,
						  gpio_pin_t pin,
						  enum gpio_int_mode mode,
						  enum gpio_int_trig trig)
{
	int ret = 0;
	const struct gpio_pca95xx_config * const config = dev->config;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;
	uint16_t reg;
	bool enabled, edge, level, active;

	/* Check if GPIO port supports interrupts */
	if ((config->capabilities & PCA_HAS_INTERRUPT) == 0U) {
		return -ENOTSUP;
	}

	/* Check for an invalid pin number */
	if (BIT(pin) > config->common.port_pin_mask) {
		return -EINVAL;
	}

	/* Check configured pin direction */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    (BIT(pin) & drv_data->reg_cache.dir) != BIT(pin)) {
		LOG_ERR("PCA95XX[0x%X]: output pin cannot trigger interrupt",
			config->bus.addr);
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Check if GPIO port has an interrupt mask register */
	if (config->capabilities & PCA_HAS_INTERRUPT_MASK_REG) {
		uint16_t reg_out;

		reg_out = drv_data->reg_cache.int_mask;
		WRITE_BIT(reg_out, pin, (mode == GPIO_INT_MODE_DISABLED));

		ret = update_int_mask_reg(dev, pin, reg_out);
		if (ret != 0) {
			LOG_ERR("PCA95XX[0x%X]: failed to update int mask (%d)",
				config->bus.addr, ret);
			goto err;
		}
	}

	/* Update interrupt masks */
	enabled = ((mode & GPIO_INT_MODE_DISABLED) == 0U);
	edge = (mode == GPIO_INT_MODE_EDGE);
	level = (mode == GPIO_INT_MODE_LEVEL);
	WRITE_BIT(drv_data->interrupts.edge_rising, pin, (enabled &&
		edge && ((trig & GPIO_INT_TRIG_HIGH) == GPIO_INT_TRIG_HIGH)));
	WRITE_BIT(drv_data->interrupts.edge_falling, pin, (enabled &&
		edge && ((trig & GPIO_INT_TRIG_LOW) == GPIO_INT_TRIG_LOW)));
	WRITE_BIT(drv_data->interrupts.level_high, pin, (enabled &&
		level && ((trig & GPIO_INT_TRIG_HIGH) == GPIO_INT_TRIG_HIGH)));
	WRITE_BIT(drv_data->interrupts.level_low, pin, (enabled &&
		level && ((trig & GPIO_INT_TRIG_LOW) == GPIO_INT_TRIG_LOW)));

	active = ((drv_data->interrupts.edge_rising ||
		   drv_data->interrupts.edge_falling ||
		   drv_data->interrupts.level_high ||
		   drv_data->interrupts.level_low) > 0);

	/* Enable / disable interrupt as needed */
	if (active != drv_data->interrupt_active) {
		ret = gpio_pin_interrupt_configure_dt(
			&config->int_gpio, active ?
				   GPIO_INT_EDGE_TO_ACTIVE :
				   GPIO_INT_MODE_DISABLED);
		if (ret != 0) {
			LOG_ERR("PCA95XX[0x%X]: failed to configure interrupt "
				"on pin %d (%d)", config->bus.addr,
				config->int_gpio.pin, ret);
			goto err;
		}
		drv_data->interrupt_active = active;

		if (active) {
			/* Read current status to reset any
			 * active signal on INT line
			 */
			update_input_regs(dev, &reg);
		}
	}

err:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_pca95xx_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	if ((config->capabilities & PCA_HAS_INTERRUPT) == 0U) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	gpio_manage_callback(&drv_data->callbacks, callback, set);

	k_sem_give(&drv_data->lock);
	return 0;
}
#endif /* CONFIG_GPIO_PCA95XX_INTERRUPT */

static DEVICE_API(gpio, gpio_pca95xx_drv_api_funcs) = {
	.pin_configure = gpio_pca95xx_config,
	.port_get_raw = gpio_pca95xx_port_get_raw,
	.port_set_masked_raw = gpio_pca95xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_pca95xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_pca95xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_pca95xx_port_toggle_bits,
#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
	.pin_interrupt_configure = gpio_pca95xx_pin_interrupt_configure,
	.manage_callback = gpio_pca95xx_manage_callback,
#endif
};

/**
 * @brief Initialization function of PCA95XX
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_pca95xx_init(const struct device *dev)
{
	const struct gpio_pca95xx_config * const config = dev->config;
	struct gpio_pca95xx_drv_data * const drv_data =
		(struct gpio_pca95xx_drv_data * const)dev->data;

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	k_sem_init(&drv_data->lock, 1, 1);

#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
	/* Check if GPIO port supports interrupts */
	if ((config->capabilities & PCA_HAS_INTERRUPT) != 0) {
		int ret;

		/* Store self-reference for interrupt handling */
		drv_data->instance = dev;

		/* Prepare interrupt worker */
		k_work_init(&drv_data->interrupt_worker,
			    gpio_pca95xx_interrupt_worker);

		/* Configure GPIO interrupt pin */
		if (!gpio_is_ready_dt(&config->int_gpio)) {
			LOG_ERR("PCA95XX[0x%X]: interrupt GPIO not ready",
				config->bus.addr);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("PCA95XX[0x%X]: failed to configure interrupt"
				" pin %d (%d)", config->bus.addr,
				config->int_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&drv_data->gpio_callback,
				   gpio_pca95xx_interrupt_callback,
				   BIT(config->int_gpio.pin));
		ret = gpio_add_callback(config->int_gpio.port, &drv_data->gpio_callback);
		if (ret != 0) {
			LOG_ERR("PCA95XX[0x%X]: failed to add interrupt callback for"
				" pin %d (%d)", config->bus.addr,
				config->int_gpio.pin, ret);
			return ret;
		}
	}
#endif

	return 0;
}

#define GPIO_PCA95XX_DEVICE_INSTANCE(inst)				\
static const struct gpio_pca95xx_config gpio_pca95xx_##inst##_cfg = {	\
	.common = {							\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),	\
	},								\
	.bus = I2C_DT_SPEC_INST_GET(inst),				\
	.capabilities =							\
		(DT_INST_PROP(inst, has_pud) ? PCA_HAS_PUD : 0) |	\
		IF_ENABLED(CONFIG_GPIO_PCA95XX_INTERRUPT, (		\
		(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios) ?		\
			PCA_HAS_INTERRUPT : 0) |			\
		(DT_INST_PROP(inst, has_interrupt_mask_reg) ?		\
			PCA_HAS_INTERRUPT_MASK_REG : 0) |		\
		))							\
		0,							\
	IF_ENABLED(CONFIG_GPIO_PCA95XX_INTERRUPT,			\
	(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {}),)) \
};									\
									\
static struct gpio_pca95xx_drv_data gpio_pca95xx_##inst##_drvdata = {	\
	.reg_cache.input = 0x0,						\
	.reg_cache.output = 0xFFFF,					\
	.reg_cache.dir = 0xFFFF,					\
	.reg_cache.pud_en = 0x0,					\
	.reg_cache.pud_sel = 0xFFFF,					\
	.reg_cache.int_mask = 0xFFFF,					\
	.reg_cache.input_latch = 0x0,				\
	IF_ENABLED(CONFIG_GPIO_PCA95XX_INTERRUPT, (			\
	.interrupt_active = false,					\
	))								\
};									\
									\
DEVICE_DT_INST_DEFINE(inst,						\
	gpio_pca95xx_init,						\
	NULL,								\
	&gpio_pca95xx_##inst##_drvdata,					\
	&gpio_pca95xx_##inst##_cfg,					\
	POST_KERNEL, CONFIG_GPIO_PCA95XX_INIT_PRIORITY,			\
	&gpio_pca95xx_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PCA95XX_DEVICE_INSTANCE)
