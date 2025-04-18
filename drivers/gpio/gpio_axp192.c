/*
 * Copyright (c) 2023 Martin Kiepfer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp192_gpio

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/mfd/axp192.h>

/* AXP192 GPIO register addresses */
#define AXP192_EXTEN_DCDC2_CONTROL_REG 0x10U
#define AXP192_GPIO012_PINVAL_REG      0x94U
#define AXP192_GPIO34_PINVAL_REG       0x96U
#define AXP192_GPIO012_PULLDOWN_REG    0x97U

#define AXP192_EXTEN_ENA  0x04U
#define AXP192_EXTEN_MASK 0x04U

/* Pull-Down enable parameters */
#define AXP192_GPIO0_PULLDOWN_ENABLE 0x01U
#define AXP192_GPIO1_PULLDOWN_ENABLE 0x02U
#define AXP192_GPIO2_PULLDOWN_ENABLE 0x04U

/* GPIO Value parameters */
#define AXP192_GPIO0_INPUT_VAL      0x10U
#define AXP192_GPIO1_INPUT_VAL      0x20U
#define AXP192_GPIO2_INPUT_VAL      0x40U
#define AXP192_GPIO012_INTPUT_SHIFT 4U
#define AXP192_GPIO012_INTPUT_MASK                                                                 \
	(AXP192_GPIO0_INPUT_VAL | AXP192_GPIO1_INPUT_VAL | AXP192_GPIO2_INPUT_VAL)
#define AXP192_GPIO3_INPUT_VAL     0x10U
#define AXP192_GPIO4_INPUT_VAL     0x20U
#define AXP192_GPIO34_INTPUT_SHIFT 4U
#define AXP192_GPIO34_INTPUT_MASK  (AXP192_GPIO3_INPUT_VAL | AXP192_GPIO4_INPUT_VAL)

#define AXP192_GPIO0_OUTPUT_VAL 0x01U
#define AXP192_GPIO1_OUTPUT_VAL 0x02U
#define AXP192_GPIO2_OUTPUT_VAL 0x04U
#define AXP192_GPIO012_OUTPUT_MASK                                                                 \
	(AXP192_GPIO0_OUTPUT_VAL | AXP192_GPIO1_OUTPUT_VAL | AXP192_GPIO2_OUTPUT_VAL)

#define AXP192_GPIO3_OUTPUT_VAL   0x01U
#define AXP192_GPIO4_OUTPUT_VAL   0x02U
#define AXP192_GPIO34_OUTPUT_MASK (AXP192_GPIO3_OUTPUT_VAL | AXP192_GPIO4_OUTPUT_VAL)

#define AXP192_GPIO5_OUTPUT_MASK  0x04U
#define AXP192_GPIO5_OUTPUT_VAL   0x04U
#define AXP192_GPIO5_OUTPUT_SHIFT 3U

LOG_MODULE_REGISTER(gpio_axp192, CONFIG_GPIO_LOG_LEVEL);

struct gpio_axp192_config {
	struct gpio_driver_config common;
	const struct device *mfd;
	uint32_t ngpios;
};

struct gpio_axp192_data {
	struct gpio_driver_data common;
	sys_slist_t cb_list_gpio;
};

/**
 * @brief Read out the current pull-down configuration of a specific GPIO.
 *
 * @param dev axp192 mfd device
 * @param gpio GPIO to control pull-downs
 * @param enabled Pointer to current pull-down configuration (true: pull-down
 * enabled/ false: pull-down disabled)
 * @retval -EINVAL if an invalid argument is given (e.g. invalid GPIO number)
 * @retval -ENOTSUP if pull-down is not supported by the givenn GPIO
 * @retval -errno in case of any bus error
 */
