/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_gpio

#include <nrfx_gpiote.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_gpio_pad_group)
#define GPIO_HAS_PAD_GROUP 1
#else
#define GPIO_HAS_PAD_GROUP 0
#endif

#define GPIOTE_PHANDLE(id) DT_INST_PHANDLE(id, gpiote_instance)
#define GPIOTE_PROP(idx, prop)     DT_PROP(GPIOTE(idx), prop)

#define IS_NO_PORT_INSTANCE(id) DT_PROP_OR(GPIOTE_PHANDLE(id), no_port_event, 0) ||
#define IS_FIXED_CH_INSTANCE(id) DT_PROP_OR(GPIOTE_PHANDLE(id), fixed_channels_supported, 0) ||

#if DT_INST_FOREACH_STATUS_OKAY(IS_NO_PORT_INSTANCE) 0
#define GPIOTE_NO_PORT_EVT_SUPPORT 1
#endif

#if DT_INST_FOREACH_STATUS_OKAY(IS_FIXED_CH_INSTANCE) 0
#define GPIOTE_FIXED_CH_SUPPORT 1
#endif

#if defined(GPIOTE_NO_PORT_EVT_SUPPORT) || defined(GPIOTE_FIXED_CH_SUPPORT)
#define GPIOTE_FEATURE_FLAG 1
#define GPIOTE_FLAG_NO_PORT_EVT BIT(0)
#define GPIOTE_FLAG_FIXED_CHAN  BIT(1)
#endif

struct gpio_nrfx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

struct gpio_nrfx_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	NRF_GPIO_Type *port;
	uint32_t edge_sense;
	uint8_t port_num;
	nrfx_gpiote_t gpiote;
#if GPIO_HAS_PAD_GROUP
	const struct device *pad_group;
#endif
#if defined(GPIOTE_FEATURE_FLAG)
	uint32_t flags;
#endif
};

static inline struct gpio_nrfx_data *get_port_data(const struct device *port)
{
	return port->data;
}

static inline const struct gpio_nrfx_cfg *get_port_cfg(const struct device *port)
{
	return port->config;
}

static bool has_gpiote(const struct gpio_nrfx_cfg *cfg)
{
	return cfg->gpiote.p_reg != NULL;
}

static nrf_gpio_pin_pull_t get_pull(gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		return NRF_GPIO_PIN_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		return NRF_GPIO_PIN_PULLDOWN;
	}

	return NRF_GPIO_PIN_NOPULL;
}

