/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPO and GPIO pins of the TI TPS65214 (SLVSHK7A) and TPS65215 (SLVSHQ9).
 */

#define DT_DRV_COMPAT ti_tps6521x_gpio

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/tps6521x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(gpio_tps6521x, CONFIG_GPIO_LOG_LEVEL);

/* pin numbers are the GENERAL_CONFIG enable bits */
#define TPS6521X_PIN_GPO1 0U
#define TPS6521X_PIN_GPO2 1U
#define TPS6521X_PIN_GPIO 2U

/*
 * TPS65214: GENERAL_CONFIG bit 1 is GPIO_CONFIG, the GPIO pin direction,
 * 1 = output in SLVSHK7A table 8-29. Table 7-6 shows the opposite and is
 * taken as an error. The NVM owns the direction: the bit is never written,
 * and the pin mask keeps pin writes away from it.
 */
#define TPS65214_GENERAL_CONFIG_GPIO_CONFIG BIT(1)
/* TPS65214: MFP_1_CONFIG GPIO_VSEL_CONFIG, 1 = VSEL */
#define TPS65214_MFP_1_GPIO_VSEL_CONFIG     BIT(3)
/* TPS65214: MFP_2_CONFIG GPO_nWAKEUP_CONFIG, 1 = nWAKEUP */
#define TPS65214_MFP_2_GPO_NWAKEUP_CONFIG   BIT(2)
/* TPS65215: MFP_1_CONFIG MULTI_DEVICE_ENABLE, 1 = GPIO is a sync input */
#define TPS65215_MFP_1_MULTI_DEVICE_ENABLE  BIT(3)

#define TPS6521X_MFP_CTRL_GPIO_STATUS BIT(4)

#define TPS65214_PINS (BIT(TPS6521X_PIN_GPO1) | BIT(TPS6521X_PIN_GPIO))
#define TPS65215_PINS (BIT(TPS6521X_PIN_GPO1) | BIT(TPS6521X_PIN_GPO2) | BIT(TPS6521X_PIN_GPIO))

struct gpio_tps6521x_config {
	struct gpio_driver_config common;
	const struct device *mfd;
};

struct gpio_tps6521x_data {
	struct gpio_driver_data common;
	/* pins configured as outputs; only these are ever written */
	atomic_t outputs;
};

static bool is_tps65214(const struct gpio_tps6521x_config *config)
{
	return mfd_tps6521x_get_variant(config->mfd) == MFD_TPS6521X_VARIANT_TPS65214;
}

static gpio_port_pins_t gpio_tps6521x_pins(const struct gpio_tps6521x_config *config)
{
	return is_tps65214(config) ? TPS65214_PINS : TPS65215_PINS;
}

/*
 * Which directions the NVM pin function allows. Nothing here writes the MFP
 * configuration: GPIO_VSEL_CONFIG and MULTI_DEVICE_ENABLE may only change in
 * the INITIALIZE state.
 */
static int gpio_tps6521x_allowed(const struct device *dev, gpio_pin_t pin, bool *can_in,
				 bool *can_out)
{
	const struct gpio_tps6521x_config *config = dev->config;
	uint8_t mfp;
	int ret;

	*can_in = false;
	*can_out = true;

	if (pin != TPS6521X_PIN_GPIO) {
		if (!is_tps65214(config)) {
			/* GPO1 and GPO2 are always outputs (SLVSHQ9 6.3.11) */
			return 0;
		}

		ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_MFP_2_CONFIG, &mfp);
		if (ret < 0) {
			return ret;
		}
		if ((mfp & TPS65214_MFP_2_GPO_NWAKEUP_CONFIG) != 0U) {
			LOG_ERR("GPO is configured as nWAKEUP");
			*can_out = false;
		}
		return 0;
	}

	ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_MFP_1_CONFIG, &mfp);
	if (ret < 0) {
		return ret;
	}

	if (is_tps65214(config)) {
		if ((mfp & TPS65214_MFP_1_GPIO_VSEL_CONFIG) != 0U) {
			LOG_ERR("GPIO/VSEL is configured as VSEL");
			*can_out = false;
			return 0;
		}
		/* GPIO_STATUS is readable in either direction */
		*can_in = true;

		ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_GENERAL_CONFIG, &mfp);
		if (ret < 0) {
			return ret;
		}
		if ((mfp & TPS65214_GENERAL_CONFIG_GPIO_CONFIG) == 0U) {
			LOG_ERR("GPIO is configured as an input");
			*can_out = false;
		}
		return 0;
	}

	if ((mfp & TPS65215_MFP_1_MULTI_DEVICE_ENABLE) != 0U) {
		/* multi-PMIC synchronisation line: never drive it */
		*can_in = true;
		*can_out = false;
	}

	return 0;
}

