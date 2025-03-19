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

#include <zephyr/drivers/gpio/gpio_utils.h>

#ifdef CONFIG_SOC_NRF54H20_GPD
#include <nrf/gpd.h>
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
#ifdef CONFIG_SOC_NRF54H20_GPD
	uint8_t pad_pd;
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

static int gpio_nrfx_gpd_retain_set(const struct device *port, uint32_t mask, gpio_flags_t flags)
{
#ifdef CONFIG_SOC_NRF54H20_GPD
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);

	if (cfg->pad_pd == NRF_GPD_FAST_ACTIVE1) {
		int ret;

		if (flags & GPIO_OUTPUT) {
			nrf_gpio_port_retain_enable(cfg->port, mask);
		}

		ret = nrf_gpd_release(NRF_GPD_FAST_ACTIVE1);
		if (ret < 0) {
			return ret;
		}
	}
#else
	ARG_UNUSED(port);
	ARG_UNUSED(mask);
	ARG_UNUSED(flags);
#endif

	return 0;
}

static int gpio_nrfx_gpd_retain_clear(const struct device *port, uint32_t mask)
{
#ifdef CONFIG_SOC_NRF54H20_GPD
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);

	if (cfg->pad_pd == NRF_GPD_FAST_ACTIVE1) {
		int ret;

		ret = nrf_gpd_request(NRF_GPD_FAST_ACTIVE1);
		if (ret < 0) {
			return ret;
		}

		nrf_gpio_port_retain_disable(cfg->port, mask);
	}
#else
	ARG_UNUSED(port);
	ARG_UNUSED(mask);
#endif

	return 0;
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

	ret = gpio_nrfx_gpd_retain_clear(port, BIT(pin));
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
		err = nrfx_gpiote_channel_free(&cfg->gpiote, ch);
		__ASSERT_NO_MSG(err == NRFX_SUCCESS);
	}

end:
	(void)gpio_nrfx_gpd_retain_set(port, BIT(pin), flags);
	return ret;
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
	}

	nrf_gpio_pin_input_t input = nrf_gpio_pin_input_get(abs_pin);

	if (input == NRF_GPIO_PIN_INPUT_CONNECT) {
		*flags |= GPIO_INPUT;
	}

	nrf_gpio_pin_pull_t pull = nrf_gpio_pin_pull_get(abs_pin);

	switch (pull) {
	case NRF_GPIO_PIN_PULLUP:
		*flags |= GPIO_PULL_UP;
	case NRF_GPIO_PIN_PULLDOWN:
		*flags |= GPIO_PULL_DOWN;
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

	ret = gpio_nrfx_gpd_retain_clear(port, mask);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_set(reg, set_mask);
	nrf_gpio_port_out_clear(reg, clear_mask);

	return gpio_nrfx_gpd_retain_set(port, mask, GPIO_OUTPUT);
}

static int gpio_nrfx_port_set_bits_raw(const struct device *port,
				       gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	int ret;

	ret = gpio_nrfx_gpd_retain_clear(port, mask);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_set(reg, mask);

	return gpio_nrfx_gpd_retain_set(port, mask, GPIO_OUTPUT);
}

static int gpio_nrfx_port_clear_bits_raw(const struct device *port,
					 gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	int ret;

	ret = gpio_nrfx_gpd_retain_clear(port, mask);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_clear(reg, mask);

	return gpio_nrfx_gpd_retain_set(port, mask, GPIO_OUTPUT);
}

static int gpio_nrfx_port_toggle_bits(const struct device *port,
				      gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	const uint32_t value = nrf_gpio_port_out_read(reg) ^ mask;
	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~value) & mask;
	int ret;

	ret = gpio_nrfx_gpd_retain_clear(port, mask);
	if (ret < 0) {
		return ret;
	}

	nrf_gpio_port_out_set(reg, set_mask);
	nrf_gpio_port_out_clear(reg, clear_mask);

	return gpio_nrfx_gpd_retain_set(port, mask, GPIO_OUTPUT);
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
			err = nrfx_gpiote_channel_alloc(&cfg->gpiote, &ch);
			if (err != NRFX_SUCCESS) {
				return -ENOMEM;
			}
		}

		trigger_config.p_in_channel = &ch;
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

static int gpio_nrfx_init(const struct device *port)
{
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	nrfx_err_t err;

	if (!has_gpiote(cfg)) {
		return 0;
	}

	if (nrfx_gpiote_init_check(&cfg->gpiote)) {
		return 0;
	}

	err = nrfx_gpiote_init(&cfg->gpiote, 0 /*not used*/);
	if (err != NRFX_SUCCESS) {
		return -EIO;
	}

#ifdef CONFIG_GPIO_NRFX_INTERRUPT
	nrfx_gpiote_global_callback_set(&cfg->gpiote, nrfx_gpio_handler, NULL);
	DT_FOREACH_STATUS_OKAY(nordic_nrf_gpiote, GPIOTE_IRQ_HANDLER_CONNECT);
#endif /* CONFIG_GPIO_NRFX_INTERRUPT */

	return 0;
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

#define GPIOTE_PHANDLE(id) DT_INST_PHANDLE(id, gpiote_instance)
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

#ifdef CONFIG_SOC_NRF54H20_GPD
#define PAD_PD(inst) \
	.pad_pd = DT_INST_PHA_BY_NAME_OR(inst, power_domains, pad, id,	       \
					 NRF_GPD_SLOW_MAIN),
#else
#define PAD_PD(inst)
#endif

#define GPIO_NRF_DEVICE(id)						\
	GPIOTE_CHECK(id);						\
	static const struct gpio_nrfx_cfg gpio_nrfx_p##id##_cfg = {	\
		.common = {						\
			.port_pin_mask =				\
			GPIO_PORT_PIN_MASK_FROM_DT_INST(id),		\
		},							\
		.port = _CONCAT(NRF_P, DT_INST_PROP(id, port)),		\
		.port_num = DT_INST_PROP(id, port),			\
		.edge_sense = DT_INST_PROP_OR(id, sense_edge_mask, 0),	\
		.gpiote = GPIOTE_INSTANCE(id),				\
		PAD_PD(id)						\
	};								\
									\
	static struct gpio_nrfx_data gpio_nrfx_p##id##_data;		\
									\
	DEVICE_DT_INST_DEFINE(id, gpio_nrfx_init,			\
			 NULL,						\
			 &gpio_nrfx_p##id##_data,			\
			 &gpio_nrfx_p##id##_cfg,			\
			 PRE_KERNEL_1,					\
			 CONFIG_GPIO_INIT_PRIORITY,			\
			 &gpio_nrfx_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_DEVICE)
