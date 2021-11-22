/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_gpio

#include <drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <nrfx_gpiote.h>

#include "gpio_utils.h"

#if IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE) &&	\
	!defined(NRF_GPIO_LATCH_PRESENT)
#error "GPIO LATCH is required by edge interrupts using GPIO SENSE," \
	"but it is not supported by the platform."
#endif

struct gpio_nrfx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;

	/* Mask holding information about which pins have been configured to
	 * trigger interrupts using gpio_nrfx_config function.
	 */
	uint32_t pin_int_en;

	uint32_t int_active_level;
	uint32_t trig_edge;
	uint32_t double_edge;
};

struct gpio_nrfx_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	NRF_GPIO_Type *port;
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

static int gpiote_channel_alloc(uint32_t abs_pin,
				nrf_gpiote_polarity_t polarity)
{
	uint8_t channel;

	if (nrfx_gpiote_channel_alloc(&channel) != NRFX_SUCCESS) {
		return -ENODEV;
	}

	nrf_gpiote_event_t evt = offsetof(NRF_GPIOTE_Type, EVENTS_IN[channel]);

	nrf_gpiote_event_configure(NRF_GPIOTE, channel, abs_pin, polarity);
	nrf_gpiote_event_clear(NRF_GPIOTE, evt);
	nrf_gpiote_event_enable(NRF_GPIOTE, channel);
	nrf_gpiote_int_enable(NRF_GPIOTE, BIT(channel));

	return 0;
}

/* Function checks if given pin does not have already enabled GPIOTE event and
 * disables it.
 */
static void gpiote_pin_cleanup(uint32_t abs_pin)
{
	uint32_t intenset = nrf_gpiote_int_enable_check(NRF_GPIOTE,
						     NRF_GPIOTE_INT_IN_MASK);

	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		if ((nrf_gpiote_event_pin_get(NRF_GPIOTE, i) == abs_pin)
		    && (intenset & BIT(i))) {
			nrf_gpiote_event_disable(NRF_GPIOTE, i);
			nrf_gpiote_int_disable(NRF_GPIOTE, BIT(i));
			nrfx_gpiote_channel_free(i);
			return;
		}
	}
}

static inline uint32_t sense_for_pin(const struct gpio_nrfx_data *data,
				  uint32_t pin)
{
	if ((BIT(pin) & data->int_active_level) != 0U) {
		return NRF_GPIO_PIN_SENSE_HIGH;
	}
	return NRF_GPIO_PIN_SENSE_LOW;
}

static int gpiote_pin_int_cfg(const struct device *port, uint32_t pin)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
	int res = 0;

	gpiote_pin_cleanup(abs_pin);
	nrf_gpio_cfg_sense_set(abs_pin, NRF_GPIO_PIN_NOSENSE);

	/* Pins trigger interrupts only if pin has been configured to do so */
	if (data->pin_int_en & BIT(pin)) {
		if (data->trig_edge & BIT(pin)) {
			if (IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE)) {
				bool high;
				uint32_t sense;

				if (nrf_gpio_pin_dir_get(abs_pin) ==
				    NRF_GPIO_PIN_DIR_OUTPUT) {
					high = nrf_gpio_pin_out_read(abs_pin);
				} else {
					high = nrf_gpio_pin_read(abs_pin);
				}

				if (high) {
					sense = GPIO_PIN_CNF_SENSE_Low;
				} else {
					sense = GPIO_PIN_CNF_SENSE_High;
				}

				nrf_gpio_cfg_sense_set(abs_pin, sense);
			} else {
				/* For edge triggering we use GPIOTE channels. */
				nrf_gpiote_polarity_t pol;

				if (data->double_edge & BIT(pin)) {
					pol = NRF_GPIOTE_POLARITY_TOGGLE;
				} else if ((data->int_active_level & BIT(pin)) != 0U) {
					pol = NRF_GPIOTE_POLARITY_LOTOHI;
				} else {
					pol = NRF_GPIOTE_POLARITY_HITOLO;
				}

				res = gpiote_channel_alloc(abs_pin, pol);
			}
		} else {
			/* For level triggering we use sense mechanism. */
			uint32_t sense = sense_for_pin(data, pin);

			nrf_gpio_cfg_sense_set(abs_pin, sense);
		}
	}
	return res;
}

