/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_gpio

#include <errno.h>

#include <kernel.h>
#include <drivers/gpio.h>
#include "gpio_dw.h"
#include "gpio_utils.h"

#include <soc.h>
#include <sys/sys_io.h>
#include <init.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include <drivers/clock_control.h>

#ifdef CONFIG_SHARED_IRQ
#include <shared_irq.h>
#endif

#ifdef CONFIG_IOAPIC
#include <drivers/interrupt_controller/ioapic.h>
#endif

#ifdef CONFIG_PM_DEVICE
#include <power/power.h>
#endif

static int gpio_dw_port_set_bits_raw(const struct device *port, uint32_t mask);
static int gpio_dw_port_clear_bits_raw(const struct device *port,
				       uint32_t mask);

/*
 * ARC architecture configure IP through IO auxiliary registers.
 * Other architectures as ARM and x86 configure IP through MMIO registers
 */
#ifdef GPIO_DW_IO_ACCESS
static inline uint32_t dw_read(uint32_t base_addr, uint32_t offset)
{
	return sys_in32(base_addr + offset);
}

static inline void dw_write(uint32_t base_addr, uint32_t offset,
			    uint32_t val)
{
	sys_out32(val, base_addr + offset);
}

static void dw_set_bit(uint32_t base_addr, uint32_t offset,
		       uint32_t bit, bool value)
{
	if (!value) {
		sys_io_clear_bit(base_addr + offset, bit);
	} else {
		sys_io_set_bit(base_addr + offset, bit);
	}
}
#else
static inline uint32_t dw_read(uint32_t base_addr, uint32_t offset)
{
	return sys_read32(base_addr + offset);
}

static inline void dw_write(uint32_t base_addr, uint32_t offset,
			    uint32_t val)
{
	sys_write32(val, base_addr + offset);
}

static void dw_set_bit(uint32_t base_addr, uint32_t offset,
		       uint32_t bit, bool value)
{
	if (!value) {
		sys_clear_bit(base_addr + offset, bit);
	} else {
		sys_set_bit(base_addr + offset, bit);
	}
}
#endif

#ifdef CONFIG_GPIO_DW_CLOCK_GATE
static inline void gpio_dw_clock_config(const struct device *port)
{
	char *drv = CONFIG_GPIO_DW_CLOCK_GATE_DRV_NAME;
	const struct device *clk;

	clk = device_get_binding(drv);
	if (clk) {
		struct gpio_dw_runtime *context = port->data;

		context->clock = clk;
	}
}

static inline void gpio_dw_clock_on(const struct device *port)
{
	const struct gpio_dw_config *config = port->config;
	struct gpio_dw_runtime *context = port->data;

	clock_control_on(context->clock, config->clock_data);
}

static inline void gpio_dw_clock_off(const struct device *port)
{
	const struct gpio_dw_config *config = port->config;
	struct gpio_dw_runtime *context = port->data;

	clock_control_off(context->clock, config->clock_data);
}
#else
#define gpio_dw_clock_config(...)
#define gpio_dw_clock_on(...)
#define gpio_dw_clock_off(...)
#endif

static inline int dw_base_to_block_base(uint32_t base_addr)
{
	return (base_addr & 0xFFFFFFC0);
}

static inline int dw_derive_port_from_base(uint32_t base_addr)
{
	uint32_t port = (base_addr & 0x3f) / 12U;
	return port;
}

static inline int dw_interrupt_support(const struct gpio_dw_config *config)
{
	return ((int)(config->irq_num) > 0U);
}

static inline uint32_t dw_get_ext_port(uint32_t base_addr)
{
	uint32_t ext_port;

	/* 4-port GPIO implementation translates from base address to port */
	switch (dw_derive_port_from_base(base_addr)) {
	case 1:
		ext_port = EXT_PORTB;
		break;
	case 2:
		ext_port = EXT_PORTC;
		break;
	case 3:
		ext_port = EXT_PORTD;
		break;
	case 0:
	default:
		ext_port = EXT_PORTA;
		break;
	}

	return ext_port;
}

static inline uint32_t dw_get_data_port(uint32_t base_addr)
{
	uint32_t dr_port;

	/* 4-port GPIO implementation translates from base address to port */
	switch (dw_derive_port_from_base(base_addr)) {
	case 1:
		dr_port = SWPORTB_DR;
		break;
	case 2:
		dr_port = SWPORTC_DR;
		break;
	case 3:
		dr_port = SWPORTD_DR;
		break;
	case 0:
	default:
		dr_port = SWPORTA_DR;
		break;
	}

	return dr_port;
}

