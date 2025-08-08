/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gpio

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <sl_hal_gpio.h>

LOG_MODULE_REGISTER(gpio_silabs, CONFIG_GPIO_LOG_LEVEL);

#if GPIO_SILABS_COMMON_INIT_PRIORITY >= CONFIG_GPIO_INIT_PRIORITY
#error GPIO_SILABS_COMMON_INIT_PRIORITY must be less than CONFIG_GPIO_INIT_PRIORITY
#endif

#define SILABS_GPIO_PORT_ADDR_SPACE_SIZE sizeof(GPIO_PORT_TypeDef)
#define GET_SILABS_GPIO_INDEX(node_id)                                                             \
	(DT_REG_ADDR(node_id) - DT_REG_ADDR(DT_NODELABEL(gpioa))) / SILABS_GPIO_PORT_ADDR_SPACE_SIZE

#define NUMBER_OF_PORTS (SIZEOF_FIELD(GPIO_TypeDef, P) / SIZEOF_FIELD(GPIO_TypeDef, P[0]))

struct gpio_silabs_common_config {
	/* IRQ configuration function */
	void (*irq_connect)(const struct device *dev);
	/* Clock device */
	const struct device *clock;
	/* Clock control subsystem */
	const struct silabs_clock_control_cmu_config clock_cfg;
};

struct gpio_silabs_common_data {
	/* a list of all registered GPIO port devices */
	const struct device *ports[NUMBER_OF_PORTS];
	/* Total number of registered ports */
	size_t count;
	/* maps interrupt line index to GPIO pin number, 0xFF if unused */
	uint8_t irq_pin_map[16];
};

struct gpio_silabs_port_config {
	/* gpio_driver_config must be first */
	struct gpio_driver_config common;
	/* index of the GPIO port */
	sl_gpio_port_t gpio_index;
};

struct gpio_silabs_port_data {
	/* gpio_driver_data must be first */
	struct gpio_driver_data common;
	/* port ISR callback routine list */
	sys_slist_t callbacks;
	/* bitmask of pins with interrupt enabled */
	uint32_t int_enabled_mask;
	/* pointer to common device */
	const struct device *common_dev;
};

static inline void gpio_silabs_add_port(struct gpio_silabs_common_data *data,
					const struct device *dev)
{
	__ASSERT(dev, "No port device!");
	data->ports[data->count++] = dev;
	LOG_DBG("Added GPIO port %s, count: %d", dev->name, data->count);
}

static int gpio_silabs_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_silabs_port_config *config = dev->config;
	sl_gpio_t gpio = {.port = config->gpio_index, .pin = pin};
	sl_gpio_mode_t mode;
	uint16_t out = 0U;
	bool pin_out;

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				mode = SL_GPIO_MODE_WIRED_AND;
			} else {
				mode = SL_GPIO_MODE_WIRED_OR;
			}
		} else {
			mode = SL_GPIO_MODE_PUSH_PULL;
		}
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			out = 1U;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			out = 0U;
		} else {
			pin_out = sl_hal_gpio_get_pin_output(&gpio);
			out = pin_out;
		}
	} else if (flags & GPIO_INPUT) {
		if (flags & GPIO_PULL_UP) {
			mode = SL_GPIO_MODE_INPUT_PULL;
			out = 1U;
		} else if (flags & GPIO_PULL_DOWN) {
			mode = SL_GPIO_MODE_INPUT_PULL;
		} else {
			mode = SL_GPIO_MODE_INPUT;
		}
	} else {
		mode = SL_GPIO_MODE_DISABLED;
	}

	sl_hal_gpio_set_pin_mode(&gpio, mode, out);
	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_silabs_pin_get_config(const struct device *dev, gpio_pin_t pin,
				      gpio_flags_t *out_flags)
{
	const struct gpio_silabs_port_config *config = dev->config;
	sl_gpio_t gpio = {.port = config->gpio_index, .pin = pin};
	sl_gpio_mode_t mode;
	bool out;
	gpio_flags_t flags = 0;

	mode = sl_hal_gpio_get_pin_mode(&gpio);
	out = sl_hal_gpio_get_pin_output(&gpio);

	switch (mode) {
	case SL_GPIO_MODE_WIRED_AND:
		flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;

		if (out) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
		break;

	case SL_GPIO_MODE_WIRED_OR:
		flags = GPIO_OUTPUT | GPIO_OPEN_SOURCE;

		if (out) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
		break;

	case SL_GPIO_MODE_PUSH_PULL:
		flags = GPIO_OUTPUT | GPIO_PUSH_PULL;

		if (out) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
		break;

	case SL_GPIO_MODE_INPUT_PULL:
		flags = GPIO_INPUT;

		if (out) {
			flags |= GPIO_PULL_UP;
		} else {
			flags |= GPIO_PULL_DOWN;
		}
		break;

	case SL_GPIO_MODE_INPUT:
		flags = GPIO_INPUT;
		break;

	case SL_GPIO_MODE_DISABLED:
		flags = GPIO_DISCONNECTED;
		break;

	default:
		break;
	}

	*out_flags = flags;
	return 0;
}
#endif

