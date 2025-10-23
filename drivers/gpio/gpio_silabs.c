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

#define SILABS_GPIO_PORT_ADDR_SPACE_SIZE sizeof(GPIO_PORT_TypeDef)
#define GET_SILABS_GPIO_INDEX(node_id)                                                             \
	(DT_REG_ADDR(node_id) - DT_REG_ADDR(DT_NODELABEL(gpioa))) / SILABS_GPIO_PORT_ADDR_SPACE_SIZE

#define NUMBER_OF_PORTS      (SIZEOF_FIELD(GPIO_TypeDef, P) / SIZEOF_FIELD(GPIO_TypeDef, P[0]))
#define NUM_IRQ_LINES        16
#define MAX_EM4_IRQ_PER_PORT 3
#define EM4WU_TO_INT(wu)     ((wu) + NUM_IRQ_LINES)
#define INT_TO_EM4WU(int_no) ((int_no) - NUM_IRQ_LINES)

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
};

struct gpio_silabs_em4wu_mapping {
	uint8_t wu_no;
	uint8_t pin;
};

struct gpio_silabs_port_config {
	/* gpio_driver_config must be first */
	struct gpio_driver_config common;
	/* index of the GPIO port */
	sl_gpio_port_t gpio_index;
	/* pointer to common device */
	const struct device *common_dev;
	/* Number of valid EM4 wakeup interrupt mappings */
	int em4wu_pin_count;
	/* EM4 wakeup interrupt mapping for GPIO pins */
	struct gpio_silabs_em4wu_mapping em4wu_pins[MAX_EM4_IRQ_PER_PORT];
};

struct gpio_silabs_port_data {
	/* gpio_driver_data must be first */
	struct gpio_driver_data common;
	/* port ISR callback routine list */
	sys_slist_t callbacks;
};