static int gpio_nrfx_pin_configure(const struct device *port, gpio_pin_t pin,
				   gpio_flags_t flags)
{
	int ret = 0;
	nrfx_err_t err = NRFX_SUCCESS;
	uint8_t ch;
	bool free_ch = false;
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	nrfx_gpiote_pin_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
	nrf_gpio_pin_pull_t pull = get_pull(flags);
	nrf_gpio_pin_drive_t drive;
	int pm_ret;

	switch (flags & (NRF_GPIO_DRIVE_MSK | GPIO_OPEN_DRAIN)) {
	case NRF_GPIO_DRIVE_S0S1:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case NRF_GPIO_DRIVE_S0H1:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case NRF_GPIO_DRIVE_H0S1:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case NRF_GPIO_DRIVE_H0H1:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case NRF_GPIO_DRIVE_S0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_S0D1;
		break;
	case NRF_GPIO_DRIVE_H0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_H0D1;
		break;
	case NRF_GPIO_DRIVE_S1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case NRF_GPIO_DRIVE_H1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0H1;
		break;
	default:
		return -EINVAL;
	}

	ret = pm_device_runtime_get(port);
	if (ret < 0) {
		return ret;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		nrf_gpio_port_out_set(cfg->port, BIT(pin));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		nrf_gpio_port_out_clear(cfg->port, BIT(pin));
	}

	if (!has_gpiote(cfg)) {
		nrf_gpio_pin_dir_t dir = (flags & GPIO_OUTPUT)
				       ? NRF_GPIO_PIN_DIR_OUTPUT
				       : NRF_GPIO_PIN_DIR_INPUT;
		nrf_gpio_pin_input_t input = (flags & GPIO_INPUT)
					   ? NRF_GPIO_PIN_INPUT_CONNECT
					   : NRF_GPIO_PIN_INPUT_DISCONNECT;

		nrf_gpio_reconfigure(abs_pin, &dir, &input, &pull, &drive, NULL);

		goto end;
	}

	/* Get the GPIOTE channel associated with this pin, if any. It needs
	 * to be freed when the pin is reconfigured or disconnected.
	 */
	if (IS_ENABLED(CONFIG_GPIO_NRFX_INTERRUPT)) {
		err = nrfx_gpiote_channel_get(&cfg->gpiote, abs_pin, &ch);
		free_ch = (err == NRFX_SUCCESS);
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		/* Ignore the error code. The pin may not have been used. */
		(void)nrfx_gpiote_pin_uninit(&cfg->gpiote, abs_pin);
	} else {
		/* Remove previously configured trigger when pin is reconfigured. */
		if (IS_ENABLED(CONFIG_GPIO_NRFX_INTERRUPT)) {
			nrfx_gpiote_trigger_config_t trigger_config = {
				.trigger = NRFX_GPIOTE_TRIGGER_NONE,
			};
			nrfx_gpiote_input_pin_config_t input_pin_config = {
				.p_trigger_config = &trigger_config,
			};

			err = nrfx_gpiote_input_configure(&cfg->gpiote,
				abs_pin, &input_pin_config);
			if (err != NRFX_SUCCESS) {
				ret = -EINVAL;

				goto end;
			}
		}

		if (flags & GPIO_OUTPUT) {
			nrfx_gpiote_output_config_t output_config = {
				.drive = drive,
				.input_connect = (flags & GPIO_INPUT)
					       ? NRF_GPIO_PIN_INPUT_CONNECT
					       : NRF_GPIO_PIN_INPUT_DISCONNECT,
				.pull = pull,
			};

			err = nrfx_gpiote_output_configure(&cfg->gpiote,
				abs_pin, &output_config, NULL);
		} else {
			nrfx_gpiote_input_pin_config_t input_pin_config = {
				.p_pull_config = &pull,
			};

			err = nrfx_gpiote_input_configure(&cfg->gpiote,
				abs_pin, &input_pin_config);
		}

		if (err != NRFX_SUCCESS) {
			ret = -EINVAL;
			goto end;
		}
	}

	if (IS_ENABLED(CONFIG_GPIO_NRFX_INTERRUPT) && free_ch) {
#ifdef GPIOTE_FEATURE_FLAG
		/* Fixed channel was used, no need to free. */
		if (cfg->flags & GPIOTE_FLAG_FIXED_CHAN) {
			goto end;
		}
#endif
		err = nrfx_gpiote_channel_free(&cfg->gpiote, ch);
		__ASSERT_NO_MSG(err == NRFX_SUCCESS);
	}

end:
	pm_ret = pm_device_runtime_put(port);

	return (ret != 0) ? ret : pm_ret;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_nrfx_pin_get_config(const struct device *port, gpio_pin_t pin,
				    gpio_flags_t *flags)
{
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	nrfx_gpiote_pin_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);

	*flags = 0U;

	nrf_gpio_pin_dir_t dir = nrf_gpio_pin_dir_get(abs_pin);

	if (dir == NRF_GPIO_PIN_DIR_OUTPUT) {
		*flags |= GPIO_OUTPUT;
		*flags |= nrf_gpio_pin_out_read(abs_pin)
			? GPIO_OUTPUT_INIT_HIGH
			: GPIO_OUTPUT_INIT_LOW;
	}

	nrf_gpio_pin_input_t input = nrf_gpio_pin_input_get(abs_pin);

	if (input == NRF_GPIO_PIN_INPUT_CONNECT) {
		*flags |= GPIO_INPUT;
	}

	nrf_gpio_pin_pull_t pull = nrf_gpio_pin_pull_get(abs_pin);

	switch (pull) {
	case NRF_GPIO_PIN_PULLUP:
		*flags |= GPIO_PULL_UP;
		break;
	case NRF_GPIO_PIN_PULLDOWN:
		*flags |= GPIO_PULL_DOWN;
		break;
	default:
		break;
	}

	nrf_gpio_pin_drive_t drive = nrf_gpio_pin_drive_get(abs_pin);

	switch (drive) {
	case NRF_GPIO_PIN_S0S1:
		*flags |= NRF_GPIO_DRIVE_S0S1;
		break;
	case NRF_GPIO_PIN_S0H1:
		*flags |= NRF_GPIO_DRIVE_S0H1;
		break;
	case NRF_GPIO_PIN_H0S1:
		*flags |= NRF_GPIO_DRIVE_H0S1;
		break;
	case NRF_GPIO_PIN_H0H1:
		*flags |= NRF_GPIO_DRIVE_H0H1;
		break;
	case NRF_GPIO_PIN_S0D1:
		*flags |= NRF_GPIO_DRIVE_S0 | GPIO_OPEN_DRAIN;
		break;
	case NRF_GPIO_PIN_H0D1:
		*flags |= NRF_GPIO_DRIVE_H0 | GPIO_OPEN_DRAIN;
		break;
	case NRF_GPIO_PIN_D0S1:
		*flags |= NRF_GPIO_DRIVE_S1 | GPIO_OPEN_SOURCE;
		break;
	case NRF_GPIO_PIN_D0H1:
		*flags |= NRF_GPIO_DRIVE_H1 | GPIO_OPEN_SOURCE;
		break;
	default:
		break;
	}

	return 0;
}
#endif

