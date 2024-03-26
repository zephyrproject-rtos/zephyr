/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rzt2m_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/errno_private.h>
#include <zephyr/dt-bindings/gpio/renesas-rzt2m-gpio.h>
#include <soc.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

static const struct device *const ns_portnf_md_dev = DEVICE_DT_GET(DT_NODELABEL(ns_portnf_md));

#define PMm_OFFSET    0x200
#define PMCm_OFFSET   0x400
#define PFCm_OFFSET   0x600
#define PINm_OFFSET   0x800
#define DRCTLm_OFFSET 0xa00

#define PMm_SIZE    0x2
#define DRCTLm_SIZE 0x8
#define PFCm_SIZE   0x4

/* config defines in include/zephyr/dt-bindings/gpio/renesas-rzt2m-gpio.h */
#define DRIVE_SHIFT           0
#define SCHMITT_TRIGGER_SHIFT 4
#define SLEW_RATE_SHIFT       5

#define PULL_SHIFT 2
#define PULL_NONE  (0 << PULL_SHIFT)
#define PULL_UP    (1 << PULL_SHIFT)
#define PULL_DOWN  (2 << PULL_SHIFT)

#define INT_INVERT       0
#define INT_FALLING_EDGE 1
#define INT_RISING_EDGE  2
#define INT_BOTH_EDGE    3

#define IRQ_COUNT    16
#define NS_IRQ_COUNT 14

#define MAX_PORT_SIZE 8

#define RZT2M_GPIO_VALUE_IDENTITY(i, _) i

struct rzt2m_gpio_config {
	struct gpio_driver_config common;
	uint8_t pin_irqs[MAX_PORT_SIZE];
	uint8_t *port_nsr;
	uint8_t *ptadr;
	uint8_t port;
};

struct rzt2m_gpio_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
};

struct rzt2m_gpio_irq_slot {
	const struct device *dev;
	uint8_t pin;
};

struct rzt2m_gpio_common_data {
	struct rzt2m_gpio_irq_slot irq_registered_ports[IRQ_COUNT];
};

static struct rzt2m_gpio_common_data rzt2m_gpio_common_data_inst;

static void rzt2m_gpio_unlock(void)
{
	rzt2m_unlock_prcrn(PRCRN_PRC1 | PRCRN_PRC2);
	rzt2m_unlock_prcrs(PRCRS_GPIO);
}

static void rzt2m_gpio_lock(void)
{
	rzt2m_lock_prcrn(PRCRN_PRC1 | PRCRN_PRC2);
	rzt2m_lock_prcrs(PRCRS_GPIO);
}

/* Port m output data store */
static volatile uint8_t *rzt2m_gpio_get_p_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->port_nsr + config->port);
}

/* Port m input data store */
static volatile uint8_t *rzt2m_gpio_get_pin_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->port_nsr + PINm_OFFSET + config->port);
}

/* Port m mode register */
static volatile uint16_t *rzt2m_gpio_get_pm_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint16_t *)(config->port_nsr + PMm_OFFSET + PMm_SIZE * config->port);
}

/* IO Buffer m function switching register */
static volatile uint64_t *rzt2m_gpio_get_drctl_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint64_t *)(config->port_nsr + DRCTLm_OFFSET + DRCTLm_SIZE * config->port);
}

/* Port m region select register */
static volatile uint8_t *rzt2m_gpio_get_rselp_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->ptadr + config->port);
}

/* Port m mode control register */
static volatile uint8_t *rzt2m_gpio_get_pmc_reg(const struct device *dev, uint8_t port)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->port_nsr + PMCm_OFFSET + port);
}

/* Port m function control register */
static volatile uint32_t *rzt2m_gpio_get_pfc_reg(const struct device *dev, uint8_t port)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint32_t *)(config->port_nsr + PFCm_OFFSET + PFCm_SIZE * port);
}

