/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct38xx_gpio_port

#include <zephyr/drivers/gpio.h>

#include "gpio_nct38xx.h"
#include "gpio_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(gpio_ntc38xx, CONFIG_GPIO_LOG_LEVEL);

/* Driver config */
struct gpio_nct38xx_port_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* NCT38XX controller dev */
	const struct device *nct38xx_dev;
	/* GPIO port index */
	uint8_t gpio_port;
	/* GPIO port 0 pinmux mask */
	uint8_t pinmux_mask;
};

/* Driver data */
struct gpio_nct38xx_port_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* GPIO callback list */
	sys_slist_t cb_list_gpio;
	/* lock GPIO register access */
	struct k_sem lock;
};

/* GPIO api functions */
static int gpio_nct38xx_pin_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint32_t mask = BIT(pin);
	uint8_t reg, new_reg;
	int ret;

	/* Don't support simultaneous in/out mode */
	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0) && ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) {
		return -ENOTSUP;
	}

	/* Don't support pull-up/pull-down */
	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Pin multiplexing */
	if (config->gpio_port == 0) {
		ret = nct38xx_reg_read_byte(config->nct38xx_dev, NCT38XX_REG_MUX_CONTROL, &reg);
		if (ret < 0) {
			goto done;
		}

		new_reg = reg | mask;
		/* NCT3807 bit3 should be set to 0 */
		new_reg &= config->pinmux_mask;

		ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_MUX_CONTROL, reg,
					 new_reg);
		if (ret < 0) {
			goto done;
		}
	}

	/* Configure pin as input. */
	if (flags & GPIO_INPUT) {
		ret = nct38xx_reg_read_byte(config->nct38xx_dev,
					    NCT38XX_REG_GPIO_DIR(config->gpio_port), &reg);
		if (ret < 0) {
			goto done;
		}
		new_reg = reg & ~mask;
		ret = nct38xx_reg_update(config->nct38xx_dev,
					 NCT38XX_REG_GPIO_DIR(config->gpio_port), reg, new_reg);

		k_sem_give(&data->lock);
		return ret;
	}

	/* Select open drain 0:push-pull 1:open-drain */
	ret = nct38xx_reg_read_byte(config->nct38xx_dev, NCT38XX_REG_GPIO_OD_SEL(config->gpio_port),
				    &reg);
	if (ret < 0) {
		goto done;
	}
	if (flags & GPIO_OPEN_DRAIN) {
		new_reg = reg | mask;
	} else {
		new_reg = reg & ~mask;
	}
	ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_GPIO_OD_SEL(config->gpio_port),
				 reg, new_reg);
	if (ret < 0) {
		goto done;
	}

	/* Set level 0:low 1:high */
	nct38xx_reg_read_byte(config->nct38xx_dev, NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port),
			      &reg);
	if (ret < 0) {
		goto done;
	}
	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		new_reg = reg | mask;
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		new_reg = reg & ~mask;
	}
	ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port),
				 reg, new_reg);
	if (ret < 0) {
		goto done;
	}

	/* Configure pin as output, if requested 0:input 1:output */
	if (flags & GPIO_OUTPUT) {
		nct38xx_reg_read_byte(config->nct38xx_dev, NCT38XX_REG_GPIO_DIR(config->gpio_port),
				      &reg);
		if (ret < 0) {
			goto done;
		}
		new_reg = reg | mask;
		ret = nct38xx_reg_update(config->nct38xx_dev,
					 NCT38XX_REG_GPIO_DIR(config->gpio_port), reg, new_reg);
	}

done:
	k_sem_give(&data->lock);
	return ret;
}

static int gpio_nct38xx_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;

	return nct38xx_reg_read_byte(config->nct38xx_dev,
				     NCT38XX_REG_GPIO_DATA_IN(config->gpio_port), (uint8_t *)value);
}

static int gpio_nct38xx_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint8_t reg, new_reg;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port), &reg);
	if (ret < 0) {
		goto done;
	}
	new_reg = ((reg & ~mask) | (value & mask));
	ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port),
				 reg, new_reg);

