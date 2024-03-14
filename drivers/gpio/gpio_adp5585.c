/*
 * Copyright 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/mfd/adp5585.h>

#define DT_DRV_COMPAT adi_adp5585_gpio

LOG_MODULE_REGISTER(adp5585_gpio, CONFIG_GPIO_LOG_LEVEL);

#define ADP5585_BANK(offs) (offs >> 3)
#define ADP5585_BIT(offs)  (offs & 0b0111)

/* Number of pins supported by the device */
#define NUM_PINS (10U)

/* Max to select all pins supported on the device. */
#define ALL_PINS (uint16_t)((BIT_MASK(5U) << 8U) | (BIT_MASK(5U)))

struct gpio_pin_gaps {
	uint8_t start;
	uint8_t len;
};

enum adp5585_gpio_pin_direction {
	adp5585_pin_input = 0U,
	adp5585_pin_output,
};

enum adp5585_gpio_pin_drive_mode {
	adp5585_pin_drive_pp = 0U,
	adp5585_pin_drive_od,
};

enum adp5585_gpio_pull_config {
	adp5585_pull_up_300k = 0U,
	adp5585_pull_dn_300k,
	adp5585_pull_up_100k, /* not used */
	adp5585_pull_disable,
};

enum adp5585_gpio_int_en {
	adp5585_int_disable = 0U,
	adp5585_int_enable,
};

enum adp5585_gpio_int_level {
	adp5585_int_active_low = 0U,
	adp5585_int_active_high,
};

/** Configuration data */
struct adp5585_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *mfd_dev;
	const struct gpio_dt_spec gpio_int;
	const struct gpio_pin_gaps *pin_gaps;
	const uint8_t gap_count;
};

/** Runtime driver data */
struct adp5585_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	uint16_t output;

	sys_slist_t callbacks;
};

static int gpio_adp5585_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct adp5585_gpio_config *cfg = dev->config;
	struct adp5585_gpio_data *data = dev->data;
	const struct mfd_adp5585_config *parent_cfg =
		(struct mfd_adp5585_config *)(cfg->mfd_dev->config);
	struct mfd_adp5585_data *parent_data = (struct mfd_adp5585_data *)(cfg->mfd_dev->data);

	int ret = 0;
	uint8_t reg_value;

	/* ADP5585 has non-contiguous gpio pin layouts, account for this */
	for (int i = 0; i < cfg->gap_count; i++) {
		if ((pin >= cfg->pin_gaps[i].start) &&
		    (pin < (cfg->pin_gaps[i].start + cfg->pin_gaps[i].len))) {
			/* Pin is reserved unavailable */
			LOG_ERR("pin %d is invalid for this device", pin);
			return -ENOTSUP;
		}
	}

	uint8_t bank = ADP5585_BANK(pin);
	uint8_t bank_pin = ADP5585_BIT(pin);

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Single Ended lines (Open drain and open source) not supported */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Simultaneous PU & PD mode not supported */
	if (((flags & GPIO_PULL_UP) != 0) && ((flags & GPIO_PULL_DOWN) != 0)) {
		return -ENOTSUP;
	}

	/* Simultaneous input & output mode not supported */
	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	k_sem_take(&parent_data->lock, K_FOREVER);

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		reg_value = adp5585_pin_drive_od << bank_pin;
	} else {
		reg_value = adp5585_pin_drive_pp << bank_pin;
	}
	ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus, ADP5585_GPO_OUT_MODE_A + bank,
				     (0b1U << bank_pin), reg_value);
	if (ret != 0) {
		goto out;
	}

	{
		uint8_t regaddr = ADP5585_RPULL_CONFIG_A + (bank << 1);
		uint8_t shift = bank_pin << 1;

		if (bank_pin > 3U) {
			regaddr += 1U;
			shift = (bank_pin - 3U) << 1;
		}
		if ((flags & GPIO_PULL_UP) != 0) {
			reg_value = adp5585_pull_up_300k;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			reg_value = adp5585_pull_dn_300k;
		} else {
			reg_value = adp5585_pull_disable;
		}

		ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus, regaddr, 0b11U << shift,
					     (reg_value << shift));
		if (ret != 0) {
			goto out;
		}
	}

	/* Ensure either Output or Input is specified */
	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			data->output &= ~BIT(pin);
			if (ret == 0) {
				ret = i2c_burst_write_dt(&parent_cfg->i2c_bus,
							 ADP5585_GPO_DATA_OUT_A,
							 (uint8_t *)&data->output, 2U);
			}
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			data->output |= BIT(pin);
			if (ret == 0) {
				ret = i2c_burst_write_dt(&parent_cfg->i2c_bus,
							 ADP5585_GPO_DATA_OUT_A,
							 (uint8_t *)&data->output, 2U);
			}
		}
		if (ret == 0) {
			ret = i2c_reg_update_byte_dt(
				&parent_cfg->i2c_bus, ADP5585_GPIO_DIRECTION_A + bank,
				(1U << bank_pin), (adp5585_pin_output << bank_pin));
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		if (ret == 0) {
			ret = i2c_reg_update_byte_dt(
				&parent_cfg->i2c_bus, ADP5585_GPIO_DIRECTION_A + bank,
				(1U << bank_pin), (adp5585_pin_input << bank_pin));
		}
	} else {
		ret = -ENOTSUP;
		goto out;
	}

	if (ret == 0) {
		ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus, ADP5585_GPIO_DIRECTION_A + bank,
					     (1U << bank_pin), reg_value);
	}