static int rzt2m_gpio_init(const struct device *dev)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *rselp_reg = rzt2m_gpio_get_rselp_reg(dev);
	*rselp_reg = 0xFF;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *pin_reg = rzt2m_gpio_get_pin_reg(dev);
	*value = *pin_reg;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				     gpio_port_value_t value)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg = (*p_reg & ~mask) | (value & mask);

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg |= pins;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg &= ~pins;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_toggle(const struct device *dev, gpio_port_pins_t pins)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg ^= pins;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	volatile uint16_t *pm_reg = rzt2m_gpio_get_pm_reg(dev);
	volatile uint64_t *drctl_reg = rzt2m_gpio_get_drctl_reg(dev);

	rzt2m_gpio_unlock();

	WRITE_BIT(*pm_reg, pin * 2, flags & GPIO_INPUT);
	WRITE_BIT(*pm_reg, pin * 2 + 1, flags & GPIO_OUTPUT);

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			rzt2m_port_clear_bits_raw(dev, 1 << pin);
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			rzt2m_port_set_bits_raw(dev, 1 << pin);
		}
	}

	if (flags & GPIO_PULL_UP && flags & GPIO_PULL_DOWN) {
		rzt2m_gpio_lock();
		return -EINVAL;
	}

	uint8_t drctl_pin_config = 0;

	if (flags & GPIO_PULL_UP) {
		drctl_pin_config |= PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		drctl_pin_config |= PULL_DOWN;
	} else {
		drctl_pin_config |= PULL_NONE;
	}

	drctl_pin_config |=
		(flags & RZT2M_GPIO_DRIVE_MASK) >> (RZT2M_GPIO_DRIVE_OFFSET - DRIVE_SHIFT);
	drctl_pin_config |= (flags & RZT2M_GPIO_SCHMITT_TRIGGER_MASK) >>
			    (RZT2M_GPIO_SCHMITT_TRIGGER_OFFSET - SCHMITT_TRIGGER_SHIFT);
	drctl_pin_config |= (flags & RZT2M_GPIO_SLEW_RATE_MASK) >>
			    (RZT2M_GPIO_SLEW_RATE_OFFSET - SLEW_RATE_SHIFT);

	uint64_t drctl_pin_value = *drctl_reg & ~(0xFFULL << (pin * 8));
	*drctl_reg = drctl_pin_value | ((uint64_t)drctl_pin_config << (pin * 8));

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_get_pin_irq(const struct device *dev, gpio_pin_t pin)
{
	const struct rzt2m_gpio_config *config = dev->config;

	if (pin >= MAX_PORT_SIZE) {
		return -1;
	}
	return config->pin_irqs[pin] - 1;
}

static bool rzt2m_gpio_is_irq_used_by_other_pin(const struct device *dev, gpio_pin_t pin,
						uint8_t irq)
{
	if (irq >= IRQ_COUNT) {
		return false;
	}
	if (rzt2m_gpio_common_data_inst.irq_registered_ports[irq].dev == NULL) {
		return false;
	}
	if (rzt2m_gpio_common_data_inst.irq_registered_ports[irq].dev != dev) {
		return true;
	}
	return rzt2m_gpio_common_data_inst.irq_registered_ports[irq].pin != pin;
}

static void rzt2m_gpio_isr(uint8_t *irq_n)
{
	const struct device *dev = rzt2m_gpio_common_data_inst.irq_registered_ports[*irq_n].dev;

	if (dev) {
		struct rzt2m_gpio_data *data = dev->data;
		int irq_pin = rzt2m_gpio_common_data_inst.irq_registered_ports[*irq_n].pin;

		if (irq_pin >= 0) {
			gpio_fire_callbacks(&data->cb, dev, 1 << irq_pin);
		}
	}
}

static int rzt2m_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct rzt2m_gpio_config *config = dev->config;
	volatile uint8_t *pmc_reg = rzt2m_gpio_get_pmc_reg(dev, config->port);
	volatile uint32_t *pfc_reg = rzt2m_gpio_get_pfc_reg(dev, config->port);
	uint32_t ns_portnf_md_val = 0;

	syscon_read_reg(ns_portnf_md_dev, 0, &ns_portnf_md_val);

	/* level interrupts are not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	uint8_t irq = rzt2m_gpio_get_pin_irq(dev, pin);
	bool irq_used_by_other = rzt2m_gpio_is_irq_used_by_other_pin(dev, pin, irq);

	if (irq < 0) {
		return -ENOTSUP;
	}

	/* secure range - currently not supported*/
	if (irq >= NS_IRQ_COUNT) {
		return -ENOSYS;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		rzt2m_gpio_unlock();
		WRITE_BIT(*pmc_reg, pin, 0);

		/* check if selected pin is using irq line to avoid unregistering other pin irq
		 * handler
		 */
		if (!irq_used_by_other) {
			rzt2m_gpio_common_data_inst.irq_registered_ports[irq].dev = NULL;
		}
		rzt2m_gpio_lock();
		return 0;
	}

	/* the irq line is used by another pin */
	if (irq_used_by_other) {
		return -EBUSY;
	}

	uint8_t md_mode = 0x0;

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		md_mode = INT_FALLING_EDGE;
		break;
	case GPIO_INT_TRIG_HIGH:
		md_mode = INT_RISING_EDGE;
		break;
	case GPIO_INT_TRIG_BOTH:
		md_mode = INT_BOTH_EDGE;
	}

	rzt2m_gpio_unlock();

	uint32_t mdx_mask =
		~((uint32_t)0b11 << irq); /* description of interrupt type has length of 2 bits */
	ns_portnf_md_val = (ns_portnf_md_val & mdx_mask) | (md_mode << irq);
	syscon_write_reg(ns_portnf_md_dev, 0, ns_portnf_md_val); /* set interrupt type */

	WRITE_BIT(*pmc_reg, pin, 1); /* enable special function on selected pin */

	/* in case of every pin on every port irq function number is 0 */
	*pfc_reg &= ~((uint32_t)0b1111 << pin * 4);

	/* register handling interrupt in isr for selected port and pin */
	rzt2m_gpio_common_data_inst.irq_registered_ports[irq].dev = dev;
	rzt2m_gpio_common_data_inst.irq_registered_ports[irq].pin = pin;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_manage_callback(const struct device *dev, struct gpio_callback *cb, bool set)
{
	struct rzt2m_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->cb, cb, set);
}

