/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <zephyr/drivers/pinctrl/pinctrl_ra2.h>
#include <zephyr/drivers/interrupt_controller/intc_ra2_icu.h>
#include <zephyr/drivers/gpio/gpio_renesas_ra2.h>

#define DT_DRV_COMPAT renesas_ra2_gpio

#define PCNTR1_OFF		0x0
#define		PODR_OFF	0x0
#define		PDR_OFF		0x2
#define PCNTR2_OFF		0x4
#define		EIDR_OFF	0x4
#define		PIDR_OFF	0x6
#define PCNTR3_OFF		0x8
#define		PORR_OFF	0x8
#define		POSR_OFF	0xa
#define PCNTR4_OFF		0xc
#define		EORR_OFF	0xc
#define		EOSR_OFF	0xe

typedef struct gpio_irq_item {
	gpio_pin_t	pin;
	uint8_t		irq;
} gpio_irq_item_t;

struct gpio_ra_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	mm_reg_t  base;
	const gpio_irq_item_t *irqs;
	uint8_t ioport_id;
	uint8_t num_irqs;
};

struct gpio_ra_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

struct port_irq_event {
	uint8_t event1;
	uint8_t event2;
};

struct port_irq_data {
	struct icu_event	*event;
	const struct device	*gpio_port;
	gpio_pin_t		pin;
};

static K_MUTEX_DEFINE(lock);

const struct port_irq_event port_irq_events[] = {
	{ ICU_EVENT_GROUP0_PORT_IRQ0, ICU_EVENT_GROUP4_PORT_IRQ0, },
	{ ICU_EVENT_GROUP1_PORT_IRQ1, ICU_EVENT_GROUP5_PORT_IRQ1, },
	{ ICU_EVENT_GROUP2_PORT_IRQ2, ICU_EVENT_GROUP6_PORT_IRQ2, },
	{ ICU_EVENT_GROUP3_PORT_IRQ3, ICU_EVENT_GROUP7_PORT_IRQ3, },
	{ ICU_EVENT_GROUP4_PORT_IRQ4, 0, },
	{ ICU_EVENT_GROUP5_PORT_IRQ5, 0, },
	{ ICU_EVENT_GROUP6_PORT_IRQ6, 0, },
	{ ICU_EVENT_GROUP7_PORT_IRQ7, 0, },
};

struct port_irq_data irq_datas[ARRAY_SIZE(port_irq_events)];

static int gpio_ra_configure(const struct device *dev, gpio_pin_t pin,
			      gpio_flags_t flags)
{
	const struct gpio_ra_config *cfg = dev->config;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) ==
			(GPIO_INPUT | GPIO_OUTPUT)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	if ((BIT(pin) & cfg->common.port_pin_mask) != 0) {
		pinctrl_soc_pin_t pfs_cache = {
			RA_PIN_FLAGS_PIN(pin) |
			RA_PIN_FLAGS_PORT(cfg->ioport_id),
		};

		if (flags & GPIO_OUTPUT) {
			pfs_cache |= RA_PIN_FLAGS_PDR;
		}

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			pfs_cache |= RA_PIN_FLAGS_PODR;
		}

		if (flags & GPIO_PULL_UP) {
			pfs_cache |= RA_PIN_FLAGS_PCR;
		}

		if (flags & GPIO_OPEN_DRAIN) {
			pfs_cache |= RA_PIN_FLAGS_NCODR;
		}

		return pinctrl_configure_pins(&pfs_cache, 1, PINCTRL_REG_NONE);
	}

	return -ENODEV;
}

static int gpio_ra_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_ra_config *cfg = port->config;

	*value = sys_read16(cfg->base + PIDR_OFF) & cfg->common.port_pin_mask;

	return 0;
}

static int gpio_ra_port_set_masked_raw(const struct device *port,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_ra_config *cfg = port->config;

	sys_write16((value & mask) & cfg->common.port_pin_mask,
			cfg->base + POSR_OFF);

	return 0;
}

static int gpio_ra_port_set_bits_raw(const struct device *port,
				      gpio_port_pins_t pins)
{
	const struct gpio_ra_config *cfg = port->config;

	sys_write16(pins & cfg->common.port_pin_mask, cfg->base + POSR_OFF);

	return 0;
}

static int gpio_ra_port_clear_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	const struct gpio_ra_config *cfg = port->config;

	sys_write16(pins & cfg->common.port_pin_mask, cfg->base + PORR_OFF);

	return 0;
}

static int gpio_ra_port_toggle_bits(const struct device *port,
					gpio_port_pins_t pins)
{
	const struct gpio_ra_config *cfg = port->config;
	uint32_t val = (sys_read16(cfg->base + PODR_OFF) ^ pins) &
				cfg->common.port_pin_mask;

	val |= (~val & cfg->common.port_pin_mask) << 16;

	sys_write32(val, cfg->base + PCNTR3_OFF);

	return 0;
}

static const gpio_irq_item_t *find_irq_data(const struct device *port,
		gpio_pin_t pin)
{
	const struct gpio_ra_config *cfg = port->config;
	int i;

	for (i = 0; i < cfg->num_irqs; i++) {
		if (cfg->irqs[i].pin == pin) {
			return &cfg->irqs[i];
		}
	}

	return NULL;
}

static void gpio_ra_port_isr(struct icu_event *icu_evt, void *data);

