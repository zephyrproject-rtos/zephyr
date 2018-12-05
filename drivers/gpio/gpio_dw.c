/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#include <power/power.h>
#endif

/*
 * ARC architecture configure IP through IO auxiliary registers.
 * Other architectures as ARM and x86 configure IP through MMIO registers
 */
#ifdef GPIO_DW_IO_ACCESS
static inline u32_t dw_read(u32_t base_addr, u32_t offset)
{
	return sys_in32(base_addr + offset);
}

static inline void dw_write(u32_t base_addr, u32_t offset,
			    u32_t val)
{
	sys_out32(val, base_addr + offset);
}

static void dw_set_bit(u32_t base_addr, u32_t offset,
		       u32_t bit, bool value)
{
	if (!value) {
		sys_io_clear_bit(base_addr + offset, bit);
	} else {
		sys_io_set_bit(base_addr + offset, bit);
	}
}
#else
static inline u32_t dw_read(u32_t base_addr, u32_t offset)
{
	return sys_read32(base_addr + offset);
}

static inline void dw_write(u32_t base_addr, u32_t offset,
			    u32_t val)
{
	sys_write32(val, base_addr + offset);
}

static void dw_set_bit(u32_t base_addr, u32_t offset,
		       u32_t bit, bool value)
{
	if (!value) {
		sys_clear_bit(base_addr + offset, bit);
	} else {
		sys_set_bit(base_addr + offset, bit);
	}
}
#endif

#ifdef CONFIG_GPIO_DW_CLOCK_GATE
static inline void gpio_dw_clock_config(struct device *port)
{
	char *drv = CONFIG_GPIO_DW_CLOCK_GATE_DRV_NAME;
	struct device *clk;

	clk = device_get_binding(drv);
	if (clk) {
		struct gpio_dw_runtime *context = port->driver_data;

		context->clock = clk;
	}
}

static inline void gpio_dw_clock_on(struct device *port)
{
	const struct gpio_dw_config *config = port->config->config_info;
	struct gpio_dw_runtime *context = port->driver_data;

	clock_control_on(context->clock, config->clock_data);
}

static inline void gpio_dw_clock_off(struct device *port)
{
	const struct gpio_dw_config *config = port->config->config_info;
	struct gpio_dw_runtime *context = port->driver_data;

	clock_control_off(context->clock, config->clock_data);
}
#else
#define gpio_dw_clock_config(...)
#define gpio_dw_clock_on(...)
#define gpio_dw_clock_off(...)
#endif

static inline void dw_set_both_edges(u32_t base_addr, u32_t pin)
{
	dw_set_bit(base_addr, INT_BOTHEDGE, pin, 1);
}
static inline int dw_base_to_block_base(u32_t base_addr)
{
	return (base_addr & 0xFFFFFFC0);
}
static inline int dw_derive_port_from_base(u32_t base_addr)
{
	u32_t port = (base_addr & 0x3f) / 12U;
	return port;
}
static inline int dw_interrupt_support(const struct gpio_dw_config *config)
{
	return ((int)(config->irq_num) > 0);
}

static inline void dw_interrupt_config(struct device *port, int access_op,
				       u32_t pin, int flags)
{
	struct gpio_dw_runtime *context = port->driver_data;
	const struct gpio_dw_config *config = port->config->config_info;
	u32_t base_addr = dw_base_to_block_base(context->base_addr);
	bool flag_is_set;

	ARG_UNUSED(access_op);

	/* set as an input pin */
	dw_set_bit(context->base_addr, SWPORTA_DDR, pin, 0);

	if (dw_interrupt_support(config)) {

		/* level or edge */
		flag_is_set = (flags & GPIO_INT_EDGE);
		dw_set_bit(base_addr, INTTYPE_LEVEL, pin, flag_is_set);

		/* Active low/high */
		flag_is_set = (flags & GPIO_INT_ACTIVE_HIGH);
		dw_set_bit(base_addr, INT_POLARITY, pin, flag_is_set);

		/* both edges */
		flag_is_set = (flags & GPIO_INT_DOUBLE_EDGE);
		if (flag_is_set) {
			dw_set_both_edges(base_addr, pin);
			dw_set_bit(base_addr, INTTYPE_LEVEL, pin, flag_is_set);
		}

		/* use built-in debounce  */
		flag_is_set = (flags & GPIO_INT_DEBOUNCE);
		dw_set_bit(base_addr, PORTA_DEBOUNCE, pin, flag_is_set);

		/* Finally enabling interrupt */
		dw_set_bit(base_addr, INTEN, pin, 1);
	}
}

