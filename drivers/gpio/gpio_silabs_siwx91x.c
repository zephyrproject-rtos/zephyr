/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_gpio

#include "sl_si91x_driver_gpio.h"
#include "sl_status.h"

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

/* Zephyr GPIO header must be included after driver, due to symbol conflicts
 * for GPIO_INPUT and GPIO_OUTPUT between preprocessor macros in the Zephyr
 * API and struct member register definitions for the SiWx91x device.
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#if CONFIG_GPIO_SILABS_SIWX91X_COMMON_INIT_PRIORITY >= CONFIG_GPIO_INIT_PRIORITY
#error CONFIG_GPIO_SILABS_SIWX91X_COMMON_INIT_PRIORITY must be less than \
	CONFIG_GPIO_INIT_PRIORITY.
#endif

#define MAX_PORT_COUNT  4
#define MAX_PIN_COUNT   16
#define INVALID_PORT    0xFF
#define INTERRUPT_COUNT 8

/* Types */
struct gpio_siwx91x_common_config {
	EGPIO_Type *reg;
};

struct gpio_siwx91x_port_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *parent;
	uint8_t pads[MAX_PIN_COUNT];
	int port;
	int hal_port;
	bool ulp;
};

struct gpio_siwx91x_common_data {
	/* a list of all ports */
	const struct device *ports[MAX_PORT_COUNT];
	sl_gpio_t interrupts[INTERRUPT_COUNT];
};

struct gpio_siwx91x_port_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

/* Functions */
static int gpio_siwx91x_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_siwx91x_port_config *cfg = dev->config;
	const struct device *parent = cfg->parent;
	const struct gpio_siwx91x_common_config *pcfg = parent->config;
	sl_status_t status;
	sl_si91x_gpio_driver_disable_state_t disable_state = GPIO_HZ;

	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	uint8_t pad = cfg->pads[pin];

	if (pad == 0) {
		/* Enable MCU pad */
		status = sl_si91x_gpio_driver_enable_host_pad_selection((cfg->hal_port << 4) | pin);
		if (status != SL_STATUS_OK) {
			return -ENODEV;
		}
	} else if (pad != 0xFF && pad != 9) {
		/* Assign pad to MCU subsystem */
		status = sl_si91x_gpio_driver_enable_pad_selection(pad);
		if (status != SL_STATUS_OK) {
			return -ENODEV;
		}
	}

	if (flags & GPIO_PULL_UP) {
		disable_state = GPIO_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		disable_state = GPIO_PULLDOWN;
	}
	if (cfg->ulp) {
		sl_si91x_gpio_select_ulp_pad_driver_disable_state(pin, disable_state);
	} else {
		sl_si91x_gpio_select_pad_driver_disable_state((cfg->port << 4) | pin,
							      disable_state);
	}

	if (flags & GPIO_INPUT) {
		if (cfg->ulp) {
			sl_si91x_gpio_driver_enable_ulp_pad_receiver(pin);
		} else {
			sl_si91x_gpio_driver_enable_pad_receiver((cfg->port << 4) | pin);
		}
	} else {
		if (cfg->ulp) {
			sl_si91x_gpio_driver_disable_ulp_pad_receiver(pin);
		} else {
			sl_si91x_gpio_driver_disable_pad_receiver((cfg->port << 4) | pin);
		}
	}

	pcfg->reg->PIN_CONFIG[(cfg->port << 4) + pin].GPIO_CONFIG_REG_b.MODE = 0;

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		sl_gpio_set_pin_output(cfg->hal_port, pin);
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		sl_gpio_clear_pin_output(cfg->hal_port, pin);
	}

	sl_si91x_gpio_set_pin_direction(cfg->hal_port, pin, (flags & GPIO_OUTPUT) ? 0 : 1);

	return 0;
}

static int gpio_siwx91x_port_get(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;

	*value = sl_gpio_get_port_input(cfg->hal_port);

	return 0;
}

static int gpio_siwx91x_port_set_masked(const struct device *port, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;
	const struct device *parent = cfg->parent;
	const struct gpio_siwx91x_common_config *pcfg = parent->config;

	/* Cannot use HAL function sl_gpio_set_port_output_value(), as it doesn't clear bits. */
	pcfg->reg->PORT_CONFIG[cfg->port].PORT_LOAD_REG =
		(pcfg->reg->PORT_CONFIG[cfg->port].PORT_LOAD_REG & ~mask) | (value & mask);

	return 0;
}

static int gpio_siwx91x_port_set_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;

	sl_gpio_set_port_output(cfg->hal_port, pins);

	return 0;
}

static int gpio_siwx91x_port_clear_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;

	sl_gpio_clear_port_output(cfg->hal_port, pins);

	return 0;
}

static int gpio_siwx91x_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;

	sl_gpio_toggle_port_output(cfg->hal_port, pins);

	return 0;
}