out:
	k_sem_give(&parent_data->lock);
	if (ret != 0) {
		LOG_ERR("pin configure error: %d", ret);
	}
	return ret;
}

static int gpio_adp5585_port_read(const struct device *dev, gpio_port_value_t *value)
{
	const struct adp5585_gpio_config *cfg = dev->config;
	/* struct adp5585_gpio_data *data = dev->data; */
	const struct mfd_adp5585_config *parent_cfg =
		(struct mfd_adp5585_config *)(cfg->mfd_dev->config);
	struct mfd_adp5585_data *parent_data = (struct mfd_adp5585_data *)(cfg->mfd_dev->data);

	uint16_t input_data;
	int ret = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&parent_data->lock, K_FOREVER);

	/* Read Input Register */
	ret = i2c_burst_read_dt(&parent_cfg->i2c_bus, ADP5585_GPI_STATUS_A, (uint8_t *)&input_data,
				2U);
	LOG_WRN("read %x got %d", input_data, ret);
	if (ret == 0) {
		*value = input_data;
	}

	k_sem_give(&parent_data->lock);
	return ret;
}

static int gpio_adp5585_port_write(const struct device *dev, gpio_port_pins_t mask,
				   gpio_port_value_t value, gpio_port_value_t toggle)
{
	const struct adp5585_gpio_config *cfg = dev->config;
	struct adp5585_gpio_data *data = dev->data;
	const struct mfd_adp5585_config *parent_cfg =
		(struct mfd_adp5585_config *)(cfg->mfd_dev->config);
	struct mfd_adp5585_data *parent_data = (struct mfd_adp5585_data *)(cfg->mfd_dev->data);

	uint16_t orig_out;
	uint16_t out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&parent_data->lock, K_FOREVER);

	orig_out = data->output;
	out = ((orig_out & ~mask) | (value & mask)) ^ toggle;

	ret = i2c_burst_write_dt(&parent_cfg->i2c_bus, ADP5585_GPO_DATA_OUT_A, (uint8_t *)&out, 2U);

	if (ret == 0) {
		data->output = out;
	}

	k_sem_give(&parent_data->lock);

	LOG_DBG("write %x msk %08x val %08x => %x: %d", orig_out, mask, value, out, ret);

	return ret;
}

static int gpio_adp5585_port_set_masked(const struct device *dev, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	return gpio_adp5585_port_write(dev, mask, value, 0);
}