static inline uint32_t dw_get_dir_port(uint32_t base_addr)
{
	uint32_t ddr_port;

	/* 4-port GPIO implementation translates from base address to port */
	switch (dw_derive_port_from_base(base_addr)) {
	case 1:
		ddr_port = SWPORTB_DDR;
		break;
	case 2:
		ddr_port = SWPORTC_DDR;
		break;
	case 3:
		ddr_port = SWPORTD_DDR;
		break;
	case 0:
	default:
		ddr_port = SWPORTA_DDR;
		break;
	}

	return ddr_port;
}

static int gpio_dw_pin_interrupt_configure(const struct device *port,
					   gpio_pin_t pin,
					   enum gpio_int_mode mode,
					   enum gpio_int_trig trig)
{
	struct gpio_dw_runtime *context = port->data;
	const struct gpio_dw_config *config = port->config;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t dir_port = dw_get_dir_port(port_base_addr);
	uint32_t data_port = dw_get_data_port(port_base_addr);
	uint32_t dir_reg;

	/* Check for invalid pin number */
	if (pin >= config->bits) {
		return -EINVAL;
	}

	/* Only PORT-A supports interrupts */
	if (data_port != SWPORTA_DR) {
		return -ENOTSUP;
	}

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* Check if GPIO port supports interrupts */
		if (!dw_interrupt_support(config)) {
			return -ENOTSUP;
		}

		/* Interrupt to be enabled but pin is not set to input */
		dir_reg = dw_read(port_base_addr, dir_port) & BIT(pin);
		if (dir_reg != 0U) {
			return -EINVAL;
		}
	}

	/* Does not support both edges */
	if ((mode == GPIO_INT_MODE_EDGE) &&
	    (trig == GPIO_INT_TRIG_BOTH)) {
		return -ENOTSUP;
	}

	/* Clear interrupt enable */
	dw_set_bit(base_addr, INTEN, pin, false);

	/* Mask and clear interrupt */
	dw_set_bit(base_addr, INTMASK, pin, true);
	dw_write(base_addr, PORTA_EOI, BIT(pin));

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* level (0) or edge (1) */
		dw_set_bit(base_addr, INTTYPE_LEVEL, pin,
			   (mode == GPIO_INT_MODE_EDGE));

		/* Active low/high */
		dw_set_bit(base_addr, INT_POLARITY, pin,
			   (trig == GPIO_INT_TRIG_HIGH));

		/* Finally enabling interrupt */
		dw_set_bit(base_addr, INTEN, pin, true);
		dw_set_bit(base_addr, INTMASK, pin, false);
	}

	return 0;
}

static inline void dw_pin_config(const struct device *port,
				 uint32_t pin, int flags)
{
	struct gpio_dw_runtime *context = port->data;
	const struct gpio_dw_config *config = port->config;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t dir_port = dw_get_dir_port(port_base_addr);
	bool pin_is_output, need_debounce;

	/* Set init value then direction */
	pin_is_output = (flags & GPIO_OUTPUT) != 0U;
	if (pin_is_output) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			gpio_dw_port_set_bits_raw(port, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			gpio_dw_port_clear_bits_raw(port, BIT(pin));
		}
	}

	dw_set_bit(port_base_addr, dir_port, pin, pin_is_output);

	/* Use built-in debounce.
	 * Note debounce circuit is only available if also supporting
	 * interrupts according to datasheet.
	 */
	if (dw_interrupt_support(config) && (dir_port == SWPORTA_DDR)) {
		need_debounce = (flags & GPIO_INT_DEBOUNCE);
		dw_set_bit(base_addr, PORTA_DEBOUNCE, pin, need_debounce);
	}
}

static inline int gpio_dw_config(const struct device *port,
				 gpio_pin_t pin,
				 gpio_flags_t flags)
{
	const struct gpio_dw_config *config = port->config;
	uint32_t io_flags;

	/* Check for invalid pin number */
	if (pin >= config->bits) {
		return -EINVAL;
	}

	/* Does not support disconnected pin, and
	 * not supporting both input/output at same time.
	 */
	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if ((io_flags == GPIO_DISCONNECTED)
	    || (io_flags == (GPIO_INPUT | GPIO_OUTPUT))) {
		return -ENOTSUP;
	}

	/* No open-drain support */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	/* Does not support pull-up/pull-down */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	dw_pin_config(port, pin, flags);

	return 0;
}