static int gpio_nrfx_config(const struct device *port,
			    gpio_pin_t pin, gpio_flags_t flags)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	nrf_gpio_pin_pull_t pull;
	nrf_gpio_pin_drive_t drive;
	nrf_gpio_pin_dir_t dir;
	nrf_gpio_pin_input_t input;

	switch (flags & (GPIO_DS_LOW_MASK | GPIO_DS_HIGH_MASK |
			 GPIO_OPEN_DRAIN)) {
	case GPIO_DS_DFLT:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case GPIO_DS_DFLT_LOW | GPIO_DS_ALT_HIGH:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case GPIO_DS_DFLT_LOW | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_S0D1;
		break;

	case GPIO_DS_ALT_LOW | GPIO_DS_DFLT_HIGH:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case GPIO_DS_ALT:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case GPIO_DS_ALT_LOW | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_H0D1;
		break;

	case GPIO_DS_DFLT_HIGH | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case GPIO_DS_ALT_HIGH | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0H1;
		break;

	default:
		return -EINVAL;
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		pull = NRF_GPIO_PIN_PULLUP;
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		pull = NRF_GPIO_PIN_PULLDOWN;
	} else {
		pull = NRF_GPIO_PIN_NOPULL;
	}

	dir = ((flags & GPIO_OUTPUT) != 0)
	      ? NRF_GPIO_PIN_DIR_OUTPUT
	      : NRF_GPIO_PIN_DIR_INPUT;

	input = ((flags & GPIO_INPUT) != 0)
		? NRF_GPIO_PIN_INPUT_CONNECT
		: NRF_GPIO_PIN_INPUT_DISCONNECT;

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			nrf_gpio_port_out_set(reg, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			nrf_gpio_port_out_clear(reg, BIT(pin));
		}
	}

	nrf_gpio_cfg(NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num, pin),
		     dir, input, pull, drive, NRF_GPIO_PIN_NOSENSE);

	return 0;
}

static int gpio_nrfx_port_get_raw(const struct device *port, uint32_t *value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	*value = nrf_gpio_port_in_read(reg);

	return 0;
}

static int gpio_nrfx_port_set_masked_raw(const struct device *port,
					 uint32_t mask,
					 uint32_t value)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	uint32_t value_tmp;

	value_tmp = nrf_gpio_port_out_read(reg) & ~mask;
	nrf_gpio_port_out_write(reg, value_tmp | (mask & value));

	return 0;
}

static int gpio_nrfx_port_set_bits_raw(const struct device *port,
				       uint32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_set(reg, mask);

	return 0;
}

static int gpio_nrfx_port_clear_bits_raw(const struct device *port,
					 uint32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;

	nrf_gpio_port_out_clear(reg, mask);

	return 0;
}

static int gpio_nrfx_port_toggle_bits(const struct device *port,
				      uint32_t mask)
{
	NRF_GPIO_Type *reg = get_port_cfg(port)->port;
	uint32_t value;

	value = nrf_gpio_port_out_read(reg);
	nrf_gpio_port_out_write(reg, value ^ mask);

	return 0;
}

static int gpio_nrfx_pin_interrupt_configure(const struct device *port,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(get_port_cfg(port)->port_num, pin);

	if (!IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE) &&
	    (mode == GPIO_INT_MODE_EDGE) &&
	    (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_OUTPUT)) {
		/*
		 * The pin's output value as specified in the GPIO will be
		 * ignored as long as the pin is controlled by GPIOTE.
		 * Pin with output enabled cannot be used as an edge interrupt
		 * source.
		 */
		return -ENOTSUP;
	}

	WRITE_BIT(data->pin_int_en, pin, mode != GPIO_INT_MODE_DISABLED);
	WRITE_BIT(data->trig_edge, pin, mode == GPIO_INT_MODE_EDGE);
	WRITE_BIT(data->double_edge, pin, trig == GPIO_INT_TRIG_BOTH);
	WRITE_BIT(data->int_active_level, pin, trig == GPIO_INT_TRIG_HIGH);

	return gpiote_pin_int_cfg(port, pin);
}

static int gpio_nrfx_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&get_port_data(port)->callbacks,
				     callback, set);
}

static const struct gpio_driver_api gpio_nrfx_drv_api_funcs = {
	.pin_configure = gpio_nrfx_config,
	.port_get_raw = gpio_nrfx_port_get_raw,
	.port_set_masked_raw = gpio_nrfx_port_set_masked_raw,
	.port_set_bits_raw = gpio_nrfx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nrfx_port_clear_bits_raw,
	.port_toggle_bits = gpio_nrfx_port_toggle_bits,
	.pin_interrupt_configure = gpio_nrfx_pin_interrupt_configure,
	.manage_callback = gpio_nrfx_manage_callback,
};