static int gpio_adp5585_port_set_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_adp5585_port_write(dev, pins, pins, 0);
}

static int gpio_adp5585_port_clear_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_adp5585_port_write(dev, pins, 0, 0);
}

static int gpio_adp5585_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_adp5585_port_write(dev, 0, 0, pins);
}

static int gpio_adp5585_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct adp5585_gpio_config *cfg = dev->config;
	/* struct adp5585_gpio_data *data = dev->data; */
	const struct mfd_adp5585_config *parent_cfg =
		(struct mfd_adp5585_config *)(cfg->mfd_dev->config);
	struct mfd_adp5585_data *parent_data = (struct mfd_adp5585_data *)(cfg->mfd_dev->data);
	int ret = 0;

	if (parent_cfg->nint_gpio.port == NULL) {
		return -ENOTSUP;
	}

	/* ADP5585 has non-contiguous gpio pin layouts, account for this */
	for (int i = 0; i < cfg->gap_count; i++) {
		if ((pin >= cfg->pin_gaps[i].start) &&
		    (pin < (cfg->pin_gaps[i].start + cfg->pin_gaps[i].len))) {
			/* Pin is reserved unavailable */
			LOG_ERR("pin %d is invalid for this device", pin);
			return -ENOTSUP;
		}
	}

	/* This device supports only level-triggered interrupts. */
	/* This device does NOT support either-level interrupt. */
	if (mode == GPIO_INT_MODE_EDGE || trig == GPIO_INT_TRIG_BOTH) {
		return -ENOTSUP;
	}
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	uint8_t bank = ADP5585_BANK(pin);
	uint8_t bank_pin = ADP5585_BIT(pin);

	k_sem_take(&parent_data->lock, K_FOREVER);

	if (mode == GPIO_INT_MODE_DISABLED) {
		ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus,
					     ADP5585_GPI_INTERRUPT_EN_A + bank, (0b1U << bank_pin),
					     (adp5585_int_disable << bank_pin));
	} else if ((trig & GPIO_INT_TRIG_BOTH) != 0) {
		if (trig == GPIO_INT_TRIG_LOW) {
			ret = i2c_reg_update_byte_dt(
				&parent_cfg->i2c_bus, ADP5585_GPI_INT_LEVEL_A + bank,
				(0b1U << bank_pin), (adp5585_int_active_low << bank_pin));
		} else {
			ret = i2c_reg_update_byte_dt(
				&parent_cfg->i2c_bus, ADP5585_GPI_INT_LEVEL_A + bank,
				(0b1U << bank_pin), (adp5585_int_active_high << bank_pin));
		}

		/* make sure GPI_n_EVENT_EN is disabled, otherwise it will generate FIFO event */
		ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus, ADP5585_GPI_EVENT_EN_A + bank,
					     (0b1U << bank_pin), 0U);
		ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus,
					     ADP5585_GPI_INTERRUPT_EN_A + bank, (0b1U << bank_pin),
					     (adp5585_int_enable << bank_pin));
	}

	k_sem_give(&parent_data->lock);
	return ret;
}

static int gpio_adp5585_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct adp5585_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

void gpio_adp5585_irq_handler(const struct device *dev)
{
	const struct adp5585_gpio_config *cfg = dev->config;
	struct adp5585_gpio_data *data = dev->data;
	const struct mfd_adp5585_config *parent_cfg =
		(struct mfd_adp5585_config *)(cfg->mfd_dev->config);
	struct mfd_adp5585_data *parent_data = (struct mfd_adp5585_data *)(cfg->mfd_dev->data);

	uint16_t reg_int_status;
	int ret = 0;

	k_sem_take(&parent_data->lock, K_FOREVER);

	/* Read Input Register */
	ret = i2c_burst_read_dt(&parent_cfg->i2c_bus, ADP5585_GPI_INT_STAT_A,
				(uint8_t *)&reg_int_status, 2U);
	if (ret != 0) {
		LOG_WRN("%s failed to read interrupt status %d", dev->name, ret);
		goto out;
	}

out:
	k_sem_give(&parent_data->lock);

	if (ret == 0 && reg_int_status != 0) {
		gpio_fire_callbacks(&data->callbacks, dev, reg_int_status);
	}
}