static inline void dw_pin_config(struct device *port,
				 u32_t pin, int flags)
{
	struct gpio_dw_runtime *context = port->driver_data;
	const struct gpio_dw_config *config = port->config->config_info;
	u32_t base_addr = dw_base_to_block_base(context->base_addr);
	u32_t port_base_addr = context->base_addr;
	int interrupt_support = dw_interrupt_support(config);

	if (interrupt_support) {
		/* clear interrupt enable */
		dw_set_bit(base_addr, INTEN, pin, 0);
	}

	/* set direction */
	dw_set_bit(port_base_addr, SWPORTA_DDR, pin, (flags & GPIO_DIR_MASK));

	if (interrupt_support && (flags & GPIO_INT)) {
		dw_interrupt_config(port, GPIO_ACCESS_BY_PIN, pin, flags);
	}
}

static inline void dw_port_config(struct device *port, int flags)
{
	const struct gpio_dw_config *config = port->config->config_info;
	int i;

	for (i = 0; i < config->bits; i++) {
		dw_pin_config(port, i, flags);
	}
}

static inline int gpio_dw_config(struct device *port, int access_op,
				 u32_t pin, int flags)
{
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_pin_config(port, pin, flags);
	} else {
		dw_port_config(port, flags);
	}
	return 0;
}

static inline int gpio_dw_write(struct device *port, int access_op,
				u32_t pin, u32_t value)
{
	struct gpio_dw_runtime *context = port->driver_data;
	u32_t base_addr = context->base_addr;

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_set_bit(base_addr, SWPORTA_DR, pin, value);
	} else {
		dw_write(base_addr, SWPORTA_DR, value);
	}

	return 0;
}

static inline int gpio_dw_read(struct device *port, int access_op,
			       u32_t pin, u32_t *value)
{
	struct gpio_dw_runtime *context = port->driver_data;
	u32_t base_addr = context->base_addr;
	u32_t ext_port = EXT_PORTA;

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
		break;
	}
	*value = dw_read(dw_base_to_block_base(base_addr), ext_port);

	if (GPIO_ACCESS_BY_PIN == access_op) {
		*value = !!(*value & BIT(pin));
	}
	return 0;
}

static inline int gpio_dw_manage_callback(struct device *port,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_dw_runtime *context = port->driver_data;

	return gpio_manage_callback(&context->callbacks, callback, set);
}

static inline int gpio_dw_enable_callback(struct device *port, int access_op,
					  u32_t pin)
{
	const struct gpio_dw_config *config = port->config->config_info;
	struct gpio_dw_runtime *context = port->driver_data;
	u32_t base_addr = dw_base_to_block_base(context->base_addr);

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_write(base_addr, PORTA_EOI, BIT(pin));
		dw_set_bit(base_addr, INTMASK, pin, 0);
	} else {
		dw_write(base_addr, PORTA_EOI, BIT_MASK(config->bits));
		dw_write(base_addr, INTMASK, 0);
	}

	return 0;
}

static inline int gpio_dw_disable_callback(struct device *port, int access_op,
					   u32_t pin)
{
	const struct gpio_dw_config *config = port->config->config_info;
	struct gpio_dw_runtime *context = port->driver_data;
	u32_t base_addr = dw_base_to_block_base(context->base_addr);

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_set_bit(base_addr, INTMASK, pin, 1);
	} else {
		dw_write(base_addr, INTMASK, BIT_MASK(config->bits));
	}

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void gpio_dw_set_power_state(struct device *port, u32_t power_state)
{
	struct gpio_dw_runtime *context = port->driver_data;

	context->device_power_state = power_state;
}

