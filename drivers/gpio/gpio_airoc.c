/*
 * Copyright (c) 2025 Beechwoods Software, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_airoc_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_airoc, CONFIG_GPIO_LOG_LEVEL);

#include <airoc_whd_hal_common.h>
whd_interface_t airoc_wifi_get_whd_interface(void);
whd_result_t whd_wifi_set_iovar_buffer(whd_interface_t ifp, const char *iovar,
				       void *buffer, uint16_t buffer_length);
whd_result_t whd_wifi_get_iovar_buffer(whd_interface_t ifp, const char *iovar_name,
				       uint8_t *out_buffer, uint16_t out_length);
static whd_interface_t airoc_if;
static uint32_t gpio_value_buffer;

#define IOVAR_STR_GPIOOUT "gpioout"

#define AIROC_GPIO_PINS 5
#define AIROC_GPIO_LOGIC1 0x1
#define AIROC_GPIO_LOGIC0 0x0

#define PSEUDO_IOVAR_GET

struct airoc_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
};

struct airoc_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int airoc_wifi_init(void)
{
	int err = 0;

	if (airoc_if != NULL) {
		return 0;
	}

	LOG_INF("AIROC initializing...");

	airoc_if = airoc_wifi_get_whd_interface();
	if (airoc_if == NULL) {
		LOG_ERR("airoc_if is NULL");
		err = -ENODEV;
	}

	LOG_INF("airoc_if: %p", airoc_if);

	return err;
}

static int airoc_gpio_get(uint32_t *data)
{
	uint32_t result;

#ifdef PSEUDO_IOVAR_GET
	*data = gpio_value_buffer;
	result = 0;
#else
	result = whd_wifi_get_iovar_buffer(airoc_if, IOVAR_STR_GPIOOUT,
					(uint8_t *)data, sizeof(uint32_t));
	/* Read back data is always 0U with this undocumented iovar API */
#endif
	LOG_INF("%s: data: %d size: %zu", __func__, *data, sizeof(*data));
	LOG_INF("result: %d", result);

	if (result != WHD_SUCCESS) {
		LOG_ERR("%s: result: %d", __func__, result);
		return -EIO;
	}
	return 0;
}

static int airoc_gpio_set(uint8_t gpio_pin, uint8_t gpio_state)
{
	uint32_t parameters[] = {gpio_pin, gpio_state};
	uint32_t result;

	LOG_INF("%s: gpio_pin: %d gpio_state: %d", __func__, gpio_pin, gpio_state);

	result = whd_wifi_set_iovar_buffer(airoc_if, IOVAR_STR_GPIOOUT,
				    (uint8_t *)&parameters, sizeof(parameters));

	if (result != WHD_SUCCESS) {
		LOG_ERR("%s: result: %d", __func__, result);
		return -EIO;
	}
	return 0;
}

static int gpio_airoc_pin_configure(const struct device *dev,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{
	LOG_INF("%s: pin_configure", dev->name);

	gpio_value_buffer = 0U;

	if (pin >= AIROC_GPIO_PINS) {
		return -EINVAL;
	}

	if (airoc_wifi_init() != 0) {
		LOG_ERR("airoc_wifi_init failed");
		return -ENODEV;
	}

	return 0;
}

static int gpio_airoc_port_get_raw(const struct device *dev,
				 gpio_port_value_t *value)
{
	int ret;
	uint32_t data;

	ret = airoc_gpio_get(&data);

	if (ret < 0) {
		return ret;
	}

	*value = data;

	return 0;
}

static int gpio_airoc_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	int ret = 0;

	for (size_t idx = 0; idx < AIROC_GPIO_PINS; idx++) {
		if ((mask & BIT(idx)) != 0U) {
			if ((value & BIT(idx)) != 0U) {
				ret = airoc_gpio_set(idx, AIROC_GPIO_LOGIC1);
			} else {
				ret = airoc_gpio_set(idx, AIROC_GPIO_LOGIC0);
			}
			if (ret != 0U) {
				return ret;
			}
		}
	}

	gpio_value_buffer = value;

	return ret;
}

static int gpio_airoc_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t pins)
{
	return gpio_airoc_port_set_masked_raw(dev, pins, pins);
}

static int gpio_airoc_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	return gpio_airoc_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_airoc_toggle_bits(const struct device *dev,
					uint32_t pins)
{
	int ret;
	uint32_t value;

	ret = gpio_airoc_port_get_raw(dev, &value);

	if (ret < 0) {
		return ret;
	}

	return gpio_airoc_port_set_masked_raw(dev, pins, ~value);
}

static DEVICE_API(gpio, airoc_gpio_api) = {
	.pin_configure = gpio_airoc_pin_configure,
	.port_get_raw = gpio_airoc_port_get_raw,
	.port_set_masked_raw = gpio_airoc_port_set_masked_raw,
	.port_set_bits_raw = gpio_airoc_port_set_bits_raw,
	.port_clear_bits_raw = gpio_airoc_port_clear_bits_raw,
	.port_toggle_bits = gpio_airoc_toggle_bits,
};

static int airoc_gpio_init(const struct device *dev)
{
	LOG_INF("%s: initializing", dev->name);

	return 0;
}

#define AIROC_GPIO_INIT(n)							\
	static const struct airoc_gpio_config airoc_gpio_config_##n = {		\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
	};									\
										\
	static struct airoc_gpio_data airoc_gpio_data_##n;			\
										\
	DEVICE_DT_INST_DEFINE(n, airoc_gpio_init, NULL, &airoc_gpio_data_##n,	\
			      &airoc_gpio_config_##n, POST_KERNEL,		\
			      CONFIG_GPIO_INIT_PRIORITY,			\
			      &airoc_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(AIROC_GPIO_INIT)
