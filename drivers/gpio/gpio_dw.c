/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <nanokernel.h>
#include <gpio.h>
#include "gpio_dw.h"
#include "gpio_utils.h"
#include "gpio_api_compat.h"

#include <board.h>
#include <sys_io.h>
#include <init.h>
#include <misc/util.h>
#include <misc/__assert.h>
#include <clock_control.h>

#ifdef CONFIG_SHARED_IRQ
#include <shared_irq.h>
#endif

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#include <power.h>
#endif

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
		       uint32_t bit, uint8_t value)
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
		       uint32_t bit, uint8_t value)
{
	if (!value) {
		sys_clear_bit(base_addr + offset, bit);
	} else {
		sys_set_bit(base_addr + offset, bit);
	}
}
#endif

#ifdef CONFIG_GPIO_DW_CLOCK_GATE
static inline void _gpio_dw_clock_config(struct device *port)
{
	char *drv = CONFIG_GPIO_DW_CLOCK_GATE_DRV_NAME;
	struct device *clk;

	clk = device_get_binding(drv);
	if (clk) {
		struct gpio_dw_runtime *context = port->driver_data;

		context->clock = clk;
	}
}

static inline void _gpio_dw_clock_on(struct device *port)
{
	struct gpio_dw_config *config = port->config->config_info;
	struct gpio_dw_runtime *context = port->driver_data;

	clock_control_on(context->clock, config->clock_data);
}

static inline void _gpio_dw_clock_off(struct device *port)
{
	struct gpio_dw_config *config = port->config->config_info;
	struct gpio_dw_runtime *context = port->driver_data;

	clock_control_off(context->clock, config->clock_data);
}
#else
#define _gpio_dw_clock_config(...)
#define _gpio_dw_clock_on(...)
#define _gpio_dw_clock_off(...)
#endif

#ifdef CONFIG_SOC_QUARK_SE_SS
static inline void dw_set_both_edges(uint32_t base_addr, uint32_t pin)
{
	ARG_UNUSED(base_addr);
	ARG_UNUSED(pin);
}
static inline int dw_base_to_block_base(uint32_t base_addr)
{
	return base_addr;
}
static inline int dw_interrupt_support(struct gpio_dw_config *config)
{
	ARG_UNUSED(config);
	return 1;
}
#else
static inline void dw_set_both_edges(uint32_t base_addr, uint32_t pin)
{
	dw_set_bit(base_addr, INT_BOTHEDGE, pin, 1);
}
static inline int dw_base_to_block_base(uint32_t base_addr)
{
	return (base_addr & 0xFFFFFFC0);
}
static inline int dw_derive_port_from_base(uint32_t base_addr)
{
	uint32_t port = (base_addr & 0x3f) / 12;
	return port;
}
static inline int dw_interrupt_support(struct gpio_dw_config *config)
{
	return ((int)(config->irq_num) > 0);
}
#endif

static inline void dw_interrupt_config(struct device *port, int access_op,
				       uint32_t pin, int flags)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = dw_base_to_block_base(config->base_addr);
	uint8_t flag_is_set;

	/* set as an input pin */
	dw_set_bit(config->base_addr, SWPORTA_DDR, pin, 0);

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
				 uint32_t pin, int flags)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = dw_base_to_block_base(config->base_addr);
	uint32_t port_base_addr = config->base_addr;
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
	struct gpio_dw_config *config = port->config->config_info;
	int i;

	for (i = 0; i < config->bits; i++) {
		dw_pin_config(port, i, flags);
	}
}

static inline int gpio_dw_config(struct device *port, int access_op,
				 uint32_t pin, int flags)
{
	if (((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) ||
	    ((flags & GPIO_DIR_IN) && (flags & GPIO_DIR_OUT))) {
		return -1;
	}

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_pin_config(port, pin, flags);
	} else {
		dw_port_config(port, flags);
	}
	return 0;
}

static inline int gpio_dw_write(struct device *port, int access_op,
				uint32_t pin, uint32_t value)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_set_bit(base_addr, SWPORTA_DR, pin, value);
	} else {
		dw_write(base_addr, SWPORTA_DR, value);
	}

	return 0;
}

static inline int gpio_dw_read(struct device *port, int access_op,
				    uint32_t pin, uint32_t *value)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;
#ifndef CONFIG_SOC_QUARK_SE_SS
	uint32_t ext_port = EXT_PORTA;
#endif

#ifdef CONFIG_SOC_QUARK_SE_SS
	*value = dw_read(base_addr, EXT_PORTA);
#else
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
#endif

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

	_gpio_manage_callback(&context->callbacks, callback, set);

	return 0;
}