static u32_t gpio_dw_get_power_state(struct device *port)
{
	struct gpio_dw_runtime *context = port->driver_data;

	return context->device_power_state;
}

static inline int gpio_dw_suspend_port(struct device *port)
{
	gpio_dw_clock_off(port);
	gpio_dw_set_power_state(port, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static inline int gpio_dw_resume_from_suspend_port(struct device *port)
{
	gpio_dw_clock_on(port);
	gpio_dw_set_power_state(port, DEVICE_PM_ACTIVE_STATE);
	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int gpio_dw_device_ctrl(struct device *port, u32_t ctrl_command,
			       void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			ret = gpio_dw_suspend_port(port);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			ret = gpio_dw_resume_from_suspend_port(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = gpio_dw_get_power_state(port);
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

static void gpio_dw_isr(void *arg)
{
	struct device *port = (struct device *)arg;
	struct gpio_dw_runtime *context = port->driver_data;
	u32_t base_addr = dw_base_to_block_base(context->base_addr);
	u32_t int_status;

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
	.config = gpio_dw_config,
	.write = gpio_dw_write,
	.read = gpio_dw_read,
	.manage_callback = gpio_dw_manage_callback,
	.enable_callback = gpio_dw_enable_callback,
	.disable_callback = gpio_dw_disable_callback,
};

static int gpio_dw_initialize(struct device *port)
{
	struct gpio_dw_runtime *context = port->driver_data;
	const struct gpio_dw_config *config = port->config->config_info;
	u32_t base_addr;

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
static void gpio_config_0_irq(struct device *port);

static const struct gpio_dw_config gpio_config_0 = {
#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
	.irq_num = DT_GPIO_DW_0_IRQ,
#endif
	.bits = DT_GPIO_DW_0_BITS,
	.config_func = gpio_config_0_irq,
#ifdef CONFIG_GPIO_DW_0_IRQ_SHARED
	.shared_irq_dev_name = DT_GPIO_DW_0_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_0_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_0_runtime = {
	.base_addr = DT_GPIO_DW_0_BASE_ADDR,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

DEVICE_DEFINE(gpio_dw_0, CONFIG_GPIO_DW_0_NAME, gpio_dw_initialize,
		       gpio_dw_device_ctrl, &gpio_0_runtime, &gpio_config_0,
		       POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_0, CONFIG_GPIO_DW_0_NAME, gpio_dw_initialize,
		    &gpio_0_runtime, &gpio_config_0,
		    POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif

static void gpio_config_0_irq(struct device *port)
{
#if (DT_GPIO_DW_0_IRQ > 0)
	const struct gpio_dw_config *config = port->config->config_info;

#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
	IRQ_CONNECT(DT_GPIO_DW_0_IRQ, CONFIG_GPIO_DW_0_IRQ_PRI, gpio_dw_isr,
		    DEVICE_GET(gpio_dw_0), DT_GPIO_DW_0_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_0_IRQ_SHARED)
	struct device *shared_irq_dev;

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
static void gpio_config_1_irq(struct device *port);

static const struct gpio_dw_config gpio_dw_config_1 = {
#ifdef CONFIG_GPIO_DW_1_IRQ_DIRECT
	.irq_num = DT_GPIO_DW_1_IRQ,
#endif
	.bits = DT_GPIO_DW_1_BITS,
	.config_func = gpio_config_1_irq,

#ifdef CONFIG_GPIO_DW_1_IRQ_SHARED
	.shared_irq_dev_name = DT_GPIO_DW_1_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_1_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_1_runtime = {
	.base_addr = DT_GPIO_DW_1_BASE_ADDR,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_DEFINE(gpio_dw_1, CONFIG_GPIO_DW_1_NAME, gpio_dw_initialize,
		       gpio_dw_device_ctrl, &gpio_1_runtime, &gpio_dw_config_1,
		       POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_1, CONFIG_GPIO_DW_1_NAME, gpio_dw_initialize,
		    &gpio_1_runtime, &gpio_dw_config_1,
		    POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif

static void gpio_config_1_irq(struct device *port)
{
#if (DT_GPIO_DW_1_IRQ > 0)
	const struct gpio_dw_config *config = port->config->config_info;

#ifdef CONFIG_GPIO_DW_1_IRQ_DIRECT
	IRQ_CONNECT(DT_GPIO_DW_1_IRQ, CONFIG_GPIO_DW_1_IRQ_PRI, gpio_dw_isr,
		    DEVICE_GET(gpio_dw_1), GPIO_DW_1_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_1_IRQ_SHARED)
	struct device *shared_irq_dev;

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
static void gpio_config_2_irq(struct device *port);

static const struct gpio_dw_config gpio_dw_config_2 = {
#ifdef CONFIG_GPIO_DW_2_IRQ_DIRECT
	.irq_num = DT_GPIO_DW_2_IRQ,
#endif
	.bits = DT_GPIO_DW_2_BITS,
	.config_func = gpio_config_2_irq,

#ifdef CONFIG_GPIO_DW_2_IRQ_SHARED
	.shared_irq_dev_name = DT_GPIO_DW_2_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_2_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_2_runtime = {
	.base_addr = DT_GPIO_DW_2_BASE_ADDR,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_DEFINE(gpio_dw_2, CONFIG_GPIO_DW_2_NAME, gpio_dw_initialize,
		       gpio_dw_device_ctrl, &gpio_2_runtime, &gpio_dw_config_2,
		       POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_2, CONFIG_GPIO_DW_2_NAME, gpio_dw_initialize,
		    &gpio_2_runtime, &gpio_dw_config_2,
		    POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif

static void gpio_config_2_irq(struct device *port)
{
#if (DT_GPIO_DW_2_IRQ > 0)
	const struct gpio_dw_config *config = port->config->config_info;

#ifdef CONFIG_GPIO_DW_2_IRQ_DIRECT
	IRQ_CONNECT(DT_GPIO_DW_2_IRQ, CONFIG_GPIO_DW_2_IRQ_PRI, gpio_dw_isr,
		    DEVICE_GET(gpio_dw_2), GPIO_DW_2_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_2_IRQ_SHARED)
	struct device *shared_irq_dev;

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
static void gpio_config_3_irq(struct device *port);

static const struct gpio_dw_config gpio_dw_config_3 = {
#ifdef CONFIG_GPIO_DW_3_IRQ_DIRECT
	.irq_num = DT_GPIO_DW_3_IRQ,
#endif
	.bits = DT_GPIO_DW_3_BITS,
	.config_func = gpio_config_3_irq,

#ifdef CONFIG_GPIO_DW_3_IRQ_SHARED
	.shared_irq_dev_name = DT_GPIO_DW_3_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_3_CLOCK_GATE_SUBSYS),
#endif
};

static struct gpio_dw_runtime gpio_3_runtime = {
	.base_addr = DT_GPIO_DW_3_BASE_ADDR,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_DEFINE(gpio_dw_3, CONFIG_GPIO_DW_3_NAME, gpio_dw_initialize,
		       gpio_dw_device_ctrl, &gpio_3_runtime, &gpio_dw_config_3,
		       POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_3, CONFIG_GPIO_DW_3_NAME, gpio_dw_initialize,
		    &gpio_3_runtime, &gpio_dw_config_3,
		    POST_KERNEL, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif

static void gpio_config_3_irq(struct device *port)
{
#if (DT_GPIO_DW_3_IRQ > 0)
	const struct gpio_dw_config *config = port->config->config_info;

#ifdef CONFIG_GPIO_DW_3_IRQ_DIRECT
	IRQ_CONNECT(DT_GPIO_DW_3_IRQ, CONFIG_GPIO_DW_3_IRQ_PRI, gpio_dw_isr,
			    DEVICE_GET(gpio_dw_3), GPIO_DW_3_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_3_IRQ_SHARED)
	struct device *shared_irq_dev;

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
