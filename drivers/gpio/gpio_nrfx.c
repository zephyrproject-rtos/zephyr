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
};

static inline struct gpio_nrfx_data *get_port_data(const struct device *port)
{
	return port->data;
}

static inline const struct gpio_nrfx_cfg *get_port_cfg(const struct device *port)
{
	return port->config;
}

static int get_drive(gpio_flags_t flags, nrf_gpio_pin_drive_t *drive)
{
	switch (flags & (NRF_GPIO_DRIVE_MSK | GPIO_OPEN_DRAIN)) {
	case NRF_GPIO_DRIVE_S0S1:
		*drive = NRF_GPIO_PIN_S0S1;
		break;
	case NRF_GPIO_DRIVE_S0H1:
		*drive = NRF_GPIO_PIN_S0H1;
		break;
	case NRF_GPIO_DRIVE_H0S1:
		*drive = NRF_GPIO_PIN_H0S1;
		break;
	case NRF_GPIO_DRIVE_H0H1:
		*drive = NRF_GPIO_PIN_H0H1;
		break;
	case NRF_GPIO_DRIVE_S0 | GPIO_OPEN_DRAIN:
		*drive = NRF_GPIO_PIN_S0D1;
		break;
	case NRF_GPIO_DRIVE_H0 | GPIO_OPEN_DRAIN:
		*drive = NRF_GPIO_PIN_H0D1;
		break;
	case NRF_GPIO_DRIVE_S1 | GPIO_OPEN_SOURCE:
		*drive = NRF_GPIO_PIN_D0S1;
		break;
	case NRF_GPIO_DRIVE_H1 | GPIO_OPEN_SOURCE:
		*drive = NRF_GPIO_PIN_D0H1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
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
	nrfx_err_t err;
	uint8_t ch;
	bool free_ch;
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	nrfx_gpiote_pin_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);

	/* Get the GPIOTE channel associated with this pin, if any. It needs
	 * to be freed when the pin is reconfigured or disconnected.
	 */
	err = nrfx_gpiote_channel_get(abs_pin, &ch);
	free_ch = (err == NRFX_SUCCESS);

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		/* Ignore the error code. The pin may not have been used. */
		(void)nrfx_gpiote_pin_uninit(abs_pin);

		if (free_ch) {
			err = nrfx_gpiote_channel_free(ch);
			__ASSERT_NO_MSG(err == NRFX_SUCCESS);
		}

		return 0;
	}

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_NONE
	};

	/* Remove previously configured trigger when pin is reconfigured. */
	err = nrfx_gpiote_input_configure(abs_pin, NULL, &trigger_config, NULL);
	if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	if (free_ch) {
		err = nrfx_gpiote_channel_free(ch);
		__ASSERT_NO_MSG(err == NRFX_SUCCESS);
	}

	if (flags & GPIO_OUTPUT) {
		nrf_gpio_pin_drive_t drive;
		int rv = get_drive(flags, &drive);

		if (rv != 0) {
			return rv;
		}

		nrfx_gpiote_output_config_t output_config = {
			.drive = drive,
			.input_connect = (flags & GPIO_INPUT) ?
				NRF_GPIO_PIN_INPUT_CONNECT :
				NRF_GPIO_PIN_INPUT_DISCONNECT,
			.pull = get_pull(flags)
		};


		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			nrf_gpio_port_out_set(cfg->port, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			nrf_gpio_port_out_clear(cfg->port, BIT(pin));
		}

		err = nrfx_gpiote_output_configure(abs_pin, &output_config, NULL);
		return (err != NRFX_SUCCESS) ? -EINVAL : 0;
	}

	nrfx_gpiote_input_config_t input_config = {
		.pull = get_pull(flags)
	};

	err = nrfx_gpiote_input_configure(abs_pin, &input_config, NULL, NULL);

	return (err != NRFX_SUCCESS) ? -EINVAL : 0;
}

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

	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~set_mask) & mask;

	nrf_gpio_port_out_set(reg, set_mask);
	nrf_gpio_port_out_clear(reg, clear_mask);

	return 0;
}