static inline int gpio_dw_enable_callback(struct device *port, int access_op,
					  uint32_t pin)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = dw_base_to_block_base(config->base_addr);

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_write(base_addr, PORTA_EOI, BIT(pin));
		dw_set_bit(base_addr, INTMASK, pin, 0);
		_gpio_enable_callback(port, BIT(pin));
	} else {
		dw_write(base_addr, PORTA_EOI, BIT_MASK(config->bits));
		dw_write(base_addr, INTMASK, 0);
		_gpio_enable_callback(port, BIT_MASK(config->bits));
	}

	return 0;
}

static inline int gpio_dw_disable_callback(struct device *port, int access_op,
					   uint32_t pin)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = dw_base_to_block_base(config->base_addr);

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_set_bit(base_addr, INTMASK, pin, 1);
		_gpio_disable_callback(port, BIT(pin));
	} else {
		dw_write(base_addr, INTMASK, BIT_MASK(config->bits));
		_gpio_disable_callback(port, BIT_MASK(config->bits));
	}

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static inline int gpio_dw_suspend_port(struct device *port, int pm_policy)
{
	_gpio_dw_clock_off(port);

	return 0;
}

static inline int gpio_dw_resume_port(struct device *port, int pm_policy)
{
	_gpio_dw_clock_on(port);

	return 0;
}
#endif

#if defined(CONFIG_SOC_QUARK_SE) || defined(CONFIG_SOC_QUARK_D2000)
static inline void gpio_dw_unmask_int(uint32_t mask_addr)
{
	sys_write32(sys_read32(mask_addr) & INT_UNMASK_IA, mask_addr);
}
#elif CONFIG_SOC_QUARK_SE_SS
static inline void gpio_dw_unmask_int(uint32_t mask_addr)
{
	sys_write32(sys_read32(mask_addr) & INT_ENABLE_ARC, mask_addr);
}
#else
#define gpio_dw_unmask_int(...)
#endif

void gpio_dw_isr(void *arg)
{
	struct device *port = (struct device *)arg;
	struct gpio_dw_runtime *context = port->driver_data;
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr = dw_base_to_block_base(config->base_addr);
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

	_gpio_fire_callbacks(&context->callbacks, port, int_status);
}

static struct gpio_driver_api api_funcs = {
	.config = gpio_dw_config,
	.write = gpio_dw_write,
	.read = gpio_dw_read,
	.manage_callback = gpio_dw_manage_callback,
	.enable_callback = gpio_dw_enable_callback,
	.disable_callback = gpio_dw_disable_callback,
};

#ifdef CONFIG_PCI
static inline int gpio_dw_setup(struct device *dev)
{
	struct gpio_dw_config *config = dev->config->config_info;

	pci_bus_scan_init();

	if (!pci_bus_scan(&config->pci_dev)) {
		return 0;
	}

#ifdef CONFIG_PCI_ENUMERATION
	config->base_addr = config->pci_dev.addr;
	config->irq_num = config->pci_dev.irq;
#endif
	pci_enable_regs(&config->pci_dev);

	pci_show(&config->pci_dev);

	return 1;
}
#else
#define gpio_dw_setup(_unused_) (1)
#endif /* CONFIG_PCI */


int gpio_dw_initialize(struct device *port)
{
	struct gpio_dw_config *config = port->config->config_info;
	uint32_t base_addr;

	if (!gpio_dw_setup(port)) {
		port->driver_api = NULL;
		return -EPERM;
	}

	if (dw_interrupt_support(config)) {

		base_addr = dw_base_to_block_base(config->base_addr);
#ifdef CONFIG_SOC_QUARK_SE_SS
		/* Need to enable clock for GPIO controller */
		dw_set_bit(base_addr, INT_CLOCK_SYNC, CLK_ENA_POS, 1);
#endif /* CONFIG_SOC_QUARK_SE_SS */

		/* interrupts in sync with system clock */
		dw_set_bit(base_addr, INT_CLOCK_SYNC, LS_SYNC_POS, 1);

		_gpio_dw_clock_config(port);

		/* mask and disable interrupts */
		dw_write(base_addr, INTMASK, ~(0));
		dw_write(base_addr, INTEN, 0);
		dw_write(base_addr, PORTA_EOI, ~(0));

		config->config_func(port);
	}

	return 0;
}

/* Bindings to the plaform */
#if CONFIG_GPIO_DW_0
void gpio_config_0_irq(struct device *port);