static int gpio_nrfx_port_get_raw(const struct device *port,
				  gpio_port_value_t *value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	*value = nrf_gpio_port_in_read(reg);

	return 0;
}

static int gpio_nrfx_port_set_masked_raw(const struct device *port,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	int ret;

	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~set_mask) & mask;

	ret = pm_device_runtime_get(port);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_set(reg, set_mask);
	nrf_gpio_port_out_clear(reg, clear_mask);
	return pm_device_runtime_put(port);
}

static int gpio_nrfx_port_set_bits_raw(const struct device *port,
				       gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	int ret;

	ret = pm_device_runtime_get(port);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_set(reg, mask);
	return pm_device_runtime_put(port);
}

static int gpio_nrfx_port_clear_bits_raw(const struct device *port,
					 gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	int ret;

	ret = pm_device_runtime_get(port);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_clear(reg, mask);
	return pm_device_runtime_put(port);
}

static int gpio_nrfx_port_toggle_bits(const struct device *port,
				      gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	const uint32_t value = nrf_gpio_port_out_read(reg) ^ mask;
	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~value) & mask;
	int ret;

	ret = pm_device_runtime_get(port);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_set(reg, set_mask);
	nrf_gpio_port_out_clear(reg, clear_mask);
	return pm_device_runtime_put(port);
}

#ifdef CONFIG_GPIO_NRFX_INTERRUPT
static nrfx_gpiote_trigger_t get_trigger(enum gpio_int_mode mode,
					 enum gpio_int_trig trig)
{
	if (mode == GPIO_INT_MODE_LEVEL) {
		return trig == GPIO_INT_TRIG_LOW ? NRFX_GPIOTE_TRIGGER_LOW :
						   NRFX_GPIOTE_TRIGGER_HIGH;
	}

	return trig == GPIO_INT_TRIG_BOTH ? NRFX_GPIOTE_TRIGGER_TOGGLE :
	       trig == GPIO_INT_TRIG_LOW  ? NRFX_GPIOTE_TRIGGER_HITOLO :
					    NRFX_GPIOTE_TRIGGER_LOTOHI;
}

static nrfx_err_t chan_alloc(const struct gpio_nrfx_cfg *cfg, gpio_pin_t pin, uint8_t *ch)
{
#ifdef GPIOTE_FEATURE_FLAG
	if (cfg->flags & GPIOTE_FLAG_FIXED_CHAN) {
		/* Currently fixed channel relation is only present in one instance (GPIOTE0 on
		 * cpurad). The rules are following:
		 * - GPIOTE0 can only be used with P1 (pins 4-11) and P2 (pins (0-11))
		 * - P1: channel => pin - 4, e.g. P1.4 => channel 0, P1.5 => channel 1
		 * - P2: channel => pin % 8, e.g. P2.0 => channel 0, P2.8 => channel 0
		 */
		nrfx_err_t err = NRFX_SUCCESS;

		if (cfg->port_num == 1) {
			if (pin < 4) {
				err = NRFX_ERROR_INVALID_PARAM;
			} else {
				*ch = pin - 4;
			}
		} else if (cfg->port_num == 2) {
			*ch = pin & 0x7;
		} else {
			err = NRFX_ERROR_INVALID_PARAM;
		}

		return err;
	}
#endif

	return nrfx_gpiote_channel_alloc(&cfg->gpiote, ch);
}