static int gpio_tps6521x_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_tps6521x_config *config = dev->config;
	struct gpio_tps6521x_data *data = dev->data;
	bool output = (flags & GPIO_OUTPUT) != 0U;
	bool input = (flags & GPIO_INPUT) != 0U;
	bool can_in;
	bool can_out;
	uint8_t mask = 0U;
	uint8_t val = 0U;
	int ret;

	if ((gpio_tps6521x_pins(config) & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U || (!input && !output)) {
		return -ENOTSUP;
	}

	/* the outputs are open-drain (SLVSHK7A table 4-1, SLVSHQ9 table 4-1) */
	if (output && (flags & GPIO_OPEN_DRAIN) != GPIO_OPEN_DRAIN) {
		return -ENOTSUP;
	}

	ret = gpio_tps6521x_allowed(dev, pin, &can_in, &can_out);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Input and output together is allowed where the pin level can be
	 * read back, which is the GPIO pin only.
	 */
	if ((output && !can_out) || (input && !can_in && !(output && pin == TPS6521X_PIN_GPIO))) {
		return -ENOTSUP;
	}

	if (!output) {
		atomic_clear_bit(&data->outputs, pin);
		return 0;
	}

	/* without an initial level the NVM or current state is kept */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		mask |= BIT(pin);
		val |= BIT(pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		mask |= BIT(pin);
	}

	if (mask != 0U) {
		ret = mfd_tps6521x_reg_update(config->mfd, MFD_TPS6521X_REG_GENERAL_CONFIG, mask,
					      val);
		if (ret < 0) {
			return ret;
		}
	}

	atomic_set_bit(&data->outputs, pin);

	return 0;
}

static int gpio_tps6521x_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_tps6521x_config *config = dev->config;
	uint8_t general;
	uint8_t ctrl;
	int ret;

	ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_GENERAL_CONFIG, &general);
	if (ret < 0) {
		return ret;
	}

	ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_MFP_CTRL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	/* GPOs: output setting; GPIO: real-time pin level */
	*value = general & gpio_tps6521x_pins(config) & ~BIT(TPS6521X_PIN_GPIO);
	if ((ctrl & TPS6521X_MFP_CTRL_GPIO_STATUS) != 0U) {
		*value |= BIT(TPS6521X_PIN_GPIO);
	}

	return 0;
}

static int gpio_tps6521x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct gpio_tps6521x_config *config = dev->config;
	struct gpio_tps6521x_data *data = dev->data;

	mask &= gpio_tps6521x_pins(config) & (gpio_port_pins_t)atomic_get(&data->outputs);
	if (mask == 0U) {
		return 0;
	}

	return mfd_tps6521x_reg_update(config->mfd, MFD_TPS6521X_REG_GENERAL_CONFIG, (uint8_t)mask,
				       (uint8_t)value);
}

static int gpio_tps6521x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_tps6521x_port_set_masked_raw(dev, pins, pins);
}

static int gpio_tps6521x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_tps6521x_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_tps6521x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tps6521x_config *config = dev->config;
	uint8_t general;
	int ret;

	ret = mfd_tps6521x_reg_read(config->mfd, MFD_TPS6521X_REG_GENERAL_CONFIG, &general);
	if (ret < 0) {
		return ret;
	}

	return gpio_tps6521x_port_set_masked_raw(dev, pins, ~general);
}

static DEVICE_API(gpio, gpio_tps6521x_api) = {
	.pin_configure = gpio_tps6521x_pin_configure,
	.port_get_raw = gpio_tps6521x_port_get_raw,
	.port_set_masked_raw = gpio_tps6521x_port_set_masked_raw,
	.port_set_bits_raw = gpio_tps6521x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tps6521x_port_clear_bits_raw,
	.port_toggle_bits = gpio_tps6521x_port_toggle_bits,
};

static int gpio_tps6521x_init(const struct device *dev)
{
	const struct gpio_tps6521x_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	return 0;
}

#define GPIO_TPS6521X_DEFINE(inst)                                                                 \
	static const struct gpio_tps6521x_config gpio_tps6521x_config_##inst = {                   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
	static struct gpio_tps6521x_data gpio_tps6521x_data_##inst;                                \
	DEVICE_DT_INST_DEFINE(inst, gpio_tps6521x_init, NULL, &gpio_tps6521x_data_##inst,          \
			      &gpio_tps6521x_config_##inst, POST_KERNEL,                           \
			      CONFIG_GPIO_TPS6521X_INIT_PRIORITY, &gpio_tps6521x_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TPS6521X_DEFINE)