struct gpio_dw_config gpio_config_0 = {
	.base_addr = GPIO_DW_0_BASE_ADDR,
	.bits = GPIO_DW_0_BITS,
#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
	.irq_num = GPIO_DW_0_IRQ,
#endif
#if CONFIG_PCI
	.pci_dev.class_type = GPIO_DW_PCI_CLASS,
	.pci_dev.bus = GPIO_DW_0_PCI_BUS,
	.pci_dev.dev = GPIO_DW_0_PCI_DEV,
	.pci_dev.vendor_id = GPIO_DW_PCI_VENDOR_ID,
	.pci_dev.device_id = GPIO_DW_PCI_DEVICE_ID,
	.pci_dev.function = GPIO_DW_0_PCI_FUNCTION,
	.pci_dev.bar = GPIO_DW_0_PCI_BAR,
#endif

	.config_func = gpio_config_0_irq,
#ifdef CONFIG_GPIO_DW_0_IRQ_SHARED
	.shared_irq_dev_name = CONFIG_GPIO_DW_0_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_0_CLOCK_GATE_SUBSYS),
#endif
};

struct gpio_dw_runtime gpio_0_runtime;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
struct device_pm_ops gpio_dev_pm_ops = {
		.suspend = gpio_dw_suspend_port,
		.resume = gpio_dw_resume_port
};

DEVICE_AND_API_INIT_PM(gpio_dw_0, CONFIG_GPIO_DW_0_NAME, gpio_dw_initialize,
		       &gpio_dev_pm_ops, &gpio_0_runtime, &gpio_config_0,
		       SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_0, CONFIG_GPIO_DW_0_NAME, gpio_dw_initialize,
		    &gpio_0_runtime, &gpio_config_0,
		    SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif
GPIO_SETUP_COMPAT_DEV(gpio_dw_0);

void gpio_config_0_irq(struct device *port)
{
#if (GPIO_DW_0_IRQ > 0)
	struct gpio_dw_config *config = port->config->config_info;
	struct device *shared_irq_dev;

#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONNECT(GPIO_DW_0_IRQ, CONFIG_GPIO_DW_0_IRQ_PRI, gpio_dw_isr,
		    DEVICE_GET(gpio_dw_0), GPIO_DW_0_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_0_IRQ_SHARED)
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


#if CONFIG_GPIO_DW_1
void gpio_config_1_irq(struct device *port);

struct gpio_dw_config gpio_dw_config_1 = {
	.base_addr = GPIO_DW_1_BASE_ADDR,
	.bits = GPIO_DW_1_BITS,
#ifdef CONFIG_GPIO_DW_1_IRQ_DIRECT
	.irq_num = GPIO_DW_1_IRQ,
#endif
#if CONFIG_PCI
	.pci_dev.class_type = GPIO_DW_PCI_CLASS,
	.pci_dev.bus = GPIO_DW_1_PCI_BUS,
	.pci_dev.dev = GPIO_DW_1_PCI_DEV,
	.pci_dev.vendor_id = GPIO_DW_PCI_VENDOR_ID,
	.pci_dev.device_id = GPIO_DW_PCI_DEVICE_ID,
	.pci_dev.function = GPIO_DW_1_PCI_FUNCTION,
	.pci_dev.bar = GPIO_DW_1_PCI_BAR,
#endif
	.config_func = gpio_config_1_irq,

#ifdef CONFIG_GPIO_DW_1_IRQ_SHARED
	.shared_irq_dev_name = CONFIG_GPIO_DW_1_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_1_CLOCK_GATE_SUBSYS),
#endif
};