__maybe_unused static int gpio_axp192_pd_get(const struct device *dev, uint8_t gpio, bool *enabled)
{
	const struct gpio_axp192_config *config = dev->config;
	uint8_t gpio_val;
	uint8_t pd_reg_mask = 0;
	int ret = 0;

	switch (gpio) {
	case 0U:
		pd_reg_mask = AXP192_GPIO0_PULLDOWN_ENABLE;
		break;
	case 1U:
		pd_reg_mask = AXP192_GPIO1_PULLDOWN_ENABLE;
		break;
	case 2U:
		pd_reg_mask = AXP192_GPIO2_PULLDOWN_ENABLE;
		break;

	case 3U:
		__fallthrough;
	case 4U:
		__fallthrough;
	case 5U:
		LOG_DBG("Pull-Down not support on gpio %d", gpio);
		return -ENOTSUP;

	default:
		LOG_ERR("Invalid gpio (%d)", gpio);
		return -EINVAL;
	}

	ret = i2c_reg_read_byte_dt(axp192_get_i2c_dt_spec(config->mfd), AXP192_GPIO012_PULLDOWN_REG,
				   &gpio_val);

	if (ret == 0) {
		*enabled = ((gpio_val & pd_reg_mask) != 0);
		LOG_DBG("Pull-Down stats of gpio %d: %d", gpio, *enabled);
	}

	return 0;
}

/**
 * @brief Enable pull-down on specified GPIO pin. AXP192 only supports
 * pull-down on GPIO3..5. Pull-ups are not supported.
 *
 * @param dev axp192 mfd device
 * @param gpio GPIO to control pull-downs
 * @param enable true to enable, false to disable pull-down
 * @retval 0 on success
 * @retval -EINVAL if an invalid argument is given (e.g. invalid GPIO number)
 * @retval -ENOTSUP if pull-down is not supported by the givenn GPIO
 * @retval -errno in case of any bus error
 */
