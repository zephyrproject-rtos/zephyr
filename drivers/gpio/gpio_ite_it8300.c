/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ite_it8300, CONFIG_GPIO_LOG_LEVEL);

/* Gather Interrupt Status Register */
#define IT8300_REG_GISR 0x00U

/* Limit GPIO interrupt drain passes per work invocation. */
#define IT8300_MAX_ALERT_PASSES 10U

/* Map ISR register address to its bit position in GISR */
#define IT8300_GISR_BIT_FROM_ISR_ADDR(addr)                                                        \
	COND_CASE_1(IS_EQ(addr, 6U), (0),  /* 0x06 */                                              \
		    IS_EQ(addr, 7U), (1),  /* 0x07 */                                              \
		    IS_EQ(addr, 40U), (2), /* 0x28 */                                              \
		    IS_EQ(addr, 46U), (3), /* 0x2e */                                              \
		    IS_EQ(addr, 47U), (4), /* 0x2f */                                              \
		    (-1))

/* EXGPIO Clear Alert */
#define IT8300_REG_ECA         0x30U
#define IT8300_ECA_CLEAR_ALERT 0x5aU
/* EXGPIO Alert Enable */
#define IT8300_REG_EAE         0x31U
#define IT8300_EAE_AE          BIT(0)

/* Bits [7:6] Port Pin Mode: 10 = GPI, 01 = GPO, others reserved */
#define IT8300_GPCR_MODE_MASK    GENMASK(7, 6)
#define IT8300_GPCR_MODE_GPI     (0x2U << 6)
#define IT8300_GPCR_MODE_GPO     (0x1U << 6)
#define IT8300_GPCR_PULLUP_BIT   BIT(2)
#define IT8300_GPCR_PULLDOWN_BIT BIT(1)

/* Per-GPIO-group configuration and runtime data (child device) */
struct gpio_it8300_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c_dev;
	/* Register addresses belonging to this GPIO group */
	uint8_t reg_pdrg;
	uint8_t reg_isr;
	uint8_t reg_gpcr;
	uint8_t reg_pdmr;
	uint8_t reg_ooder;
	/* Bit corresponding to this group in the global interrupt status */
	uint8_t gisr_bit;
	/* Number of valid GPIO pins in this group */
	uint8_t num_pins;
	bool interrupt_supported;
};

struct gpio_it8300_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	struct k_sem sem;
	uint8_t input_shadow;
	uint8_t int_enabled_mask;
	uint8_t int_rising_mask;
	uint8_t int_falling_mask;
};

/* Shared chip-level configuration and runtime data (parent device) */
struct it8300_config {
	/* GPIO child devices indexed by GPIO group number */
	const struct device *const *gpio_devs;
	struct i2c_dt_spec i2c_dev;
	/* Host GPIO receiving the shared IT8300 alert signal */
	struct gpio_dt_spec alert_gpio;
	size_t num_gpio_devs;
};

struct it8300_data {
	const struct device *dev;
	struct k_work alert_work;
	/* Callback registered on the shared host alert GPIO */
	struct gpio_callback alert_gpio_cb;
	struct k_sem sem;
};