struct gpio_dw_runtime gpio_1_runtime;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_AND_API_INIT_PM(gpio_dw_1, CONFIG_GPIO_DW_1_NAME, gpio_dw_initialize,
		       &gpio_dev_pm_ops, &gpio_1_runtime, &gpio_dw_config_1,
		       SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_1, CONFIG_GPIO_DW_1_NAME, gpio_dw_initialize,
		    &gpio_1_runtime, &gpio_dw_config_1,
		    SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif
GPIO_SETUP_COMPAT_DEV(gpio_dw_1);

void gpio_config_1_irq(struct device *port)
{
#if (GPIO_DW_1_IRQ > 0)
	struct gpio_dw_config *config = port->config->config_info;
	struct device *shared_irq_dev;

#ifdef CONFIG_GPIO_DW_1_IRQ_DIRECT
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONNECT(GPIO_DW_1_IRQ, CONFIG_GPIO_DW_1_IRQ_PRI, gpio_dw_isr,
		    DEVICE_GET(gpio_dw_1), GPIO_DW_1_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_1_IRQ_SHARED)
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

#if CONFIG_GPIO_DW_2
void gpio_config_2_irq(struct device *port);

struct gpio_dw_config gpio_dw_config_2 = {
	.base_addr = GPIO_DW_2_BASE_ADDR,
	.bits = GPIO_DW_2_BITS,
#ifdef CONFIG_GPIO_DW_2_IRQ_DIRECT
	.irq_num = GPIO_DW_2_IRQ,
#endif
#if CONFIG_PCI
	.pci_dev.class_type = GPIO_DW_PCI_CLASS,
	.pci_dev.bus = GPIO_DW_2_PCI_BUS,
	.pci_dev.dev = GPIO_DW_2_PCI_DEV,
	.pci_dev.vendor_id = GPIO_DW_PCI_VENDOR_ID,
	.pci_dev.device_id = GPIO_DW_PCI_DEVICE_ID,
	.pci_dev.function = GPIO_DW_2_PCI_FUNCTION,
	.pci_dev.bar = GPIO_DW_2_PCI_BAR,
#endif
	.config_func = gpio_config_2_irq,

#ifdef CONFIG_GPIO_DW_2_IRQ_SHARED
	.shared_irq_dev_name = CONFIG_GPIO_DW_2_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_2_CLOCK_GATE_SUBSYS),
#endif
};

struct gpio_dw_runtime gpio_2_runtime;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_AND_API_INIT_PM(gpio_dw_2, CONFIG_GPIO_DW_2_NAME, gpio_dw_initialize,
		       &gpio_dev_pm_ops, &gpio_2_runtime, &gpio_dw_config_2,
		       SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_2, CONFIG_GPIO_DW_2_NAME, gpio_dw_initialize,
		    &gpio_2_runtime, &gpio_dw_config_2,
		    SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif
GPIO_SETUP_COMPAT_DEV(gpio_dw_2);

void gpio_config_2_irq(struct device *port)
{
#if (GPIO_DW_2_IRQ > 0)
	struct gpio_dw_config *config = port->config->config_info;
	struct device *shared_irq_dev;

#ifdef CONFIG_GPIO_DW_2_IRQ_DIRECT
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONNECT(GPIO_DW_2_IRQ, CONFIG_GPIO_DW_2_IRQ_PRI, gpio_dw_isr,
		    DEVICE_GET(gpio_dw_2), GPIO_DW_2_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_2_IRQ_SHARED)
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

#if CONFIG_GPIO_DW_3
void gpio_config_3_irq(struct device *port);

struct gpio_dw_config gpio_dw_config_3 = {
	.base_addr = GPIO_DW_3_BASE_ADDR,
	.bits = GPIO_DW_3_BITS,
#ifdef CONFIG_GPIO_DW_3_IRQ_DIRECT
	.irq_num = GPIO_DW_3_IRQ,
#endif
#if CONFIG_PCI
	.pci_dev.class_type = GPIO_DW_PCI_CLASS,
	.pci_dev.bus = GPIO_DW_3_PCI_BUS,
	.pci_dev.dev = GPIO_DW_3_PCI_DEV,
	.pci_dev.vendor_id = GPIO_DW_PCI_VENDOR_ID,
	.pci_dev.device_id = GPIO_DW_PCI_DEVICE_ID,
	.pci_dev.function = GPIO_DW_3_PCI_FUNCTION,
	.pci_dev.bar = GPIO_DW_3_PCI_BAR,
#endif
	.config_func = gpio_config_3_irq,

#ifdef CONFIG_GPIO_DW_3_IRQ_SHARED
	.shared_irq_dev_name = CONFIG_GPIO_DW_3_IRQ_SHARED_NAME,
#endif
#ifdef CONFIG_GPIO_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_GPIO_DW_3_CLOCK_GATE_SUBSYS),
#endif
};

struct gpio_dw_runtime gpio_3_runtime;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_AND_API_INIT_PM(gpio_dw_3, CONFIG_GPIO_DW_3_NAME, gpio_dw_initialize,
		       &gpio_dev_pm_ops, &gpio_3_runtime, &gpio_dw_config_3,
		       SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		       &api_funcs);
#else
DEVICE_AND_API_INIT(gpio_dw_3, CONFIG_GPIO_DW_3_NAME, gpio_dw_initialize,
		    &gpio_3_runtime, &gpio_dw_config_3,
		    SECONDARY, CONFIG_GPIO_DW_INIT_PRIORITY,
		    &api_funcs);
#endif
GPIO_SETUP_COMPAT_DEV(gpio_dw_3);

void gpio_config_3_irq(struct device *port)
{
#if (GPIO_DW_3_IRQ > 0)
	struct gpio_dw_config *config = port->config->config_info;
	struct device *shared_irq_dev;

#ifdef CONFIG_GPIO_DW_3_IRQ_DIRECT
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONNECT(GPIO_DW_3_IRQ, CONFIG_GPIO_DW_3_IRQ_PRI, gpio_dw_isr,
			    DEVICE_GET(gpio_dw_3), GPIO_DW_3_IRQ_FLAGS);
	irq_enable(config->irq_num);
#elif defined(CONFIG_GPIO_DW_3_IRQ_SHARED)
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