static int gpio_axp192_pd_ctrl(const struct device *dev, uint8_t gpio, bool enable)
{
	const struct gpio_axp192_config *config = dev->config;
	uint8_t reg_pd_val = 0;
	uint8_t reg_pd_mask = 0;
	int ret = 0;

	/* Configure pull-down. Pull-down is only supported by GPIO3 and GPIO4 */
	switch (gpio) {
	case 0U:
		reg_pd_mask = AXP192_GPIO0_PULLDOWN_ENABLE;
		if (enable) {
			reg_pd_val = AXP192_GPIO0_PULLDOWN_ENABLE;
		}
		break;

	case 1U:
		reg_pd_mask = AXP192_GPIO1_PULLDOWN_ENABLE;
		if (enable) {
			reg_pd_val = AXP192_GPIO1_PULLDOWN_ENABLE;
		}
		break;

	case 2U:
		reg_pd_mask = AXP192_GPIO2_PULLDOWN_ENABLE;
		if (enable) {
			reg_pd_val = AXP192_GPIO2_PULLDOWN_ENABLE;
		}
		break;

	case 3U:
		__fallthrough;
	case 4U:
		__fallthrough;
	case 5U:
		LOG_ERR("Pull-Down not support on gpio %d", gpio);
		return -ENOTSUP;

	default:
		LOG_ERR("Invalid gpio (%d)", gpio);
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(axp192_get_i2c_dt_spec(config->mfd),
				     AXP192_GPIO012_PULLDOWN_REG, reg_pd_mask, reg_pd_val);

	return ret;
}

/**
 * @brief Read GPIO port.
 *
 * @param dev axp192 mfd device
 * @param value Pointer to port value
 * @retval 0 on success
 * @retval -errno in case of any bus error
 */
static int gpio_axp192_read_port(const struct device *dev, uint8_t *value)
{
	const struct gpio_axp192_config *config = dev->config;
	uint8_t gpio012_val;
	uint8_t gpio34_val;
	uint8_t gpio5_val;
	uint8_t gpio_input_val;
	uint8_t gpio_output_val;
	int ret;

	/* read gpio0-2 */
	ret = i2c_reg_read_byte_dt(axp192_get_i2c_dt_spec(config->mfd), AXP192_GPIO012_PINVAL_REG,
				   &gpio012_val);
	if (ret != 0) {
		return ret;
	}

	/* read gpio3-4 */
	ret = i2c_reg_read_byte_dt(axp192_get_i2c_dt_spec(config->mfd), AXP192_GPIO34_PINVAL_REG,
				   &gpio34_val);
	if (ret != 0) {
		return ret;
	}

	/* read gpio5 */
	ret = i2c_reg_read_byte_dt(axp192_get_i2c_dt_spec(config->mfd),
				   AXP192_EXTEN_DCDC2_CONTROL_REG, &gpio5_val);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("GPIO012 pinval-reg=0x%x", gpio012_val);
	LOG_DBG("GPIO34 pinval-reg =0x%x", gpio34_val);
	LOG_DBG("GPIO5 pinval-reg =0x%x", gpio5_val);
	LOG_DBG("Output-Mask       =0x%x", axp192_get_gpio_mask_output(config->mfd));

	gpio_input_val =
		((gpio012_val & AXP192_GPIO012_INTPUT_MASK) >> AXP192_GPIO012_INTPUT_SHIFT);
	gpio_input_val |=
		(((gpio34_val & AXP192_GPIO34_INTPUT_MASK) >> AXP192_GPIO34_INTPUT_SHIFT) << 3u);

	gpio_output_val = (gpio012_val & AXP192_GPIO012_OUTPUT_MASK);
	gpio_output_val |= ((gpio34_val & AXP192_GPIO34_OUTPUT_MASK) << 3u);
	gpio_output_val |=
		(((gpio5_val & AXP192_GPIO5_OUTPUT_MASK) >> AXP192_GPIO5_OUTPUT_SHIFT) << 5u);

	*value = gpio_input_val & ~axp192_get_gpio_mask_output(config->mfd);
	*value |= (gpio_output_val & axp192_get_gpio_mask_output(config->mfd));

	return 0;
}

/**
 * @brief Write GPIO port.
 *
 * @param dev axp192 mfd device
 * @param value port value
 * @param mask pin mask within the port
 * @retval 0 on success
 * @retval -errno in case of any bus error
 */
static int gpio_axp192_write_port(const struct device *dev, uint8_t value, uint8_t mask)
{
	const struct gpio_axp192_config *config = dev->config;
	uint8_t gpio_reg_val;
	uint8_t gpio_reg_mask;
	int ret;

	/* Write gpio0-2. Mask out other port pins */
	gpio_reg_val = (value & AXP192_GPIO012_OUTPUT_MASK);
	gpio_reg_mask = (mask & AXP192_GPIO012_OUTPUT_MASK);
	if (gpio_reg_mask != 0) {
		ret = i2c_reg_update_byte_dt(axp192_get_i2c_dt_spec(config->mfd),
					     AXP192_GPIO012_PINVAL_REG, gpio_reg_mask,
					     gpio_reg_val);
		if (ret != 0) {
			return ret;
		}
		LOG_DBG("GPIO012 pinval-reg=0x%x mask=0x%x", gpio_reg_val, gpio_reg_mask);
	}

	/* Write gpio3-4. Mask out other port pins */
	gpio_reg_val = value >> 3U;
	gpio_reg_mask = (mask >> 3U) & AXP192_GPIO34_OUTPUT_MASK;
	if (gpio_reg_mask != 0) {
		ret = i2c_reg_update_byte_dt(axp192_get_i2c_dt_spec(config->mfd),
					     AXP192_GPIO34_PINVAL_REG, gpio_reg_mask, gpio_reg_val);
		if (ret != 0) {
			return ret;
		}
		LOG_DBG("GPIO34 pinval-reg =0x%x mask=0x%x", gpio_reg_val, gpio_reg_mask);
	}

	/* Write gpio5. Mask out other port pins */
	if ((mask & BIT(5)) != 0) {
		gpio_reg_mask = AXP192_EXTEN_MASK;
		gpio_reg_val = (value & BIT(5)) ? AXP192_EXTEN_ENA : 0U;
		ret = i2c_reg_update_byte_dt(axp192_get_i2c_dt_spec(config->mfd),
					     AXP192_EXTEN_DCDC2_CONTROL_REG, gpio_reg_mask,
					     gpio_reg_val);
		if (ret != 0) {
			return ret;
		}
		LOG_DBG("GPIO5 pinval-reg =0x%x mask=0x%x\n", gpio_reg_val, gpio_reg_mask);
	}

	return 0;
}

static int gpio_axp192_port_get_raw(const struct device *dev, uint32_t *value)
{
	int ret;
	uint8_t port_val;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = gpio_axp192_read_port(dev, &port_val);
	if (ret == 0) {
		*value = port_val;
	}

	return ret;
}

static int gpio_axp192_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = gpio_axp192_write_port(dev, value, mask);

	return ret;
}