int gpio_ra_activate_wakeup(const struct device *port, gpio_pin_t pin,
		irq_ra_sense_t sense, nmi_irq_ra_division_t div, int filtered)
{
	int ret;
	const gpio_irq_item_t *data = find_irq_data(port, pin);

	if (data == NULL) {
		return -ENOTSUP;
	}

	ret = ra_set_irq_cfg(data->irq, sense, div, filtered);
	if (ret != 0) {
		return ret;
	}

	return ra_activate_wakeup_sources(IRQ_WAKE(data->irq));
}

int gpio_ra_deactivate_wakeup(const struct device *port, gpio_pin_t pin)
{
	const gpio_irq_item_t *data = find_irq_data(port, pin);

	if (data == NULL) {
		return -ENOTSUP;
	}

	return ra_deactivate_wakeup_sources(IRQ_WAKE(data->irq));
}

static int gpio_ra_pin_interrupt_configure(const struct device *port,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const gpio_irq_item_t *irq_it = find_irq_data(port, pin);
	const struct gpio_ra_config *config = port->config;
	pinctrl_soc_pin_t pfs_cache;
	int ret;

	if (!config->num_irqs || !irq_it ||
			irq_it->irq >= ARRAY_SIZE(irq_datas)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ret = pinctrl_ra_get_pin(config->ioport_id, pin, &pfs_cache);
	if (ret) {
		goto out;
	}

	switch (mode) {
	case GPIO_INT_MODE_EDGE:
		pfs_cache |= RA_PIN_FLAGS_ISEL;
		break;
	case GPIO_INT_MODE_DISABLED:
		pfs_cache &= ~RA_PIN_FLAGS_ISEL;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	pfs_cache &= ~RA_PIN_FLAGS_EOFR(3);
	switch (trig) {
	case GPIO_INT_TRIG_BOTH:
		pfs_cache |= RA_PIN_FLAGS_EOFR(3);
		break;
	case GPIO_INT_TRIG_HIGH:
		pfs_cache |= RA_PIN_FLAGS_EOFR(1);
		break;
	case GPIO_INT_TRIG_LOW:
		pfs_cache |= RA_PIN_FLAGS_EOFR(2);
		break;
	default:
		break;
	}

	ret = pinctrl_configure_pins(&pfs_cache, 1, PINCTRL_REG_NONE);
	if (ret) {
		goto out;
	}

	switch (mode) {
	case GPIO_INT_MODE_EDGE: {
			struct icu_event *event = ra_icu_setup_event_irq(
					port_irq_events[irq_it->irq].event1,
					gpio_ra_port_isr,
					&irq_datas[irq_it->irq]);

			if (!event && port_irq_events[irq_it->irq].event2) {
				event = ra_icu_setup_event_irq(
					port_irq_events[irq_it->irq].event2,
					gpio_ra_port_isr,
					&irq_datas[irq_it->irq]);
			}

			if (!event) {
				ret = -EBUSY;
				break;
			}

			irq_datas[irq_it->irq].event = event;
			irq_datas[irq_it->irq].gpio_port  = port;
			irq_datas[irq_it->irq].pin   = pin;

			ra_icu_enable_event(event);
		}
		break;
	case GPIO_INT_MODE_DISABLED:
		if (irq_datas[irq_it->irq].event) {
			struct icu_event *event = irq_datas[irq_it->irq].event;

			ra_icu_disable_event(event);
			ra_icu_shutdown_event_irq(event);
			irq_datas[irq_it->irq].event = NULL;
		}
		break;
	default:
		break;
	}

out:
	k_mutex_unlock(&lock);
	return ret;
}

static int gpio_ra_manage_callback(const struct device *port,
		struct gpio_callback *cb, bool set)
{
	struct gpio_ra_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static void gpio_ra_port_isr(struct icu_event *icu_evt, void *arg)
{
	struct port_irq_data *irq_data = arg;
	struct gpio_ra_data *gpio_data = irq_data->gpio_port->data;

	ra_icu_clear_event(icu_evt);

	gpio_fire_callbacks(&gpio_data->callbacks, irq_data->gpio_port,
			BIT(irq_data->pin));
}

static const struct gpio_driver_api gpio_ra_driver_api = {
	.pin_configure = gpio_ra_configure,
	.port_get_raw = gpio_ra_port_get_raw,
	.port_set_masked_raw = gpio_ra_port_set_masked_raw,
	.port_set_bits_raw = gpio_ra_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ra_port_clear_bits_raw,
	.port_toggle_bits = gpio_ra_port_toggle_bits,
	.pin_interrupt_configure = gpio_ra_pin_interrupt_configure,
	.manage_callback = gpio_ra_manage_callback,
};

#define GPIO_RA_INIT(n)							\
	static const struct gpio_ra_config ra_gpio_##n##_config = {	\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(\
				DT_INST_PHANDLE(n, renesas_ra2_pfs)),	\
		},							\
		.base = DT_INST_REG_ADDR(n),				\
		.irqs  = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),     \
				((const gpio_irq_item_t *)((const uint8_t [])  \
					DT_INST_PROP(n, interrupts))),	       \
				(NULL)),				       \
		.num_irqs = DT_NUM_IRQS(DT_DRV_INST(n)),		       \
		.ioport_id = DT_NODE_CHILD_IDX(DT_INST_PHANDLE(n,	       \
					renesas_ra2_pfs)),		       \
	};								\
									\
	static struct gpio_ra_data ra_gpio_##n##_data;			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			NULL,						\
			NULL,						\
			&ra_gpio_##n##_data,				\
			&ra_gpio_##n##_config,				\
			PRE_KERNEL_1,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			&gpio_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RA_INIT)