static bool receiver_enabled(bool ulp, sl_gpio_port_t port, int pin)
{
	if (ulp) {
		return ULP_PAD_CONFIG_REG & BIT(pin);
	} else {
		return PAD_REG(((port << 4) | pin))->GPIO_PAD_CONFIG_REG_b.PADCONFIG_REN;
	}
}

int gpio_siwx91x_port_get_direction(const struct device *port, gpio_port_pins_t map,
				    gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;

	if (inputs != NULL) {
		*inputs = 0;
	}
	if (outputs != NULL) {
		*outputs = 0;
	}
	for (int i = 0; i < MAX_PIN_COUNT; i++) {
		if ((map & BIT(i))) {
			if (sl_si91x_gpio_get_pin_direction(cfg->hal_port, i) == 0) {
				if (outputs != NULL) {
					*outputs |= BIT(i);
				}
			}
			if (receiver_enabled(cfg->ulp, cfg->port, i)) {
				if (inputs != NULL) {
					*inputs |= BIT(i);
				}
			}
		}
	}
	return 0;
}

static int gpio_siwx91x_manage_callback(const struct device *port, struct gpio_callback *callback,
					bool set)
{
	struct gpio_siwx91x_port_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_siwx91x_interrupt_configure(const struct device *port, gpio_pin_t pin,
					    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;
	const struct device *parent = cfg->parent;
	const struct gpio_siwx91x_common_config *pcfg = parent->config;
	struct gpio_siwx91x_common_data *data = parent->data;
	sl_si91x_gpio_interrupt_config_flag_t flags = 0;

	if (mode & GPIO_INT_DISABLE) {
		ARRAY_FOR_EACH(data->interrupts, i) {
			if (data->interrupts[i].port == cfg->port &&
			    data->interrupts[i].pin == pin) {
				data->interrupts[i].port = INVALID_PORT;
				if (cfg->ulp) {
					sl_si91x_gpio_configure_ulp_pin_interrupt(i, flags, pin);
				} else {
					sl_gpio_configure_interrupt(cfg->port, pin, i, flags);
				}
				/* Configure function doesn't mask interrupts when disabling */
				pcfg->reg->INTR[i].GPIO_INTR_CTRL_b.MASK = 1;
				if (cfg->ulp) {
					sl_si91x_gpio_clear_ulp_interrupt(i);
				} else {
					sl_gpio_clear_interrupts(i);
				}
				return 0;
			}
		}
		/* No interrupt to disable, but this is not an error */
		return 0;
	}

	if (trig == GPIO_INT_TRIG_LOW) {
		flags = (mode == GPIO_INT_MODE_EDGE) ? SL_GPIO_INTERRUPT_FALL_EDGE
						     : SL_GPIO_INTERRUPT_LEVEL_LOW;
	} else if (trig == GPIO_INT_TRIG_HIGH) {
		flags = (mode == GPIO_INT_MODE_EDGE) ? SL_GPIO_INTERRUPT_RISE_EDGE
						     : SL_GPIO_INTERRUPT_LEVEL_HIGH;
	} else if (trig == GPIO_INT_TRIG_BOTH) {
		/* SL_GPIO_INTERRUPT_RISE_FALL_EDGE would make more sense, but HAL
		 * implementation is buggy.
		 */
		flags = SL_GPIO_INTERRUPT_RISE_EDGE | SL_GPIO_INTERRUPT_FALL_EDGE;
	}

	ARRAY_FOR_EACH(data->interrupts, i) {
		if (data->interrupts[i].port == INVALID_PORT ||
		    (data->interrupts[i].port == cfg->port && data->interrupts[i].pin == pin)) {
			data->interrupts[i].port = cfg->port;
			data->interrupts[i].pin = pin;

			if (cfg->ulp) {
				sl_si91x_gpio_configure_ulp_pin_interrupt(i, flags, pin);
			} else {
				sl_gpio_configure_interrupt(cfg->port, pin, i, flags);
			}
			return 0;
		}
	}
	/* No more available interrupts */
	return -EBUSY;
}

static inline int gpio_siwx91x_init_port(const struct device *port)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;
	const struct device *parent = cfg->parent;
	struct gpio_siwx91x_common_data *data = parent->data;

	/* Register port as active */
	__ASSERT(cfg->port < MAX_PORT_COUNT, "Too many ports");
	data->ports[cfg->port] = port;

	return 0;
}

static void gpio_siwx91x_isr(const struct device *parent)
{
	const struct gpio_siwx91x_common_config *pcfg = parent->config;
	struct gpio_siwx91x_common_data *common = parent->data;
	const struct device *port;
	struct gpio_siwx91x_port_data *data;

	ARRAY_FOR_EACH(common->interrupts, i) {
		sl_gpio_port_t port_no = common->interrupts[i].port;
		uint32_t pending = pcfg->reg->INTR[i].GPIO_INTR_STATUS_b.INTERRUPT_STATUS;

		if (pending && port_no != INVALID_PORT) {
			/* Clear interrupt */
			pcfg->reg->INTR[i].GPIO_INTR_STATUS_b.INTERRUPT_STATUS = 1;
			port = common->ports[port_no];
			data = port->data;
			gpio_fire_callbacks(&data->callbacks, port, BIT(common->interrupts[i].pin));
		}
	}
}