static int gpio_axp192_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_axp192_port_set_masked_raw(dev, pins, pins);
}

static int gpio_axp192_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_axp192_port_set_masked_raw(dev, pins, 0);
}

static int gpio_axp192_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_axp192_config *config = dev->config;
	int ret;
	enum axp192_gpio_func func;

	if (pin >= config->ngpios) {
		LOG_ERR("Invalid gpio pin (%d)", pin);
		return -EINVAL;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Configure pin */
	LOG_DBG("Pin: %d / flags=0x%x", pin, flags);
	if ((flags & GPIO_OUTPUT) != 0) {

		/* Initialize output function */
		func = AXP192_GPIO_FUNC_OUTPUT_LOW;
		if ((flags & GPIO_OPEN_DRAIN) != 0) {
			func = AXP192_GPIO_FUNC_OUTPUT_OD;
		}
		ret = mfd_axp192_gpio_func_ctrl(config->mfd, dev, pin, func);
		if (ret != 0) {
			return ret;
		}

		/* Set init value */
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			ret = gpio_axp192_write_port(dev, BIT(pin), 0);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			ret = gpio_axp192_write_port(dev, BIT(pin), BIT(pin));
		}
	} else if ((flags & GPIO_INPUT) != 0) {

		/* Initialize input function */
		func = AXP192_GPIO_FUNC_INPUT;

		ret = mfd_axp192_gpio_func_ctrl(config->mfd, dev, pin, func);
		if (ret != 0) {
			return ret;
		}

		/* Configure pull-down */
		if ((flags & GPIO_PULL_UP) != 0) {
			/* not supported */
			LOG_ERR("Pull-Up not supported");
			ret = -ENOTSUP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			/* out = 0 means pull-down*/
			ret = gpio_axp192_pd_ctrl(dev, pin, true);
		} else {
			ret = gpio_axp192_pd_ctrl(dev, pin, false);
		}
	} else {
		/* Neither input nor output mode is selected */
		LOG_INF("No valid gpio mode selected");
		ret = -ENOTSUP;
	}

	return ret;
}

static int gpio_axp192_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_axp192_config *config = dev->config;
	int ret;
	uint32_t value;

	k_sem_take(axp192_get_lock(config->mfd), K_FOREVER);

	ret = gpio_axp192_port_get_raw(dev, &value);
	if (ret == 0) {
		ret = gpio_axp192_port_set_masked_raw(dev, pins, ~value);
	}

	k_sem_give(axp192_get_lock(config->mfd));

	return ret;
}

static int gpio_axp192_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