static int it8300_gpio_check_is_not_valid(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_it8300_config *cfg = dev->config;

	if (BIT(pin) & ~(cfg->common.port_pin_mask)) {
		LOG_ERR("%s: Pin %u is not supported", dev->name, pin);
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_it8300_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	gpio_flags_t direction;
	int ret;
	uint8_t pull_val = 0U;

	direction = flags & GPIO_DIR_MASK;
	/* The IT8300 cannot disconnect a pin or use input/output simultaneously */
	if (direction == GPIO_DISCONNECTED || direction == GPIO_DIR_MASK) {
		return -ENOTSUP;
	}

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0U) && ((flags & GPIO_LINE_OPEN_DRAIN) == 0U)) {
		return -ENOTSUP;
	}

	if (it8300_gpio_check_is_not_valid(dev, pin)) {
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Per-pin open-drain enable/disable */
	ret = i2c_reg_update_byte_dt(&cfg->i2c_dev, cfg->reg_ooder, BIT(pin),
				     (flags & GPIO_OPEN_DRAIN) ? BIT(pin) : 0U);
	if (ret < 0) {
		LOG_ERR("%s: Failed to update open-drain setting (ret %d)", dev->name, ret);
		goto done;
	}

	/* Set initial output level before enabling output */
	if ((flags & GPIO_OUTPUT) != 0U &&
	    (flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) != 0U) {
		uint8_t output_value = (flags & GPIO_OUTPUT_INIT_HIGH) != 0U ? BIT(pin) : 0U;

		ret = i2c_reg_update_byte_dt(&cfg->i2c_dev, cfg->reg_pdrg, BIT(pin), output_value);
		if (ret < 0) {
			LOG_ERR("%s: Failed to set output value for pin %d (ret %d)", dev->name,
				pin, ret);
			goto done;
		}
	}

	/* Configure output direction after output-specific settings */
	if ((flags & GPIO_OUTPUT) != 0U) {
		ret = i2c_reg_update_byte_dt(&cfg->i2c_dev, cfg->reg_gpcr + pin,
					     IT8300_GPCR_MODE_MASK, IT8300_GPCR_MODE_GPO);
		if (ret < 0) {
			LOG_ERR("%s: Failed to set output direction for pin %d (ret %d)", dev->name,
				pin, ret);
			goto done;
		}
	}

	/* Configure input direction after all output-specific settings */
	if ((flags & GPIO_INPUT) != 0U) {
		ret = i2c_reg_update_byte_dt(&cfg->i2c_dev, cfg->reg_gpcr + pin,
					     IT8300_GPCR_MODE_MASK, IT8300_GPCR_MODE_GPI);
		if (ret < 0) {
			LOG_ERR("%s: Failed to set input direction for pin %d (ret %d)", dev->name,
				pin, ret);
			goto done;
		}
	}

	/* Pull configuration is always applied last */
	if ((flags & GPIO_PULL_UP) != 0U) {
		pull_val |= IT8300_GPCR_PULLUP_BIT;
	}
	if ((flags & GPIO_PULL_DOWN) != 0U) {
		pull_val |= IT8300_GPCR_PULLDOWN_BIT;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c_dev, cfg->reg_gpcr + pin,
				     IT8300_GPCR_PULLUP_BIT | IT8300_GPCR_PULLDOWN_BIT, pull_val);
	if (ret < 0) {
		LOG_ERR("%s: Failed to update pull setting for pin %d (ret %d)", dev->name, pin,
			ret);
	}

done:
	k_sem_give(&data->sem);
	return ret;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_it8300_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	gpio_flags_t flags = 0U;
	int ret;
	uint8_t reg_gpcr = cfg->reg_gpcr + pin;
	uint8_t control, open_drain, value;

	if (it8300_gpio_check_is_not_valid(dev, pin)) {
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, reg_gpcr, &control);
	if (ret) {
		LOG_ERR("%s: Failed to read control value (ret %d)", dev->name, ret);
		goto done;
	}

	/* Get GPIO direction */
	if (control & IT8300_GPCR_MODE_GPO) {
		flags |= GPIO_OUTPUT;

		ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, cfg->reg_ooder, &open_drain);
		if (ret) {
			LOG_ERR("%s: Failed to read ooder value (ret %d)", dev->name, ret);
			goto done;
		}
		/* Get GPIO type, 0:push-pull 1:open-drain */
		if (open_drain & BIT(pin)) {
			flags |= GPIO_OPEN_DRAIN;
		}

		ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, cfg->reg_pdrg, &value);
		if (ret) {
			LOG_ERR("%s: Failed to read pin status (ret %d)", dev->name, ret);
			goto done;
		}

		/* Get GPIO output level */
		if (value & BIT(pin)) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	} else {
		flags |= GPIO_INPUT;
	}

	/* pullup / pulldown */
	if (control & IT8300_GPCR_PULLUP_BIT) {
		flags |= GPIO_PULL_UP;
	} else if (control & IT8300_GPCR_PULLDOWN_BIT) {
		flags |= GPIO_PULL_DOWN;
	}

	*out_flags = flags;

done:
	k_sem_give(&data->sem);
	return ret;
}
#endif