/**
 * @brief Initialization function of ADP5585_GPIO
 *
 * This sets initial input/ output configuration and output states.
 * The interrupt is configured if this is enabled.
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_adp5585_init(const struct device *dev)
{
	const struct adp5585_gpio_config *cfg = dev->config;
	/* struct adp5585_gpio_data *data = dev->data; */
	const struct mfd_adp5585_config *parent_cfg =
		(struct mfd_adp5585_config *)(cfg->mfd_dev->config);
	struct mfd_adp5585_data *parent_data = (struct mfd_adp5585_data *)(cfg->mfd_dev->data);
	int ret = 0;

	if (!device_is_ready(cfg->mfd_dev)) {
		LOG_ERR("%s: parent dev not ready", dev->name);
		ret = -ENODEV;
		goto out;
	}

	if (!device_is_ready(parent_cfg->i2c_bus.bus)) {
		LOG_ERR("I2C bus device not found");
		ret = -EIO;
		goto out;
	}

	k_sem_take(&parent_data->lock, K_FOREVER);

	/** Set RPULL to high-z by default */
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&parent_cfg->i2c_bus, ADP5585_RPULL_CONFIG_A, 0xffU);
	}
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&parent_cfg->i2c_bus, ADP5585_RPULL_CONFIG_B, 0x03U);
	}
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&parent_cfg->i2c_bus, ADP5585_RPULL_CONFIG_C, 0xffU);
	}
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&parent_cfg->i2c_bus, ADP5585_RPULL_CONFIG_D, 0x03U);
	}

	parent_data->child.gpio_dev = dev;

	/** Enable GPI interrupt */
	if ((ret == 0) && gpio_is_ready_dt(&parent_cfg->nint_gpio)) {
		ret = i2c_reg_update_byte_dt(&parent_cfg->i2c_bus, ADP5585_INT_EN, (1U << 1),
					     (1U << 1));
	}

out:
	k_sem_give(&parent_data->lock);
	if (ret) {
		LOG_ERR("%s init failed: %d", dev->name, ret);
	} else {
		LOG_INF("%s init ok", dev->name);
	}
	return ret;
}

static const struct gpio_driver_api api_table = {
	.pin_configure = gpio_adp5585_config,
	.port_get_raw = gpio_adp5585_port_read,
	.port_set_masked_raw = gpio_adp5585_port_set_masked,
	.port_set_bits_raw = gpio_adp5585_port_set_bits,
	.port_clear_bits_raw = gpio_adp5585_port_clear_bits,
	.port_toggle_bits = gpio_adp5585_port_toggle_bits,
	.pin_interrupt_configure = gpio_adp5585_pin_interrupt_configure,
	.manage_callback = gpio_adp5585_manage_callback,
};

#define GPIO_ADP5585_INIT(inst)                                                                    \
	const uint8_t adp5585_gpio_pin_gaps_##inst[] =                                             \
		DT_INST_PROP_OR(inst, gpio_reserved_ranges, {});                                   \
                                                                                                   \
	static const struct adp5585_gpio_config adp5585_gpio_cfg_##inst = {                        \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),            \
			},                                                                         \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.pin_gaps = (const struct gpio_pin_gaps *)adp5585_gpio_pin_gaps_##inst,            \
		.gap_count = (ARRAY_SIZE(adp5585_gpio_pin_gaps_##inst) / 2)};                      \
                                                                                                   \
	static struct adp5585_gpio_data adp5585_gpio_drvdata_##inst = {                            \
		.output = ALL_PINS,                                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_adp5585_init, NULL, &adp5585_gpio_drvdata_##inst,         \
			      &adp5585_gpio_cfg_##inst, POST_KERNEL,                               \
			      CONFIG_GPIO_ADP5585_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ADP5585_INIT)