#if defined(CONFIG_GPIO_GET_CONFIG) || defined(CONFIG_GPIO_GET_DIRECTION)
static int gpio_axp192_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct gpio_axp192_config *config = dev->config;
	enum axp192_gpio_func func;
	bool pd_enabled;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = mfd_axp192_gpio_func_get(config->mfd, pin, &func);
	if (ret != 0) {
		return ret;
	}

	/* Set OUTPUT/INPUT flags */
	*out_flags = 0;
	switch (func) {
	case AXP192_GPIO_FUNC_INPUT:
		*out_flags |= GPIO_INPUT;
		break;
	case AXP192_GPIO_FUNC_OUTPUT_OD:
		*out_flags |= GPIO_OUTPUT | GPIO_OPEN_DRAIN;
		break;
	case AXP192_GPIO_FUNC_OUTPUT_LOW:
		*out_flags |= GPIO_OUTPUT;
		break;

	case AXP192_GPIO_FUNC_LDO:
		__fallthrough;
	case AXP192_GPIO_FUNC_ADC:
		__fallthrough;
	case AXP192_GPIO_FUNC_FLOAT:
		__fallthrough;
	default:
		LOG_DBG("Pin %d not configured as GPIO", pin);
		break;
	}

	/* Query pull-down config status  */
	ret = gpio_axp192_pd_get(config->mfd, pin, &pd_enabled);
	if (ret != 0) {
		return ret;
	}

	if (pd_enabled) {
		*out_flags |= GPIO_PULL_DOWN;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_axp192_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_axp192_config *config = dev->config;
	gpio_flags_t flags;
	int ret;

	/* reset output variables */
	*inputs = 0;
	*outputs = 0;

	/* loop through all  */
	for (gpio_pin_t gpio = 0; gpio < config->ngpios; gpio++) {

		if ((map & (1u << gpio)) != 0) {

			/* use internal get_config method to get gpio flags */
			ret = gpio_axp192_get_config(dev, gpio, &flags);
			if (ret != 0) {
				return ret;
			}

			/* Set output and input flags */
			if ((flags & GPIO_OUTPUT) != 0) {
				*outputs |= (1u << gpio);
			} else if (0 != (flags & GPIO_INPUT)) {
				*inputs |= (1u << gpio);
			}
		}
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static int gpio_axp192_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_axp192_data *const data = dev->data;

	return gpio_manage_callback(&data->cb_list_gpio, callback, set);
}

static DEVICE_API(gpio, gpio_axp192_api) = {
	.pin_configure = gpio_axp192_configure,
	.port_get_raw = gpio_axp192_port_get_raw,
	.port_set_masked_raw = gpio_axp192_port_set_masked_raw,
	.port_set_bits_raw = gpio_axp192_port_set_bits_raw,
	.port_clear_bits_raw = gpio_axp192_port_clear_bits_raw,
	.port_toggle_bits = gpio_axp192_port_toggle_bits,
	.pin_interrupt_configure = gpio_axp192_pin_interrupt_configure,
	.manage_callback = gpio_axp192_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_axp192_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_axp192_get_config,
#endif
};

static int gpio_axp192_init(const struct device *dev)
{
	const struct gpio_axp192_config *config = dev->config;
	int ret = 0;

	LOG_DBG("Initializing");

	k_sem_take(axp192_get_lock(config->mfd), K_FOREVER);

	if (!i2c_is_ready_dt(axp192_get_i2c_dt_spec(config->mfd))) {
		LOG_ERR("device not ready");
		ret = -ENODEV;
	}

	k_sem_give(axp192_get_lock(config->mfd));

	return ret;
}

#define GPIO_AXP192_DEFINE(inst)                                                                   \
	static const struct gpio_axp192_config gpio_axp192_config##inst = {                        \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),            \
			},                                                                         \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.ngpios = DT_INST_PROP(inst, ngpios),                                              \
	};                                                                                         \
                                                                                                   \
	static struct gpio_axp192_data gpio_axp192_data##inst;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gpio_axp192_init, NULL, &gpio_axp192_data##inst,               \
			      &gpio_axp192_config##inst, POST_KERNEL,                              \
			      CONFIG_GPIO_AXP192_INIT_PRIORITY, &gpio_axp192_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AXP192_DEFINE)