static int gpio_it8300_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	int ret;
	uint8_t val;

	k_sem_take(&data->sem, K_FOREVER);

	/* Get raw bits of GPIO mirror register */
	ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, cfg->reg_pdmr, &val);
	if (ret < 0) {
		LOG_ERR("%s: Failed to get port mask (ret %d)", dev->name, ret);
		goto done;
	}

	*value = val & cfg->common.port_pin_mask;

done:
	k_sem_give(&data->sem);

	return ret;
}

static int gpio_it8300_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	ret = i2c_reg_update_byte_dt(&cfg->i2c_dev, cfg->reg_pdrg, mask, value);
	if (ret) {
		LOG_ERR("%s: Failed to update bits raw (ret %d)", dev->name, ret);
	}

	k_sem_give(&data->sem);

	return ret;
}

static int gpio_it8300_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_it8300_port_set_masked_raw(dev, pins, pins);
}

static int gpio_it8300_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_it8300_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_it8300_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	int ret;
	uint8_t val, new_val;

	k_sem_take(&data->sem, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, cfg->reg_pdrg, &val);
	if (ret) {
		LOG_ERR("%s: Failed to read data value (ret %d)", dev->name, ret);
		goto done;
	}

	/* Toggle raw bits of GPIO data register */
	new_val = val ^ pins;
	if (new_val == val) {
		goto done;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c_dev, cfg->reg_pdrg, new_val);
	if (ret) {
		LOG_ERR("%s: Failed to write toggle value (ret %d)", dev->name, ret);
	}

done:
	k_sem_give(&data->sem);

	return ret;
}

static int gpio_it8300_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	int ret;
	uint8_t mask = BIT(pin);
	uint8_t pin_state;

	if (it8300_gpio_check_is_not_valid(dev, pin)) {
		return -ENOTSUP;
	}

	if (!cfg->interrupt_supported) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		k_sem_take(&data->sem, K_FOREVER);
		data->int_enabled_mask &= ~mask;
		data->int_rising_mask &= ~mask;
		data->int_falling_mask &= ~mask;
		k_sem_give(&data->sem);
		return 0;
	}

	/*
	 * The chip only latches "pin changed" and has no per-pin edge/level
	 * selection. Edge polarity is filtered in the alert work handler by
	 * comparing the current pin level with the saved level.
	 */
	if (mode != GPIO_INT_MODE_EDGE ||
	    (trig != GPIO_INT_TRIG_LOW && trig != GPIO_INT_TRIG_HIGH &&
	     trig != GPIO_INT_TRIG_BOTH)) {
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Save the initial level used to determine the direction of the next edge. */
	ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, cfg->reg_pdmr, &pin_state);
	if (ret) {
		LOG_ERR("%s: Failed to read initial pin state (ret %d)", dev->name, ret);
		goto done;
	}
	data->input_shadow = (data->input_shadow & ~mask) | (pin_state & mask);

	/* Clear pending interrupt */
	ret = i2c_reg_write_byte_dt(&cfg->i2c_dev, cfg->reg_isr, mask);
	if (ret) {
		LOG_ERR("%s: Failed to clear pending interrupt (ret %d)", dev->name, ret);
		goto done;
	}

	if ((trig & GPIO_INT_TRIG_HIGH) != 0U) {
		data->int_rising_mask |= mask;
	} else {
		data->int_rising_mask &= ~mask;
	}
	if ((trig & GPIO_INT_TRIG_LOW) != 0U) {
		data->int_falling_mask |= mask;
	} else {
		data->int_falling_mask &= ~mask;
	}
	data->int_enabled_mask |= mask;

	/* Enable the alert function */
	ret = i2c_reg_write_byte_dt(&cfg->i2c_dev, IT8300_REG_EAE, IT8300_EAE_AE);
	if (ret < 0) {
		LOG_ERR("%s: failed to enable alert (%d)", dev->name, ret);
		data->int_enabled_mask &= ~mask;
		data->int_rising_mask &= ~mask;
		data->int_falling_mask &= ~mask;
	}

done:
	k_sem_give(&data->sem);

	return ret;
}