static int gpio_nrfx_pin_interrupt_configure(const struct device *port,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
	nrfx_err_t err;
	uint8_t ch;

	if (!has_gpiote(cfg)) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		nrfx_gpiote_trigger_disable(&cfg->gpiote, abs_pin);

		return 0;
	}

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = get_trigger(mode, trig),
	};
	nrfx_gpiote_input_pin_config_t input_pin_config = {
		.p_trigger_config = &trigger_config,
	};

	/* If edge mode is to be used and pin is not configured to use sense for
	 * edge use IN event.
	 */
	if (!(BIT(pin) & cfg->edge_sense) &&
	    (mode == GPIO_INT_MODE_EDGE) &&
	    (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_INPUT)) {
		err = nrfx_gpiote_channel_get(&cfg->gpiote, abs_pin, &ch);
		if (err == NRFX_ERROR_INVALID_PARAM) {
			err = chan_alloc(cfg, pin, &ch);
			if (err != NRFX_SUCCESS) {
				return -ENOMEM;
			}
		}

		trigger_config.p_in_channel = &ch;
	} else {
#ifdef GPIOTE_FEATURE_FLAG
		if (cfg->flags & GPIOTE_FLAG_NO_PORT_EVT) {
			return -ENOTSUP;
		}
#endif
		/* If edge mode with channel was previously used and we are changing to sense or
		 * level triggered, we must free the channel.
		 */
		err = nrfx_gpiote_channel_get(&cfg->gpiote, abs_pin, &ch);
		if (err == NRFX_SUCCESS) {
			err = nrfx_gpiote_channel_free(&cfg->gpiote, ch);
			__ASSERT_NO_MSG(err == NRFX_SUCCESS);
		}
	}

	err = nrfx_gpiote_input_configure(&cfg->gpiote, abs_pin, &input_pin_config);
	if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	nrfx_gpiote_trigger_enable(&cfg->gpiote, abs_pin, true);

	return 0;
}

static int gpio_nrfx_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&get_port_data(port)->callbacks,
				     callback, set);
}
#endif /* CONFIG_GPIO_NRFX_INTERRUPT */

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_nrfx_port_get_direction(const struct device *port,
					gpio_port_pins_t map,
					gpio_port_pins_t *inputs,
					gpio_port_pins_t *outputs)
{
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	NRF_GPIO_Type *reg = cfg->port;

	map &= cfg->common.port_pin_mask;

	if (outputs != NULL) {
		*outputs = map & nrf_gpio_port_dir_read(cfg->port);
	}

	if (inputs != NULL) {
		*inputs = 0;
		while (map) {
			uint32_t pin = NRF_CTZ(map);
			uint32_t pin_cnf = reg->PIN_CNF[pin];

			/* Check if the pin has its input buffer connected. */
			if (((pin_cnf & GPIO_PIN_CNF_INPUT_Msk) >>
			     GPIO_PIN_CNF_INPUT_Pos) ==
			    GPIO_PIN_CNF_INPUT_Connect) {
				*inputs |= BIT(pin);
			}

			map &= ~BIT(pin);
		}
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

#ifdef CONFIG_GPIO_NRFX_INTERRUPT
/* Get port device from port id. */
static const struct device *get_dev(uint32_t port_id)
{
	const struct device *dev = NULL;

	#define GPIO_NRF_GET_DEV(i) \
		else if (DT_INST_PROP(i, port) == port_id) { \
			dev = DEVICE_DT_INST_GET(i); \
		}

	if (0) {
	} /* Followed by else if from FOREACH macro. Done to avoid return statement in macro.  */
	DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_GET_DEV)
	#undef GPIO_NRF_GET_DEV

	return dev;
}

static void nrfx_gpio_handler(nrfx_gpiote_pin_t abs_pin,
			      nrfx_gpiote_trigger_t trigger,
			      void *context)
{
	uint32_t pin = abs_pin;
	uint32_t port_id = nrf_gpio_pin_port_number_extract(&pin);
	const struct device *port = get_dev(port_id);

	/* If given port is handled directly by nrfx driver it might not be enabled in DT. */
	if (port == NULL) {
		return;
	}

	struct gpio_nrfx_data *data = get_port_data(port);
	sys_slist_t *list = &data->callbacks;

	gpio_fire_callbacks(list, port, BIT(pin));
}
#endif /* CONFIG_GPIO_NRFX_INTERRUPT */

#define GPIOTE_IRQ_HANDLER_CONNECT(node_id) \
	IRQ_CONNECT(DT_IRQN(node_id), DT_IRQ(node_id, priority), nrfx_isr, \
		    NRFX_CONCAT(nrfx_gpiote_, DT_PROP(node_id, instance), _irq_handler), 0);

static int gpio_nrfx_pm_suspend(const struct device *port)
{
#if GPIO_HAS_PAD_GROUP
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);

	return pm_device_runtime_put(cfg->pad_group);
#else
	ARG_UNUSED(port);
	return 0;
#endif
}

static int gpio_nrfx_pm_resume(const struct device *port)
{
#if GPIO_HAS_PAD_GROUP
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);

	return pm_device_runtime_get(cfg->pad_group);
#else
	ARG_UNUSED(port);
	return 0;
#endif
}