static const struct gpio_driver_api rzt2m_gpio_driver_api = {
	.pin_configure = rzt2m_gpio_configure,
	.port_get_raw = rzt2m_gpio_get_raw,
	.port_set_masked_raw = rzt2m_port_set_masked_raw,
	.port_set_bits_raw = rzt2m_port_set_bits_raw,
	.port_clear_bits_raw = rzt2m_port_clear_bits_raw,
	.port_toggle_bits = rzt2m_gpio_toggle,
	.pin_interrupt_configure = rzt2m_gpio_pin_interrupt_configure,
	.manage_callback = rzt2m_gpio_manage_callback};

#define RZT2M_INIT_IRQ(irq_n)                                                              \
	IRQ_CONNECT(DT_IRQ_BY_IDX(DT_INST(0, renesas_rzt2m_gpio_common), irq_n, irq),          \
		    DT_IRQ_BY_IDX(DT_INST(0, renesas_rzt2m_gpio_common), irq_n, priority),         \
		    rzt2m_gpio_isr, &n[irq_n],                                                     \
		    DT_IRQ_BY_IDX(DT_INST(0, renesas_rzt2m_gpio_common), irq_n, flags))            \
	irq_enable(DT_IRQ_BY_IDX(DT_INST(0, renesas_rzt2m_gpio_common), irq_n, irq));

static int rzt2m_gpio_common_init(const struct device *dev)
{
	struct rzt2m_gpio_common_data *data = dev->data;

	static uint8_t n[IRQ_COUNT];

	for (int i = 0; i < ARRAY_SIZE(n); i++) {
		n[i] = i;
		data->irq_registered_ports[i].dev = NULL;
	}

	FOR_EACH(RZT2M_INIT_IRQ, (), LISTIFY(NS_IRQ_COUNT, RZT2M_GPIO_VALUE_IDENTITY, (,)))

	return 0;
}

DEVICE_DT_DEFINE(DT_INST(0, renesas_rzt2m_gpio_common),
		    rzt2m_gpio_common_init,
		    NULL,
		    &rzt2m_gpio_common_data_inst, NULL,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    NULL);

#define VALUE_2X(i, _) UTIL_X2(i)

#define PIN_IRQ_INITIALIZER(idx, inst)                                                             \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, irqs, idx),                                         \
		    ([DT_INST_PROP_BY_IDX(inst, irqs, idx)] =                                      \
			     DT_INST_PROP_BY_IDX(inst, irqs, UTIL_INC(idx)) + 1,),                \
		    ())

#define PORT_IRQS_INITIALIZER(inst)                                                                \
	FOR_EACH_FIXED_ARG(PIN_IRQ_INITIALIZER, (), inst,                                          \
			   LISTIFY(DT_INST_PROP_LEN_OR(inst, irqs, 0), VALUE_2X, (,)))

#define RZT2M_GPIO_DEFINE(inst)                                                                    \
	static struct rzt2m_gpio_data rzt2m_gpio_data##inst;                                       \
	static struct rzt2m_gpio_config rzt2m_gpio_config##inst = {                                \
		.port_nsr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_GPARENT(inst), port_nsr),       \
		.ptadr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_GPARENT(inst), ptadr),             \
		.port = DT_INST_REG_ADDR(inst),                                                    \
		.pin_irqs = {PORT_IRQS_INITIALIZER(inst)},                                         \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)}};               \
	DEVICE_DT_INST_DEFINE(inst, rzt2m_gpio_init, NULL, &rzt2m_gpio_data##inst,          \
			      &rzt2m_gpio_config##inst, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,   \
			      &rzt2m_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RZT2M_GPIO_DEFINE)