static int gpio_it8300_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_it8300_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_it8300_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;
	gpio_port_pins_t in_mask = 0U;
	gpio_port_pins_t out_mask = 0U;
	int ret = 0;

	/* If caller doesn't need any result, return early */
	if (!inputs && !outputs) {
		return 0;
	}

	/* Mask out pins not supported by this port */
	map &= cfg->common.port_pin_mask;

	k_sem_take(&data->sem, K_FOREVER);

	for (gpio_pin_t pin = 0U; pin < cfg->num_pins; pin++) {
		uint8_t gpcr;

		/* Skip pins not requested */
		if (!(map & BIT(pin))) {
			continue;
		}

		/* Read this pin's GPCR to check its mode (bit[7:6]: GPI/GPO) */
		ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, cfg->reg_gpcr + pin, &gpcr);
		if (ret < 0) {
			LOG_ERR("%s: Failed to read GPCR for pin %d (ret %d)", dev->name, pin, ret);
			goto done;
		}

		/* Only GPI(0b10)/GPO(0b01) are valid */
		if ((gpcr & IT8300_GPCR_MODE_MASK) == IT8300_GPCR_MODE_GPO) {
			out_mask |= BIT(pin);
		} else if ((gpcr & IT8300_GPCR_MODE_MASK) == IT8300_GPCR_MODE_GPI) {
			in_mask |= BIT(pin);
		}
	}

	if (inputs) {
		*inputs = in_mask;
	}
	if (outputs) {
		*outputs = out_mask;
	}

done:
	k_sem_give(&data->sem);

	return ret;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static int gpio_it8300_init(const struct device *dev)
{
	const struct gpio_it8300_config *cfg = dev->config;
	struct gpio_it8300_data *data = dev->data;

	if (!i2c_is_ready_dt(&cfg->i2c_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c_dev.bus);
		return -ENODEV;
	}

	k_sem_init(&data->sem, 1, 1);
	sys_slist_init(&data->callbacks);

	return 0;
}

/* Process the latched interrupt status of one GPIO child. */
static int it8300_gpio_process_interrupt(const struct device *dev, const struct device *gpio_dev)
{
	const struct gpio_it8300_config *gpio_cfg = gpio_dev->config;
	struct gpio_it8300_data *gpio_data = gpio_dev->data;
	uint8_t raw_isr;
	uint8_t isr_val = 0U;
	uint8_t pin_state;
	uint8_t rising;
	uint8_t falling;
	uint8_t both_edges;
	uint8_t fire_pins = 0U;
	int ret;

	k_sem_take(&gpio_data->sem, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&gpio_cfg->i2c_dev, gpio_cfg->reg_isr, &raw_isr);
	if (ret < 0) {
		LOG_ERR("%s: Failed to read ISR 0x%02x (%d)", dev->name, gpio_cfg->reg_isr, ret);
		goto unlock;
	}

	isr_val = raw_isr & gpio_cfg->common.port_pin_mask;
	if (isr_val == 0U) {
		goto clear_isr;
	}

	ret = i2c_reg_read_byte_dt(&gpio_cfg->i2c_dev, gpio_cfg->reg_pdmr, &pin_state);
	if (ret < 0) {
		LOG_ERR("%s: Failed to read pin state (%d)", gpio_dev->name, ret);
		goto unlock;
	}

	pin_state &= gpio_cfg->common.port_pin_mask;

	/*
	 * Determine the edge direction by comparing the saved level with the
	 * current level. For pins configured for both edges, report any
	 * latched change directly because multiple transitions may leave the
	 * final level equal to the saved level. Finally, filter out disabled
	 * pins.
	 */
	rising = (~gpio_data->input_shadow & pin_state) & isr_val;
	falling = (gpio_data->input_shadow & ~pin_state) & isr_val;
	both_edges = gpio_data->int_rising_mask & gpio_data->int_falling_mask;

	gpio_data->input_shadow = pin_state;

	fire_pins = ((isr_val & both_edges) | (rising & gpio_data->int_rising_mask & ~both_edges) |
		     (falling & gpio_data->int_falling_mask & ~both_edges)) &
		    gpio_data->int_enabled_mask;

clear_isr:
	/* Write-1-to-clear the latched interrupt status bits. */
	ret = i2c_reg_write_byte_dt(&gpio_cfg->i2c_dev, gpio_cfg->reg_isr, raw_isr);
	if (ret < 0) {
		LOG_ERR("%s: failed to clear ISR 0x%02x (%d)", gpio_dev->name, gpio_cfg->reg_isr,
			ret);
	}

unlock:
	k_sem_give(&gpio_data->sem);

	if (ret < 0) {
		return ret;
	}

	LOG_DBG("%s: ISR=0x%02x enabled=0x%02x fire=0x%02x", gpio_dev->name, isr_val,
		gpio_data->int_enabled_mask, fire_pins);

	if (fire_pins != 0U) {
		gpio_fire_callbacks(&gpio_data->callbacks, gpio_dev, fire_pins);
	}

	return 0;
}