static int gpio_silabs_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_silabs_port_config *config = dev->config;

	*value = sl_hal_gpio_get_port_input(config->gpio_index);

	return 0;
}

static int gpio_silabs_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct gpio_silabs_port_config *config = dev->config;

	sl_hal_gpio_set_port_value(config->gpio_index, value, mask);

	return 0;
}

static int gpio_silabs_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_silabs_port_config *config = dev->config;

	sl_hal_gpio_set_port(config->gpio_index, mask);

	return 0;
}

static int gpio_silabs_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_silabs_port_config *config = dev->config;

	sl_hal_gpio_clear_port(config->gpio_index, mask);

	return 0;
}

static int gpio_silabs_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_silabs_port_config *config = dev->config;

	sl_hal_gpio_toggle_port(config->gpio_index, mask);

	return 0;
}

static int gpio_silabs_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_silabs_port_config *config = dev->config;
	struct gpio_silabs_port_data *data = dev->data;
	struct gpio_silabs_common_data *common = data->common_dev->data;
	sl_gpio_t gpio = {.port = config->gpio_index, .pin = pin};
	int32_t int_no = SL_GPIO_INTERRUPT_UNAVAILABLE;
	sl_gpio_interrupt_flag_t flag = SL_GPIO_INTERRUPT_RISING_FALLING_EDGE;

	if (mode == GPIO_INT_MODE_LEVEL) {
		LOG_ERR("Level interrupt not supported on pin %u", pin);
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		ARRAY_FOR_EACH(common->irq_pin_map, i) {
			if ((data->int_enabled_mask & BIT(i)) && (common->irq_pin_map[i] == pin)) {
				sl_hal_gpio_disable_interrupts(BIT(i));
				WRITE_BIT(data->int_enabled_mask, i, false);
				common->irq_pin_map[i] = 0xFF;
				break;
			}
		}
	} else {
		/* Check if pin already has an interrupt configured */
		ARRAY_FOR_EACH(common->irq_pin_map, i) {
			if ((data->int_enabled_mask & BIT(i)) && (common->irq_pin_map[i] == pin)) {
				int_no = i;
				break;
			}
		}

		if (trig == GPIO_INT_TRIG_LOW) {
			flag = SL_GPIO_INTERRUPT_FALLING_EDGE;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			flag = SL_GPIO_INTERRUPT_RISING_EDGE;
		} else {
			flag = SL_GPIO_INTERRUPT_RISING_FALLING_EDGE;
		}

		int_no = sl_hal_gpio_configure_external_interrupt(&gpio, int_no, flag);

		if (int_no == SL_GPIO_INTERRUPT_UNAVAILABLE) {
			LOG_ERR("No available interrupt for pin %u", pin);
			return -EINVAL;
		}

		common->irq_pin_map[int_no] = pin;

		sl_hal_gpio_enable_interrupts(BIT(int_no));
		WRITE_BIT(data->int_enabled_mask, int_no, true);
	}

	return 0;
}