static int gpio_dw_port_get_raw(const struct device *port, uint32_t *value)
{
	struct gpio_dw_runtime *context = port->data;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t ext_port = dw_get_ext_port(port_base_addr);

	*value = dw_read(base_addr, ext_port);

	return 0;
}

static int gpio_dw_port_set_masked_raw(const struct device *port,
				       uint32_t mask, uint32_t value)
{
	struct gpio_dw_runtime *context = port->data;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t data_port = dw_get_data_port(port_base_addr);
	uint32_t pins;

	pins = dw_read(base_addr, data_port);
	pins = (pins & ~mask) | (mask & value);
	dw_write(base_addr, data_port, pins);

	return 0;
}

static int gpio_dw_port_set_bits_raw(const struct device *port, uint32_t mask)
{
	struct gpio_dw_runtime *context = port->data;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t data_port = dw_get_data_port(port_base_addr);
	uint32_t pins;

	pins = dw_read(base_addr, data_port);
	pins |= mask;
	dw_write(base_addr, data_port, pins);

	return 0;
}

static int gpio_dw_port_clear_bits_raw(const struct device *port,
				       uint32_t mask)
{
	struct gpio_dw_runtime *context = port->data;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t data_port = dw_get_data_port(port_base_addr);
	uint32_t pins;

	pins = dw_read(base_addr, data_port);
	pins &= ~mask;
	dw_write(base_addr, data_port, pins);

	return 0;
}

static int gpio_dw_port_toggle_bits(const struct device *port, uint32_t mask)
{
	struct gpio_dw_runtime *context = port->data;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t port_base_addr = context->base_addr;
	uint32_t data_port = dw_get_data_port(port_base_addr);
	uint32_t pins;

	pins = dw_read(base_addr, data_port);
	pins ^= mask;
	dw_write(base_addr, data_port, pins);

	return 0;
}

static inline int gpio_dw_manage_callback(const struct device *port,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_dw_runtime *context = port->data;

	return gpio_manage_callback(&context->callbacks, callback, set);
}

#ifdef CONFIG_PM_DEVICE
static void gpio_dw_set_power_state(const struct device *port,
				    uint32_t power_state)
{
	struct gpio_dw_runtime *context = port->data;

	context->device_power_state = power_state;
}

static uint32_t gpio_dw_get_power_state(const struct device *port)
{
	struct gpio_dw_runtime *context = port->data;

	return context->device_power_state;
}