done:
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_nct38xx_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint8_t reg, new_reg;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port), &reg);
	if (ret < 0) {
		goto done;
	}
	new_reg = reg | mask;
	ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port),
				 reg, new_reg);

done:
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_nct38xx_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint8_t reg, new_reg;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port), &reg);
	if (ret < 0) {
		goto done;
	}
	new_reg = reg & ~mask;
	ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port),
				 reg, new_reg);

done:
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_nct38xx_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint8_t reg, new_reg;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port), &reg);
	if (ret < 0) {
		goto done;
	}
	new_reg = reg ^ mask;
	ret = nct38xx_reg_update(config->nct38xx_dev, NCT38XX_REG_GPIO_DATA_OUT(config->gpio_port),
				 reg, new_reg);

done:
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_nct38xx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint8_t reg, new_reg, rise, new_rise, fall, new_fall;
	int ret;
	uint32_t mask = BIT(pin);

	k_sem_take(&data->lock, K_FOREVER);

	/* Disable irq before configuring them */
	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_ALERT_MASK(config->gpio_port), &reg);
	if (ret < 0) {
		goto done;
	}
	new_reg = reg & ~mask;
	ret = nct38xx_reg_update(config->nct38xx_dev,
				 NCT38XX_REG_GPIO_ALERT_MASK(config->gpio_port), reg, new_reg);
	if (ret < 0) {
		goto done;
	}

	/* Configure and enable interrupt? */
	if (mode == GPIO_INT_MODE_DISABLED) {
		goto done;
	}

	/* set edge register */
	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_ALERT_RISE(config->gpio_port), &rise);
	if (ret < 0) {
		goto done;
	}
	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_ALERT_FALL(config->gpio_port), &fall);
	if (ret < 0) {
		goto done;
	}

	if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_LOW) {
			new_rise = rise & ~mask;
			new_fall = fall | mask;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			new_rise = rise | mask;
			new_fall = fall & ~mask;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			new_rise = rise | mask;
			new_fall = fall | mask;
		} else {
			LOG_ERR("Invalid interrupt trigger type %d", trig);
			return -EINVAL;
		}
	} else {
		/* level mode */
		new_rise = rise & ~mask;
		new_fall = fall & ~mask;
	}

	ret = nct38xx_reg_update(config->nct38xx_dev,
				 NCT38XX_REG_GPIO_ALERT_RISE(config->gpio_port), rise, new_rise);
	if (ret < 0) {
		goto done;
	}

	ret = nct38xx_reg_update(config->nct38xx_dev,
				 NCT38XX_REG_GPIO_ALERT_FALL(config->gpio_port), fall, new_fall);
	if (ret < 0) {
		goto done;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		/* set active high/low */
		ret = nct38xx_reg_read_byte(config->nct38xx_dev,
					    NCT38XX_REG_GPIO_ALERT_LEVEL(config->gpio_port), &reg);
		if (ret < 0) {
			goto done;
		}

		if (trig == GPIO_INT_TRIG_LOW) {
			new_reg = reg & ~mask;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			new_reg = reg | mask;
		} else {
			LOG_ERR("Invalid interrupt trigger type %d", trig);
			ret = -EINVAL;
			goto done;
		}
		ret = nct38xx_reg_update(config->nct38xx_dev,
					 NCT38XX_REG_GPIO_ALERT_LEVEL(config->gpio_port), reg,
					 new_reg);

		if (ret < 0) {
			goto done;
		}
	}

	/* Clear pending bit */
	ret = nct38xx_reg_write_byte(config->nct38xx_dev,
				     NCT38XX_REG_GPIO_ALERT_STAT(config->gpio_port), mask);
	if (ret < 0) {
		goto done;
	}

	/* Enable it after configuration is completed */
	ret = nct38xx_reg_read_byte(config->nct38xx_dev,
				    NCT38XX_REG_GPIO_ALERT_MASK(config->gpio_port), &reg);
	if (ret < 0) {
		goto done;
	}
	new_reg = reg | mask;
	ret = nct38xx_reg_update(config->nct38xx_dev,
				 NCT38XX_REG_GPIO_ALERT_MASK(config->gpio_port), reg, new_reg);

done:
	k_sem_give(&data->lock);

	return ret;
}