static int gpio_silabs_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_silabs_port_config *config = dev->config;
	sl_gpio_t gpio = {
		.port = config->gpio_index,
		.pin = pin
	};
	sl_gpio_mode_t mode;
	bool out = false;

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
			out = true;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			out = false;
		} else {
			out = sl_hal_gpio_get_pin_output(&gpio);
		}
	} else if (flags & GPIO_INPUT) {
		if (flags & GPIO_PULL_UP) {
			mode = SL_GPIO_MODE_INPUT_PULL;
			out = true; /* Pull-up */
		} else if (flags & GPIO_PULL_DOWN) {
			mode = SL_GPIO_MODE_INPUT_PULL;
			out = false; /* Pull-down */
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
	sl_gpio_t gpio = {
		.port = config->gpio_index,
		.pin = pin
	};
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

static int interrupt_to_port(int int_no)
{
	uint32_t reg = 0;

	if (int_no < 8) {
		reg = GPIO->EXTIPSELL;
#if defined(_GPIO_EXTIPSELH_MASK)
	} else {
		int_no -= 8;
		reg = GPIO->EXTIPSELH;
#endif
	}
	return FIELD_GET(0xF << (int_no * 4), reg);
}

static int interrupt_to_pin(int int_no)
{
	int offset = int_no;
	uint32_t reg = 0;

	if (int_no < 8) {
		reg = GPIO->EXTIPINSELL;
#if defined(_GPIO_EXTIPINSELH_MASK)
	} else {
		offset -= 8;
		reg = GPIO->EXTIPINSELH;
#endif
	}
	return ROUND_DOWN(int_no, 4) + FIELD_GET(0xF << (offset * 4), reg);
}

static int gpio_silabs_pin_interrupt_configure_em4wu(sl_gpio_t *gpio, enum gpio_int_mode mode,
						     enum gpio_int_trig trig)
{
	int32_t em4wu_no = sl_hal_gpio_get_em4_interrupt_number(gpio);
	int32_t int_no = EM4WU_TO_INT(em4wu_no);

	if (em4wu_no == SL_GPIO_INTERRUPT_UNAVAILABLE) {
		LOG_ERR("Pin %u is not EM4 wakeup capable", gpio->pin);
		return -EINVAL;
	}

	if (mode != GPIO_INT_MODE_DISABLED) {
		if (trig == GPIO_INT_TRIG_BOTH) {
			LOG_ERR("EM4 wakeup interrupt on pin %u can only trigger on one edge",
				gpio->pin);
			return -ENOTSUP;
		}

		sl_hal_gpio_configure_wakeup_em4_external_interrupt(gpio, em4wu_no,
								    trig == GPIO_INT_TRIG_HIGH);
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		sl_hal_gpio_disable_interrupts(BIT(int_no));
		sl_hal_gpio_disable_pin_em4_wakeup(BIT(int_no));
	} else {
		sl_hal_gpio_enable_interrupts(BIT(int_no));
	}

	return 0;
}

static int gpio_silabs_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_silabs_port_config *config = dev->config;
	sl_gpio_t gpio = {
		.port = config->gpio_index,
		.pin = pin
	};
	sl_gpio_interrupt_flag_t flag = SL_GPIO_INTERRUPT_RISING_FALLING_EDGE;
	uint32_t enabled_interrupts;
	int32_t int_no = SL_GPIO_INTERRUPT_UNAVAILABLE;
	bool em4_wakeup;

	em4_wakeup = ((trig & GPIO_INT_WAKEUP) == GPIO_INT_WAKEUP);
	trig &= ~GPIO_INT_WAKEUP;

	if (mode == GPIO_INT_MODE_LEVEL) {
		LOG_ERR("Level interrupt not supported on pin %u", pin);
		return -ENOTSUP;
	}

	if (em4_wakeup) {
		return gpio_silabs_pin_interrupt_configure_em4wu(&gpio, mode, trig);
	}

	enabled_interrupts = sl_hal_gpio_get_enabled_interrupts();

	for (int i = 0; i < NUM_IRQ_LINES; ++i) {
		if ((enabled_interrupts & BIT(i)) && interrupt_to_port(i) == config->gpio_index &&
		    interrupt_to_pin(i) == pin) {
			if (mode == GPIO_INT_MODE_DISABLED) {
				sl_hal_gpio_disable_interrupts(BIT(i));
			} else if (int_no == SL_GPIO_INTERRUPT_UNAVAILABLE) {
				int_no = i;
			}
		}
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		return 0;
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

	sl_hal_gpio_enable_interrupts(BIT(int_no));

	return 0;
}

static int gpio_silabs_port_manage_callback(const struct device *dev, struct gpio_callback *cb,
					    bool set)
{
	struct gpio_silabs_port_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static void gpio_silabs_em4wu_interrupt_to_port_pin(struct gpio_silabs_common_data *data,
						    int int_no, int *port, int *pin)
{
	ARRAY_FOR_EACH(data->ports, p) {
		const struct device *dev = data->ports[p];
		const struct gpio_silabs_port_config *config = dev->config;

		for (int i = 0; i < config->em4wu_pin_count; i++) {
			const struct gpio_silabs_em4wu_mapping *em4wu = &config->em4wu_pins[i];

			if (em4wu->wu_no == INT_TO_EM4WU(int_no)) {
				*port = config->gpio_index;
				*pin = em4wu->pin;
				return;
			}
		}
	}
}

static void gpio_silabs_common_isr(const struct device *dev)
{
	struct gpio_silabs_common_data *data = dev->data;
	uint32_t pending = sl_hal_gpio_get_enabled_pending_interrupts();
	uint32_t port_pin_masks[NUMBER_OF_PORTS] = { };

	while (pending) {
		int int_no = find_lsb_set(pending) - 1;
		int port = -1;
		int pin = -1;

		if (int_no >= NUM_IRQ_LINES) {
			gpio_silabs_em4wu_interrupt_to_port_pin(data, int_no, &port, &pin);
		} else {
			port = interrupt_to_port(int_no);
			pin = interrupt_to_pin(int_no);
		}

		port_pin_masks[port] |= BIT(pin);
		sl_hal_gpio_clear_interrupts(BIT(int_no));
		pending &= ~BIT(int_no);
	}

	ARRAY_FOR_EACH(data->ports, port) {
		if (data->ports[port] && port_pin_masks[port]) {
			const struct device *port_dev = data->ports[port];
			struct gpio_silabs_port_data *port_data = port_dev->data;

			gpio_fire_callbacks(&port_data->callbacks, port_dev, port_pin_masks[port]);
		}
	}
}

static int gpio_silabs_port_init(const struct device *dev)
{
	const struct gpio_silabs_port_config *config = dev->config;
	struct gpio_silabs_common_data *common_data = config->common_dev->data;

	common_data->ports[config->gpio_index] = dev;
	LOG_DBG("Added GPIO port %s", dev->name);

	return 0;
}

static DEVICE_API(gpio, gpio_driver_api) = {
	.pin_configure           = gpio_silabs_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config          = gpio_silabs_pin_get_config,
#endif
	.port_get_raw            = gpio_silabs_port_get_raw,
	.port_set_masked_raw     = gpio_silabs_port_set_masked_raw,
	.port_set_bits_raw       = gpio_silabs_port_set_bits_raw,
	.port_clear_bits_raw     = gpio_silabs_port_clear_bits_raw,
	.port_toggle_bits        = gpio_silabs_port_toggle_bits,
	.pin_interrupt_configure = gpio_silabs_pin_interrupt_configure,
	.manage_callback         = gpio_silabs_port_manage_callback,
};

static int gpio_silabs_common_init(const struct device *dev)
{
	const struct gpio_silabs_common_config *cfg = dev->config;
	int ret;

	/* Enable clock */
	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	if (cfg->irq_connect) {
		cfg->irq_connect(dev);
	}

	LOG_DBG("Silabs GPIO common init complete");
	return 0;
}

#define EM4_WAKEUP_PIN(node, prop, idx)                                                            \
	{                                                                                          \
		.wu_no = DT_PROP_BY_IDX(node, prop, idx),                                          \
		.pin = DT_PROP_BY_IDX(node, silabs_wakeup_pins, idx),                              \
	},

#define GPIO_PORT_INIT(n)                                                                          \
	static const struct gpio_silabs_port_config gpio_silabs_port_config_##n = {                \
		.common.port_pin_mask = (gpio_port_pins_t)(-1),                                    \
		.gpio_index = GET_SILABS_GPIO_INDEX(n),                                            \
		.common_dev = DEVICE_DT_GET(DT_PARENT(n)),                                         \
		.em4wu_pin_count = DT_PROP_LEN(n, silabs_wakeup_ints),                             \
		.em4wu_pins = {DT_FOREACH_PROP_ELEM(n, silabs_wakeup_ints, EM4_WAKEUP_PIN)},       \
	};                                                                                         \
	static struct gpio_silabs_port_data gpio_silabs_port_data_##n;                             \
	DEVICE_DT_DEFINE(n, gpio_silabs_port_init, NULL, &gpio_silabs_port_data_##n,               \
			 &gpio_silabs_port_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,    \
			 &gpio_driver_api);

#define GPIO_CONTROLLER_INIT(idx)                                                                  \
	static struct gpio_silabs_common_data gpio_silabs_common_data_##idx = {};                  \
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
	DEVICE_DT_INST_DEFINE(idx, gpio_silabs_common_init, NULL, &gpio_silabs_common_data_##idx,  \
			      &gpio_silabs_common_config_##idx, PRE_KERNEL_1,                      \
			      CONFIG_GPIO_SILABS_COMMON_INIT_PRIORITY, NULL);                      \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, GPIO_PORT_INIT)

DT_INST_FOREACH_STATUS_OKAY(GPIO_CONTROLLER_INIT)