static inline int gpio_dw_suspend_port(const struct device *port)
{
	gpio_dw_clock_off(port);
	gpio_dw_set_power_state(port, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static inline int gpio_dw_resume_from_suspend_port(const struct device *port)
{
	gpio_dw_clock_on(port);
	gpio_dw_set_power_state(port, DEVICE_PM_ACTIVE_STATE);
	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int gpio_dw_device_ctrl(const struct device *port,
			       uint32_t ctrl_command,
			       void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			ret = gpio_dw_suspend_port(port);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			ret = gpio_dw_resume_from_suspend_port(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = gpio_dw_get_power_state(port);
	}

	if (cb) {
		cb(port, ret, context, arg);
	}
	return ret;
}

#else
#define gpio_dw_set_power_state(...)
#endif

#define gpio_dw_unmask_int(...)

static void gpio_dw_isr(const struct device *port)
{
	struct gpio_dw_runtime *context = port->data;
	uint32_t base_addr = dw_base_to_block_base(context->base_addr);
	uint32_t int_status;

	int_status = dw_read(base_addr, INTSTATUS);

#ifdef CONFIG_SHARED_IRQ
	/* If using with shared IRQ, this function will be called
	 * by the shared IRQ driver. So check here if the interrupt
	 * is coming from the GPIO controller (or somewhere else).
	 */
	if (!int_status) {
		return;
	}
#endif

	dw_write(base_addr, PORTA_EOI, int_status);

	gpio_fire_callbacks(&context->callbacks, port, int_status);
}

static const struct gpio_driver_api api_funcs = {
	.pin_configure = gpio_dw_config,
	.port_get_raw = gpio_dw_port_get_raw,
	.port_set_masked_raw = gpio_dw_port_set_masked_raw,
	.port_set_bits_raw = gpio_dw_port_set_bits_raw,
	.port_clear_bits_raw = gpio_dw_port_clear_bits_raw,
	.port_toggle_bits = gpio_dw_port_toggle_bits,
	.pin_interrupt_configure = gpio_dw_pin_interrupt_configure,
	.manage_callback = gpio_dw_manage_callback,
};

static int gpio_dw_initialize(const struct device *port)
{
	struct gpio_dw_runtime *context = port->data;
	const struct gpio_dw_config *config = port->config;
	uint32_t base_addr;

	if (dw_interrupt_support(config)) {

		base_addr = dw_base_to_block_base(context->base_addr);

		/* interrupts in sync with system clock */
		dw_set_bit(base_addr, INT_CLOCK_SYNC, LS_SYNC_POS, 1);

		gpio_dw_clock_config(port);

		/* mask and disable interrupts */
		dw_write(base_addr, INTMASK, ~(0));
		dw_write(base_addr, INTEN, 0);
		dw_write(base_addr, PORTA_EOI, ~(0));

		config->config_func(port);
	}

	gpio_dw_set_power_state(port, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/* Bindings to the plaform */
#ifdef CONFIG_GPIO_DW_0
static void gpio_config_0_irq(const struct device *port);

static const struct gpio_dw_config gpio_config_0 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	},
#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
	.irq_num = DT_INST_IRQN(0),
#endif
	.bits = DT_INST_PROP(0, bits),
	.config_func = gpio_config_0_irq,
#ifdef CONFIG_GPIO_DW_0_IRQ_SHARED
	.shared_irq_dev_name = DT_INST_IRQ_BY_NAME(0, shared_name, irq),
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_0_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_0_runtime = {
	.base_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0,
	      gpio_dw_initialize, gpio_dw_device_ctrl, &gpio_0_runtime,
	      &gpio_config_0, POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
	      &api_funcs);

#if DT_INST_IRQ_HAS_CELL(0, flags)
#define INST_0_IRQ_FLAGS DT_INST_IRQ(0, flags)
#else
#define INST_0_IRQ_FLAGS 0
#endif
static void gpio_config_0_irq(const struct device *port)
{
#if (DT_INST_IRQN(0) > 0)
	const struct gpio_dw_config *config = port->config;

#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), gpio_dw_isr,
		    DEVICE_DT_INST_GET(0),
		    INST_0_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_0_IRQ_SHARED)
	const struct device *shared_irq_dev;

	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	__ASSERT(shared_irq_dev != NULL,
		 "Failed to get gpio_dw_0 device binding");
	shared_irq_isr_register(shared_irq_dev, (isr_t)gpio_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
	gpio_dw_unmask_int(GPIO_DW_PORT_0_INT_MASK);
#endif
}

#endif /* CONFIG_GPIO_DW_0 */


#ifdef CONFIG_GPIO_DW_1
static void gpio_config_1_irq(const struct device *port);

static const struct gpio_dw_config gpio_dw_config_1 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(1),
	},
#ifdef CONFIG_GPIO_DW_1_IRQ_DIRECT
	.irq_num = DT_INST_IRQN(1),
#endif
	.bits = DT_INST_PROP(1, bits),
	.config_func = gpio_config_1_irq,

#ifdef CONFIG_GPIO_DW_1_IRQ_SHARED
	.shared_irq_dev_name = DT_INST_IRQ_BY_NAME(1, shared_name, irq),
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_1_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_1_runtime = {
	.base_addr = DT_INST_REG_ADDR(1),
};

DEVICE_DT_INST_DEFINE(1,
	      gpio_dw_initialize, gpio_dw_device_ctrl, &gpio_1_runtime,
	      &gpio_dw_config_1, POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
	      &api_funcs);

#if DT_INST_IRQ_HAS_CELL(1, flags)
#define INST_1_IRQ_FLAGS DT_INST_IRQ(1, flags)
#else
#define INST_1_IRQ_FLAGS 0
#endif
static void gpio_config_1_irq(const struct device *port)
{
#if (DT_INST_IRQN(1) > 0)
	const struct gpio_dw_config *config = port->config;

#ifdef CONFIG_GPIO_DW_1_IRQ_DIRECT
	IRQ_CONNECT(DT_INST_IRQN(1),
		    DT_INST_IRQ(1, priority), gpio_dw_isr,
		    DEVICE_DT_INST_GET(1),
		    INST_1_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_1_IRQ_SHARED)
	const struct device *shared_irq_dev;

	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	__ASSERT(shared_irq_dev != NULL,
		 "Failed to get gpio_dw_1 device binding");
	shared_irq_isr_register(shared_irq_dev, (isr_t)gpio_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
	gpio_dw_unmask_int(GPIO_DW_PORT_1_INT_MASK);
#endif
}

#endif /* CONFIG_GPIO_DW_1 */

#ifdef CONFIG_GPIO_DW_2
static void gpio_config_2_irq(const struct device *port);

static const struct gpio_dw_config gpio_dw_config_2 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(2),
	},
#ifdef CONFIG_GPIO_DW_2_IRQ_DIRECT
	.irq_num = DT_INST_IRQN(2),
#endif
	.bits = DT_INST_PROP(2, bits),
	.config_func = gpio_config_2_irq,

#ifdef CONFIG_GPIO_DW_2_IRQ_SHARED
	.shared_irq_dev_name = DT_INST_IRQ_BY_NAME(2, shared_name, irq),
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_2_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_2_runtime = {
	.base_addr = DT_INST_REG_ADDR(2),
};

DEVICE_DT_INST_DEFINE(2,
	      gpio_dw_initialize, gpio_dw_device_ctrl, &gpio_2_runtime,
	      &gpio_dw_config_2, POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
	      &api_funcs);

#if DT_INST_IRQ_HAS_CELL(2, flags)
#define INST_2_IRQ_FLAGS DT_INST_IRQ(2, flags)
#else
#define INST_2_IRQ_FLAGS 0
#endif
static void gpio_config_2_irq(const struct device *port)
{
#if (DT_INST_IRQN(2) > 0)
	const struct gpio_dw_config *config = port->config;

#ifdef CONFIG_GPIO_DW_2_IRQ_DIRECT
	IRQ_CONNECT(DT_INST_IRQN(2),
		    DT_INST_IRQ(2, priority), gpio_dw_isr,
		    DEVICE_DT_INST_GET(2),
		    INST_2_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_2_IRQ_SHARED)
	const struct device *shared_irq_dev;

	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	__ASSERT(shared_irq_dev != NULL,
		 "Failed to get gpio_dw_2 device binding");
	shared_irq_isr_register(shared_irq_dev, (isr_t)gpio_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
	gpio_dw_unmask_int(GPIO_DW_PORT_2_INT_MASK);
#endif
}

#endif /* CONFIG_GPIO_DW_2 */

#ifdef CONFIG_GPIO_DW_3
static void gpio_config_3_irq(const struct device *port);

static const struct gpio_dw_config gpio_dw_config_3 = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(3),
	},
#ifdef CONFIG_GPIO_DW_3_IRQ_DIRECT
	.irq_num = DT_INST_IRQN(3),
#endif
	.bits = DT_INST_PROP(3, bits),
	.config_func = gpio_config_3_irq,

#ifdef CONFIG_GPIO_DW_3_IRQ_SHARED
	.shared_irq_dev_name = DT_INST_IRQ_BY_NAME(3, shared_name, irq),
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_3_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_3_runtime = {
	.base_addr = DT_INST_REG_ADDR(3),
};

DEVICE_DT_INST_DEFINE(3,
	      gpio_dw_initialize, gpio_dw_device_ctrl, &gpio_3_runtime,
	      &gpio_dw_config_3, POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
	      &api_funcs);

#if DT_INST_IRQ_HAS_CELL(3, flags)
#define INST_3_IRQ_FLAGS DT_INST_IRQ(3, flags)
#else
#define INST_3_IRQ_FLAGS 0
#endif
static void gpio_config_3_irq(const struct device *port)
{
#if (DT_INST_IRQN(3) > 0)
	const struct gpio_dw_config *config = port->config;

#ifdef CONFIG_GPIO_DW_3_IRQ_DIRECT
	IRQ_CONNECT(DT_INST_IRQN(3),
		    DT_INST_IRQ(3, priority), gpio_dw_isr,
		    DEVICE_DT_INST_GET(3),
		    INST_3_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_3_IRQ_SHARED)
	const struct device *shared_irq_dev;

	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	__ASSERT(shared_irq_dev != NULL,
			 "Failed to get gpio_dw_3 device binding");
	shared_irq_isr_register(shared_irq_dev, (isr_t)gpio_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
	gpio_dw_unmask_int(GPIO_DW_PORT_3_INT_MASK);
#endif
}
#endif /* CONFIG_GPIO_DW_3 */