static int gpio_silabs_port_manage_callback(const struct device *dev, struct gpio_callback *cb,
					    bool set)
{
	struct gpio_silabs_port_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static void gpio_silabs_common_isr(const struct device *dev)
{
	struct gpio_silabs_common_data *data = dev->data;
	uint32_t int_status = GPIO->IF;

	ARRAY_FOR_EACH(data->ports, i) {
		if (!int_status) {
			break;
		}
		const struct device *port_dev = data->ports[i];
		struct gpio_silabs_port_data *port_data = port_dev->data;
		uint32_t enabled_int = int_status & port_data->int_enabled_mask;

		if (enabled_int) {
			uint32_t pin_mask = 0;

			ARRAY_FOR_EACH(data->irq_pin_map, int_line) {
				if ((enabled_int & BIT(int_line)) &&
				    (data->irq_pin_map[int_line] != 0xFF)) {
					pin_mask |= BIT(data->irq_pin_map[int_line]);
				}
			}

			GPIO->IF_CLR = enabled_int;
			if (pin_mask) {
				gpio_fire_callbacks(&port_data->callbacks, port_dev, pin_mask);
			}
			int_status &= ~enabled_int;
		}
	}
}

static DEVICE_API(gpio, gpio_driver_api) = {
	.pin_configure = gpio_silabs_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_silabs_pin_get_config,
#endif
	.port_get_raw = gpio_silabs_port_get_raw,
	.port_set_masked_raw = gpio_silabs_port_set_masked_raw,
	.port_set_bits_raw = gpio_silabs_port_set_bits_raw,
	.port_clear_bits_raw = gpio_silabs_port_clear_bits_raw,
	.port_toggle_bits = gpio_silabs_port_toggle_bits,
	.pin_interrupt_configure = gpio_silabs_pin_interrupt_configure,
	.manage_callback = gpio_silabs_port_manage_callback,
};

static DEVICE_API(gpio, gpio_common_driver_api) = {
	.manage_callback = gpio_silabs_port_manage_callback,
};

static struct gpio_silabs_common_data gpio_silabs_common_data_inst;

static int gpio_silabs_common_init(const struct device *dev)
{
	const struct gpio_silabs_common_config *cfg = dev->config;
	struct gpio_silabs_common_data *data = dev->data;
	int ret;

	/* Enable clock */
	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	ARRAY_FOR_EACH(data->irq_pin_map, i) {
		data->irq_pin_map[i] = 0xFF;
	}

	if (cfg->irq_connect) {
		cfg->irq_connect(dev);
	}

	LOG_DBG("Silabs GPIO common init complete");
	return 0;
}

#define GPIO_PORT_INIT(n)                                                                          \
	static int gpio_silabs_port_##n##_init(const struct device *dev)                           \
	{                                                                                          \
		struct gpio_silabs_port_data *data = dev->data;                                    \
		data->common_dev = DEVICE_DT_GET(DT_PARENT(n));                                    \
		gpio_silabs_add_port(&gpio_silabs_common_data_inst, dev);                          \
		return 0;                                                                          \
	}                                                                                          \
	static const struct gpio_silabs_port_config gpio_silabs_port_##n##_config = {              \
		.common = {.port_pin_mask = (gpio_port_pins_t)(-1)},                               \
		.gpio_index = GET_SILABS_GPIO_INDEX(n),                                            \
	};                                                                                         \
	static struct gpio_silabs_port_data gpio_silabs_port_##n##_data;                           \
	DEVICE_DT_DEFINE(n, gpio_silabs_port_##n##_init, NULL, &gpio_silabs_port_##n##_data,       \
			 &gpio_silabs_port_##n##_config, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,  \
			 &gpio_driver_api);

#define GPIO_CONTROLLER_INIT(idx)                                                                  \
	static void gpio_silabs_irq_connect_##idx(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, gpio_even, irq),                              \
			    DT_INST_IRQ_BY_NAME(idx, gpio_even, priority), gpio_silabs_common_isr, \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, gpio_odd, irq),                               \
			    DT_INST_IRQ_BY_NAME(idx, gpio_odd, priority), gpio_silabs_common_isr,  \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, gpio_even, irq));                              \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, gpio_odd, irq));                               \
	}                                                                                          \
	static const struct gpio_silabs_common_config gpio_silabs_common_config_##idx = {          \
		.irq_connect = gpio_silabs_irq_connect_##idx,                                      \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                  \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, gpio_silabs_common_init, NULL, &gpio_silabs_common_data_inst,   \
			      &gpio_silabs_common_config_##idx, PRE_KERNEL_1,                      \
			      CONFIG_GPIO_SILABS_COMMON_INIT_PRIORITY, &gpio_common_driver_api);   \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, GPIO_PORT_INIT)

DT_INST_FOREACH_STATUS_OKAY(GPIO_CONTROLLER_INIT)