static int gpio_nrfx_pm_hook(const struct device *port, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_nrfx_pm_suspend(port);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_nrfx_pm_resume(port);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int gpio_nrfx_init(const struct device *port)
{
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	nrfx_err_t err;

	if (!has_gpiote(cfg)) {
		goto pm_init;
	}

	if (nrfx_gpiote_init_check(&cfg->gpiote)) {
		goto pm_init;
	}

	err = nrfx_gpiote_init(&cfg->gpiote, 0 /*not used*/);
	if (err != NRFX_SUCCESS) {
		return -EIO;
	}

#ifdef CONFIG_GPIO_NRFX_INTERRUPT
	nrfx_gpiote_global_callback_set(&cfg->gpiote, nrfx_gpio_handler, NULL);
	DT_FOREACH_STATUS_OKAY(nordic_nrf_gpiote, GPIOTE_IRQ_HANDLER_CONNECT);
#endif /* CONFIG_GPIO_NRFX_INTERRUPT */

pm_init:
	return pm_device_driver_init(port, gpio_nrfx_pm_hook);
}

static DEVICE_API(gpio, gpio_nrfx_drv_api_funcs) = {
	.pin_configure = gpio_nrfx_pin_configure,
	.port_get_raw = gpio_nrfx_port_get_raw,
	.port_set_masked_raw = gpio_nrfx_port_set_masked_raw,
	.port_set_bits_raw = gpio_nrfx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nrfx_port_clear_bits_raw,
	.port_toggle_bits = gpio_nrfx_port_toggle_bits,
#ifdef CONFIG_GPIO_NRFX_INTERRUPT
	.pin_interrupt_configure = gpio_nrfx_pin_interrupt_configure,
	.manage_callback = gpio_nrfx_manage_callback,
#endif
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_nrfx_port_get_direction,
#endif
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_nrfx_pin_get_config,
#endif
};

#define GPIOTE_INST(id)    DT_PROP(GPIOTE_PHANDLE(id), instance)

#define GPIOTE_INSTANCE(id)                                     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(id, gpiote_instance), \
		    (NRFX_GPIOTE_INSTANCE(GPIOTE_INST(id))),    \
		    ({ .p_reg = NULL }))

/* Device instantiation is done with node labels because 'port_num' is
 * the peripheral number by SoC numbering. We therefore cannot use
 * DT_INST APIs here without wider changes.
 */

#define GPIOTE_CHECK(id)						       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(id, gpiote_instance),		       \
		(BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GPIOTE_PHANDLE(id)),    \
			"Please enable GPIOTE instance for used GPIO port!")), \
		())

#if GPIO_HAS_PAD_GROUP
#define GPIO_NRF_PAD_GROUP_INIT(id) \
	.pad_group = DEVICE_DT_GET(DT_INST_CHILD(id, pad_group)),
#else
#define GPIO_NRF_PAD_GROUP_INIT(id)
#endif

#define GPIO_NRF_DEVICE(id)								\
	GPIOTE_CHECK(id);								\
	static const struct gpio_nrfx_cfg gpio_nrfx_p##id##_cfg = {			\
		.common = {								\
			.port_pin_mask =						\
			GPIO_PORT_PIN_MASK_FROM_DT_INST(id),				\
		},									\
		.port = _CONCAT(NRF_P, DT_INST_PROP(id, port)),				\
		.port_num = DT_INST_PROP(id, port),					\
		.edge_sense = DT_INST_PROP_OR(id, sense_edge_mask, 0),			\
		.gpiote = GPIOTE_INSTANCE(id),						\
		GPIO_NRF_PAD_GROUP_INIT(id)						\
		IF_ENABLED(GPIOTE_FEATURE_FLAG,						\
			(.flags =							\
			 (DT_PROP_OR(GPIOTE_PHANDLE(id), no_port_event, 0) ?		\
			    GPIOTE_FLAG_NO_PORT_EVT : 0) |				\
			 (DT_PROP_OR(GPIOTE_PHANDLE(id), fixed_channels_supported, 0) ?	\
			  GPIOTE_FLAG_FIXED_CHAN : 0),)					\
			)								\
	};										\
											\
	static struct gpio_nrfx_data gpio_nrfx_p##id##_data;				\
											\
	PM_DEVICE_DT_INST_DEFINE(id, gpio_nrfx_pm_hook);				\
											\
	DEVICE_DT_INST_DEFINE(id, gpio_nrfx_init,					\
			 PM_DEVICE_DT_INST_GET(id),					\
			 &gpio_nrfx_p##id##_data,					\
			 &gpio_nrfx_p##id##_cfg,					\
			 PRE_KERNEL_1,							\
			 CONFIG_GPIO_INIT_PRIORITY,					\
			 &gpio_nrfx_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_DEVICE)