static int gpio_nrfx_port_set_bits_raw(const struct device *port,
				       gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_set(reg, mask);

	return 0;
}

static int gpio_nrfx_port_clear_bits_raw(const struct device *port,
					 gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_clear(reg, mask);

	return 0;
}

static int gpio_nrfx_port_toggle_bits(const struct device *port,
				      gpio_port_pins_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	const uint32_t value = nrf_gpio_port_out_read(reg) ^ mask;
	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~value) & mask;

	nrf_gpio_port_out_set(reg, set_mask);
	nrf_gpio_port_out_clear(reg, clear_mask);

	return 0;
}

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
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num, pin);
	nrfx_err_t err;
	uint8_t ch;

	if (mode == GPIO_INT_MODE_DISABLED) {
		nrfx_gpiote_trigger_disable(abs_pin);

		return 0;
	}

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = get_trigger(mode, trig),
	};

	/* If edge mode is to be used and pin is not configured to use sense for
	 * edge use IN event.
	 */
	if (!(BIT(pin) & get_port_cfg(port)->edge_sense) &&
	    (mode == GPIO_INT_MODE_EDGE) &&
	    (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_INPUT)) {
		err = nrfx_gpiote_channel_get(abs_pin, &ch);
		if (err == NRFX_ERROR_INVALID_PARAM) {
			err = nrfx_gpiote_channel_alloc(&ch);
			if (err != NRFX_SUCCESS) {
				return -ENOMEM;
			}
		}

		trigger_config.p_in_channel = &ch;
	}

	err = nrfx_gpiote_input_configure(abs_pin, NULL, &trigger_config, NULL);
	if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	nrfx_gpiote_trigger_enable(abs_pin, true);

	return 0;
}

static int gpio_nrfx_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&get_port_data(port)->callbacks,
				     callback, set);
}

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
		while (map) {
			uint32_t pin = __CLZ(__RBIT(map));
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

#define GPIOTE_NODE DT_INST(0, nordic_nrf_gpiote)

static int gpio_nrfx_init(const struct device *port)
{
	nrfx_err_t err;

	if (nrfx_gpiote_is_init()) {
		return 0;
	}

	err = nrfx_gpiote_init(0/*not used*/);
	if (err != NRFX_SUCCESS) {
		return -EIO;
	}

	nrfx_gpiote_global_callback_set(nrfx_gpio_handler, NULL);

	IRQ_CONNECT(DT_IRQN(GPIOTE_NODE), DT_IRQ(GPIOTE_NODE, priority),
		    nrfx_isr, nrfx_gpiote_irq_handler, 0);

	return 0;
}

static const struct gpio_driver_api gpio_nrfx_drv_api_funcs = {
	.pin_configure = gpio_nrfx_pin_configure,
	.port_get_raw = gpio_nrfx_port_get_raw,
	.port_set_masked_raw = gpio_nrfx_port_set_masked_raw,
	.port_set_bits_raw = gpio_nrfx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nrfx_port_clear_bits_raw,
	.port_toggle_bits = gpio_nrfx_port_toggle_bits,
	.pin_interrupt_configure = gpio_nrfx_pin_interrupt_configure,
	.manage_callback = gpio_nrfx_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_nrfx_port_get_direction,
#endif
};

/* Device instantiation is done with node labels because 'port_num' is
 * the peripheral number by SoC numbering. We therefore cannot use
 * DT_INST APIs here without wider changes.
 */

#define GPIO_NRF_DEVICE(id)						\
	static const struct gpio_nrfx_cfg gpio_nrfx_p##id##_cfg = {	\
		.common = {						\
			.port_pin_mask =				\
			GPIO_PORT_PIN_MASK_FROM_DT_INST(id),		\
		},							\
		.port = (NRF_GPIO_Type *)DT_INST_REG_ADDR(id),		\
		.port_num = DT_INST_PROP(id, port),			\
		.edge_sense = DT_INST_PROP_OR(id, sense_edge_mask, 0)	\
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