static int gpio_nct38xx_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct gpio_nct38xx_port_data *const data = dev->data;

	return gpio_manage_callback(&data->cb_list_gpio, callback, set);
}

int gpio_nct38xx_dispatch_port_isr(const struct device *dev)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;
	uint8_t alert_pins, mask;
	int ret;

	do {
		k_sem_take(&data->lock, K_FOREVER);
		ret = nct38xx_reg_read_byte(config->nct38xx_dev,
					    NCT38XX_REG_GPIO_ALERT_STAT(config->gpio_port),
					    &alert_pins);
		if (ret < 0) {
			k_sem_give(&data->lock);
			return ret;
		}

		ret = nct38xx_reg_read_byte(config->nct38xx_dev,
					    NCT38XX_REG_GPIO_ALERT_MASK(config->gpio_port), &mask);
		if (ret < 0) {
			k_sem_give(&data->lock);
			return ret;
		}
		alert_pins &= mask;
		if (alert_pins) {
			ret = nct38xx_reg_write_byte(config->nct38xx_dev,
						     NCT38XX_REG_GPIO_ALERT_STAT(config->gpio_port),
						     alert_pins);
			if (ret < 0) {
				k_sem_give(&data->lock);
				return ret;
			}
		}
		k_sem_give(&data->lock);

		gpio_fire_callbacks(&data->cb_list_gpio, dev, alert_pins);

		/*
		 * Vendor defined alert is generated if at least one STATn bit
		 * changes from 0 to 1. We should guarantee the STATn bit is
		 * clear to 0 before leaving isr.
		 */
	} while (alert_pins);

	return 0;
}

static const struct gpio_driver_api gpio_nct38xx_driver = {
	.pin_configure = gpio_nct38xx_pin_config,
	.port_get_raw = gpio_nct38xx_port_get_raw,
	.port_set_masked_raw = gpio_nct38xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_nct38xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nct38xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_nct38xx_port_toggle_bits,
	.pin_interrupt_configure = gpio_nct38xx_pin_interrupt_configure,
	.manage_callback = gpio_nct38xx_manage_callback,
};

static int gpio_nct38xx_port_init(const struct device *dev)
{
	const struct gpio_nct38xx_port_config *const config = dev->config;
	struct gpio_nct38xx_port_data *const data = dev->data;

	if (!device_is_ready(config->nct38xx_dev)) {
		LOG_ERR("%s is not ready", config->nct38xx_dev->name);
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	return 0;
}

/* NCT38XX GPIO port driver must be initialized after NCT38XX GPIO driver */
BUILD_ASSERT(CONFIG_GPIO_NCT38XX_PORT_INIT_PRIORITY > CONFIG_GPIO_NCT38XX_INIT_PRIORITY);

#define GPIO_NCT38XX_PORT_DEVICE_INSTANCE(inst)                                                    \
	static const struct gpio_nct38xx_port_config gpio_nct38xx_port_cfg_##inst = {              \
		.common = { .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst) &               \
					     DT_INST_PROP(inst, pin_mask) },                       \
		.nct38xx_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                \
		.gpio_port = DT_INST_REG_ADDR(inst),                                               \
		.pinmux_mask = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinmux_mask),               \
					   (DT_INST_PROP(inst, pinmux_mask)), (0)),                \
	};                                                                                         \
	BUILD_ASSERT(!(DT_INST_REG_ADDR(inst) == 0 &&                                              \
		       !(DT_INST_NODE_HAS_PROP(inst, pinmux_mask))),                               \
		     "Port 0 should assign pinmux_mask property.");                                \
	static struct gpio_nct38xx_port_data gpio_nct38xx_port_data_##inst;                        \
	DEVICE_DT_INST_DEFINE(inst, gpio_nct38xx_port_init, NULL, &gpio_nct38xx_port_data_##inst,  \
			      &gpio_nct38xx_port_cfg_##inst, POST_KERNEL,                          \
			      CONFIG_GPIO_NCT38XX_PORT_INIT_PRIORITY, &gpio_nct38xx_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NCT38XX_PORT_DEVICE_INSTANCE)