static void it8300_alert_work_handler(struct k_work *work)
{
	struct it8300_data *data = CONTAINER_OF(work, struct it8300_data, alert_work);
	const struct device *dev = data->dev;
	const struct it8300_config *cfg = dev->config;
	int ret;
	uint8_t gisr;

	k_sem_take(&data->sem, K_FOREVER);

	/*
	 * Recheck GISR to handle interrupts latched during callbacks. Limit the
	 * number of passes before releasing ALERT# so persistent pending status
	 * cannot keep the drain loop running indefinitely.
	 */
	for (uint8_t pass = 0U; pass < IT8300_MAX_ALERT_PASSES; pass++) {
		ret = i2c_reg_read_byte_dt(&cfg->i2c_dev, IT8300_REG_GISR, &gisr);
		if (ret < 0) {
			LOG_ERR("%s: Failed to read GISR (%d)", dev->name, ret);
			goto release_alert;
		}

		if (gisr == 0U) {
			break;
		}

		/* Check each GPIO child for a pending interrupt. */
		for (size_t i = 0; i < cfg->num_gpio_devs; i++) {
			const struct device *gpio_dev = cfg->gpio_devs[i];
			const struct gpio_it8300_config *gpio_cfg;

			if (!device_is_ready(gpio_dev)) {
				LOG_ERR_DEVICE_NOT_READY(gpio_dev);
				goto release_alert;
			}

			gpio_cfg = gpio_dev->config;

			if (!(gisr & BIT(gpio_cfg->gisr_bit))) {
				continue;
			}

			ret = it8300_gpio_process_interrupt(dev, gpio_dev);
			if (ret < 0) {
				goto release_alert;
			}
		}
	}

	/*
	 * NOTE: Writing ECA clears all interrupt status bits. If an interrupt
	 * becomes pending here, the ECA write may inadvertently clear it before
	 * it is handled, causing a race condition. This is a hardware limitation.
	 */
release_alert:
	/* Release the ALERT# line */
	ret = i2c_reg_write_byte_dt(&cfg->i2c_dev, IT8300_REG_ECA, IT8300_ECA_CLEAR_ALERT);
	if (ret < 0) {
		LOG_ERR("%s: failed to release alert (%d)", dev->name, ret);
	}

	k_sem_give(&data->sem);
}

static void it8300_alert_gpio_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	struct it8300_data *data = CONTAINER_OF(cb, struct it8300_data, alert_gpio_cb);

	k_work_submit(&data->alert_work);
}

static int it8300_init(const struct device *dev)
{
	const struct it8300_config *cfg = dev->config;
	struct it8300_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c_dev.bus);
		return -ENODEV;
	}

	/* Save dev pointer for later use in the work handler */
	data->dev = dev;

	/* Initialize mutex for GPIO controller */
	k_sem_init(&data->sem, 1, 1);

	/* Initialize work structures */
	k_work_init(&data->alert_work, it8300_alert_work_handler);

	/*
	 * ALERT# is optional. Without it, normal polled GPIO operation
	 * is still available, but pin interrupts return -ENOTSUP.
	 */
	if (cfg->alert_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->alert_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->alert_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("%s: failed to configure ALERT# input(%d)", dev->name, ret);
		return ret;
	}

	gpio_init_callback(&data->alert_gpio_cb, it8300_alert_gpio_isr, BIT(cfg->alert_gpio.pin));

	ret = gpio_add_callback(cfg->alert_gpio.port, &data->alert_gpio_cb);
	if (ret < 0) {
		LOG_ERR("%s: failed to add ALERT# callback (%d)", dev->name, ret);
		return ret;
	}

	/* Clear an old ALERT# latch before enabling the host interrupt.*/
	ret = i2c_reg_write_byte_dt(&cfg->i2c_dev, IT8300_REG_ECA, IT8300_ECA_CLEAR_ALERT);
	if (ret < 0) {
		LOG_ERR("%s: failed to clear ALERT# (%d)", dev->name, ret);
		goto remove_callback;
	}

	/*
	 * ALERT# is physically active-low, so trigger when it falls.
	 * GPIO_INT_EDGE_FALLING avoids depending on the DT logical polarity.
	 */
	ret = gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("%s: failed to configure ALERT# interrupt (%d)", dev->name, ret);
		goto remove_callback;
	}

	return 0;