static void cfg_edge_sense_pins(const struct device *port,
				uint32_t sense_levels)
{
	const struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t pin = 0U;
	uint32_t bit = 1U << pin;
	uint32_t edge_pins = data->pin_int_en & (data->trig_edge |
						 data->double_edge);

	/* Configure sense detection on all pins that use it. */
	while (edge_pins) {
		if (edge_pins & bit) {
			uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
			uint32_t sense = (sense_levels & bit) ?
				GPIO_PIN_CNF_SENSE_High :
				GPIO_PIN_CNF_SENSE_Low;

			nrf_gpio_cfg_sense_set(abs_pin, sense);
			edge_pins &= ~bit;
		}
		++pin;
		bit <<= 1;
	}
}

static inline uint32_t get_level_pins(const struct device *port)
{
	struct gpio_nrfx_data *data = get_port_data(port);

	/* Take into consideration only pins that were configured to
	 * trigger interrupts.
	 */
	uint32_t out = data->pin_int_en;

	/* Exclude pins that trigger interrupts by edge. */
	out &= ~data->trig_edge & ~data->double_edge;

	/* The sequence above assumes that the sense field will be
	 * configured only for these pins.  If anybody's modifying
	 * PIN_CNF directly it won't work.
	 */
	return out;
}

static void cfg_level_pins(const struct device *port)
{
	const struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t pin = 0U;
	uint32_t bit = 1U << pin;
	uint32_t level_pins = get_level_pins(port);

	/* Configure sense detection on all pins that use it. */
	while (level_pins) {
		if (level_pins & bit) {
			uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);
			uint32_t sense = sense_for_pin(data, pin);

			nrf_gpio_cfg_sense_set(abs_pin, sense);
			level_pins &= ~bit;
		}
		++pin;
		bit <<= 1;
	}
}

/**
 * @brief Function for getting pins that triggered level interrupt.
 *
 * @param port Pointer to GPIO port device.
 *
 * @return Bitmask where 1 marks pin as trigger source.
 */
static uint32_t check_level_trigger_pins(const struct device *port,
					 uint32_t *sense_levels)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	const struct gpio_nrfx_cfg *cfg = get_port_cfg(port);
	uint32_t level_pins = get_level_pins(port);
	uint32_t port_in = nrf_gpio_port_in_read(cfg->port);

	/* Extract which pins have logic level same as interrupt trigger level.
	 */
	uint32_t pin_states = ~(port_in ^ data->int_active_level);

	/* Discard pins that aren't configured for level. */
	uint32_t out = pin_states & level_pins;

	/* Disable sense detection on all pins that use it, whether
	 * they appear to have triggered or not.  This ensures
	 * nobody's requesting DETECT.
	 */
	uint32_t pin = 0U;
	uint32_t bit = 1U << pin;

	uint32_t port_latch = 0;

	uint32_t check_pins = level_pins;

	if (IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE)) {
		check_pins = data->pin_int_en;
	}

#if IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE)
	/* Read LATCH, which will tell us which pin has changed its state. */
	port_latch = cfg->port->LATCH;
#endif

	while (check_pins) {
		if (check_pins & bit) {
			uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, pin);

			if (!(level_pins & bit)) {
				uint32_t sense = nrf_gpio_pin_sense_get(abs_pin);
				bool high = (sense == GPIO_PIN_CNF_SENSE_High);

				if (port_latch & bit) {
					/* check if there was an interrupt */
					if ((data->double_edge & bit) ||
					    ((!!data->int_active_level) == high)) {
						out |= bit;
					}

					/* invert configured level */
					high = !high;
				}

				if (high) {
					*sense_levels |= bit;
				}
			}

			nrf_gpio_cfg_sense_set(abs_pin, NRF_GPIO_PIN_NOSENSE);
			check_pins &= ~bit;
		}
		++pin;
		bit <<= 1;
	}

#if IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE)
	/* Clear LATCH, as at this point every level detection pin is
	 * disabled.
	 */
	cfg->port->LATCH = port_latch;
#endif

	return out;
}

static inline void fire_callbacks(const struct device *port, uint32_t pins)
{
	struct gpio_nrfx_data *data = get_port_data(port);
	sys_slist_t *list = &data->callbacks;

	gpio_fire_callbacks(list, port, pins);
}