static uint32_t gpio_siwx91x_get_pending_int(const struct device *port)
{
	const struct gpio_siwx91x_port_config *cfg = port->config;
	const struct device *parent = cfg->parent;
	const struct gpio_siwx91x_common_config *pcfg = parent->config;
	uint32_t status = 0;

	ARRAY_FOR_EACH(pcfg->reg->INTR, i) {
		if (pcfg->reg->INTR[i].GPIO_INTR_STATUS_b.INTERRUPT_STATUS) {
			status |= BIT(i);
		}
	}
	return status;
}

static DEVICE_API(gpio, gpio_siwx91x_api) = {
	.pin_configure = gpio_siwx91x_pin_configure,
	.port_get_raw = gpio_siwx91x_port_get,
	.port_set_masked_raw = gpio_siwx91x_port_set_masked,
	.port_set_bits_raw = gpio_siwx91x_port_set_bits,
	.port_clear_bits_raw = gpio_siwx91x_port_clear_bits,
	.port_toggle_bits = gpio_siwx91x_port_toggle_bits,
	.pin_interrupt_configure = gpio_siwx91x_interrupt_configure,
	.manage_callback = gpio_siwx91x_manage_callback,
	.get_pending_int = gpio_siwx91x_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_siwx91x_port_get_direction,
#endif
};

#define GPIO_PORT_INIT(n)                                                                          \
	static const struct gpio_siwx91x_port_config gpio_siwx91x_port_config##n = {               \
		.common.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(n),                        \
		.parent = DEVICE_DT_GET(DT_PARENT(n)),                                             \
		.pads = DT_PROP(n, silabs_pads),                                                   \
		.port = DT_REG_ADDR(n),                                                            \
		.hal_port = (DT_PROP(DT_PARENT(n), silabs_ulp) ? SL_GPIO_ULP_PORT : 0) +           \
			    DT_REG_ADDR(n),                                                        \
		.ulp = DT_PROP(DT_PARENT(n), silabs_ulp),                                          \
	};                                                                                         \
	static struct gpio_siwx91x_port_data gpio_siwx91x_port_data##n;                            \
                                                                                                   \
	DEVICE_DT_DEFINE(n, gpio_siwx91x_init_port, NULL, &gpio_siwx91x_port_data##n,              \
			 &gpio_siwx91x_port_config##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,    \
			 &gpio_siwx91x_api);

#define CONFIGURE_SHARED_INTERRUPT(node_id, prop, idx)                                             \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq), DT_IRQ_BY_IDX(node_id, idx, priority),       \
		    gpio_siwx91x_isr, DEVICE_DT_GET(node_id), 0);                                  \
	irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));

static DEVICE_API(gpio, gpio_siwx91x_common_api) = { };

#define GPIO_CONTROLLER_INIT(idx)                                                                  \
	static const struct gpio_siwx91x_common_config gpio_siwx91x_config##idx = {                \
		.reg = (EGPIO_Type *)DT_INST_REG_ADDR(idx),                                        \
	};                                                                                         \
	static struct gpio_siwx91x_common_data gpio_siwx91x_data##idx;                             \
                                                                                                   \
	static int gpio_siwx91x_init_controller_##idx(const struct device *dev)                    \
	{                                                                                          \
		struct gpio_siwx91x_common_data *data = dev->data;                                 \
		int status;                                                                        \
                                                                                                   \
		status = sl_si91x_gpio_driver_enable_clock(                                        \
			COND_CODE_1(DT_INST_PROP(idx, silabs_ulp), (ULPCLK_GPIO), (M4CLK_GPIO)));  \
		if (status) {                                                                      \
			return -ENODEV;                                                            \
		}                                                                                  \
		ARRAY_FOR_EACH(data->interrupts, i) {                                              \
			data->interrupts[i].port = INVALID_PORT;                                   \
		}                                                                                  \
		DT_INST_FOREACH_PROP_ELEM(idx, interrupt_names, CONFIGURE_SHARED_INTERRUPT);       \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, gpio_siwx91x_init_controller_##idx, NULL,                       \
			      &gpio_siwx91x_data##idx, &gpio_siwx91x_config##idx,                  \
			      PRE_KERNEL_1, CONFIG_GPIO_SILABS_SIWX91X_COMMON_INIT_PRIORITY,       \
			      &gpio_siwx91x_common_api);                                           \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, GPIO_PORT_INIT);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CONTROLLER_INIT)