remove_callback:
	gpio_remove_callback(cfg->alert_gpio.port, &data->alert_gpio_cb);

	return ret;
}

static DEVICE_API(gpio, gpio_it8300_driver_api) = {
	.pin_configure = gpio_it8300_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_it8300_get_config,
#endif
	.port_get_raw = gpio_it8300_port_get_raw,
	.port_set_masked_raw = gpio_it8300_port_set_masked_raw,
	.port_set_bits_raw = gpio_it8300_port_set_bits_raw,
	.port_clear_bits_raw = gpio_it8300_port_clear_bits_raw,
	.port_toggle_bits = gpio_it8300_port_toggle_bits,
	.pin_interrupt_configure = gpio_it8300_pin_interrupt_configure,
	.manage_callback = gpio_it8300_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_it8300_port_get_direction,
#endif
};

/* Child ite,it8300-gpio devices */
#define DT_DRV_COMPAT ite_it8300_gpio

#define GPIO_IT8300_DEVICE_INST(inst)                                                              \
	static struct gpio_it8300_data gpio_it8300_data_##inst;                                    \
	static const struct gpio_it8300_config gpio_it8300_config_##inst = {                       \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.i2c_dev = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                  \
		.reg_pdrg = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                      \
		.reg_isr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                       \
		.reg_gpcr = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                      \
		.reg_pdmr = DT_INST_REG_ADDR_BY_IDX(inst, 3),                                      \
		.reg_ooder = DT_INST_REG_ADDR_BY_IDX(inst, 4),                                     \
		.gisr_bit = IT8300_GISR_BIT_FROM_ISR_ADDR(DT_INST_REG_ADDR_BY_IDX(inst, 1)),       \
		.num_pins = DT_INST_PROP(inst, ngpios),                                            \
		.interrupt_supported = DT_NODE_HAS_PROP(DT_INST_PARENT(inst), alert_gpios),        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_it8300_init, NULL, &gpio_it8300_data_##inst,              \
			      &gpio_it8300_config_##inst, POST_KERNEL,                             \
			      CONFIG_IT8300_INIT_PRIORITY, &gpio_it8300_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_IT8300_DEVICE_INST)

#undef DT_DRV_COMPAT

/* Parent ite,it8300 device */
#define DT_DRV_COMPAT ite_it8300

#define IT8300_CHILD_DEVICE(node_id) DEVICE_DT_GET(node_id),

#define IT8300_PARENT_DEVICE_INST(inst)                                                            \
	static const struct device *const it8300_gpio_devs_##inst[] = {                            \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), IT8300_CHILD_DEVICE)};             \
	static struct it8300_data it8300_data_##inst;                                              \
	static const struct it8300_config it8300_config_##inst = {                                 \
		.i2c_dev = I2C_DT_SPEC_GET(DT_DRV_INST(inst)),                                     \
		.alert_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), alert_gpios, {0}),            \
		.gpio_devs = it8300_gpio_devs_##inst,                                              \
		.num_gpio_devs = ARRAY_SIZE(it8300_gpio_devs_##inst),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, it8300_init, NULL, &it8300_data_##inst, &it8300_config_##inst, \
			      POST_KERNEL, CONFIG_IT8300_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IT8300_PARENT_DEVICE_INST)