static void gpiote_event_handler(void)
{
	uint32_t fired_triggers[GPIO_COUNT] = {0};
	uint32_t sense_levels[GPIO_COUNT] = {0};
	bool port_event = nrf_gpiote_event_check(NRF_GPIOTE,
						 NRF_GPIOTE_EVENT_PORT);

	if (port_event) {
		#define GPIO_NRF_GET_TRIGGERS(i) \
			fired_triggers[DT_INST_PROP(i, port)] = \
				check_level_trigger_pins(DEVICE_DT_INST_GET(i), \
							 &sense_levels[DT_INST_PROP(i, port)]);

		DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_GET_TRIGGERS)
		#undef GPIO_NRF_GET_TRIGGERS

		/* Sense detect was disabled while checking pins so
		 * DETECT should be deasserted.
		 */
		nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_PORT);
	}

	/* Handle interrupt from GPIOTE channels. */
	for (size_t i = 0; i < GPIOTE_CH_NUM; i++) {
		nrf_gpiote_event_t evt =
			offsetof(NRF_GPIOTE_Type, EVENTS_IN[i]);

		if (nrf_gpiote_int_enable_check(NRF_GPIOTE, BIT(i)) &&
		    nrf_gpiote_event_check(NRF_GPIOTE, evt)) {
			uint32_t abs_pin = nrf_gpiote_event_pin_get(NRF_GPIOTE, i);
			/* Divide absolute pin number to port and pin parts. */
			fired_triggers[abs_pin / 32U] |= BIT(abs_pin % 32);
			nrf_gpiote_event_clear(NRF_GPIOTE, evt);
		}
	}

	if (IS_ENABLED(CONFIG_GPIO_NRF_INT_EDGE_USING_SENSE) && port_event) {
		/* Reprogram sense to match current edge to be detected. Make it
		 * now to detect all new edges after callbacks were fired.
		 *
		 * This may cause DETECT to be re-asserted if pin state has
		 * already changed to the newly configured sense level.
		 */
		#define GPIO_NRF_CFG_EDGE_SENSE_PINS(i) \
		   cfg_edge_sense_pins(DEVICE_DT_INST_GET(i), \
				       sense_levels[DT_INST_PROP(i, port)]);

		DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_CFG_EDGE_SENSE_PINS)
		#undef GPIO_NRF_CFG_EDGE_SENSE_PINS
	}

	#define GPIO_NRF_FIRE_CALLBACKS(i) \
		if (fired_triggers[DT_INST_PROP(i, port)]) { \
			fire_callbacks(DEVICE_DT_INST_GET(i), \
				       fired_triggers[DT_INST_PROP(i, port)]); \
		}

	DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_FIRE_CALLBACKS)
	#undef GPIO_NRF_FIRE_CALLBACKS

	if (port_event) {
		/* Reprogram sense to match current configuration.
		 * This may cause DETECT to be re-asserted.
		 */
		#define GPIO_NRF_CFG_LEVEL_PINS(i) cfg_level_pins(DEVICE_DT_INST_GET(i));

		DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_CFG_LEVEL_PINS)
		#undef GPIO_NRF_CFG_LEVEL_PINS
	}
}


#define GPIOTE_NODE DT_INST(0, nordic_nrf_gpiote)

static int gpio_nrfx_init(const struct device *port)
{
	static bool gpio_initialized;

	if (!gpio_initialized) {
		gpio_initialized = true;
		IRQ_CONNECT(DT_IRQN(GPIOTE_NODE), DT_IRQ(GPIOTE_NODE, priority),
			    gpiote_event_handler, NULL, 0);

		irq_enable(DT_IRQN(GPIOTE_NODE));
		nrf_gpiote_int_enable(NRF_GPIOTE, NRF_GPIOTE_INT_PORT_MASK);
	}

	return 0;
}

/*
 * Device instantiation is done with node labels because 'port_num' is
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
		.port_num = DT_INST_PROP(id, port)			\
	};								\
									\
	static struct gpio_nrfx_data gpio_nrfx_p##id##_data;		\
									\
	DEVICE_DT_INST_DEFINE(id, gpio_nrfx_init,			\
			 NULL,						\
			 &gpio_nrfx_p##id##_data,			\
			 &gpio_nrfx_p##id##_cfg,			\
			 POST_KERNEL,					\
			 CONFIG_GPIO_INIT_PRIORITY,			\
			 &gpio_nrfx_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NRF_DEVICE)
