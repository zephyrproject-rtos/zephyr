/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rafw_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/sys_io.h>

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include "bsp_pd.h"

#define GPIO_PUPD_INPUT    0
#define GPIO_PUPD_INPUT_PU 1
#define GPIO_PUPD_INPUT_PD 2
#define GPIO_PUPD_OUTPUT   3

/*
 * GPIO Px share single GPIO and WKUP peripheral instance with separate
 * set registers for Px interleaved. The starting registers for direct
 * data access, bit access, mode, latch and wake-up controller are defined in
 * device tree.
 */
#define GPIO_RENESAS_RAFW_DATA_REG_DATA_OFFSET     0x00U
#define GPIO_RENESAS_RAFW_DATA_REG_SET_OFFSET      0x0CU
#define GPIO_RENESAS_RAFW_DATA_REG_RESET_OFFSET    0x18U
#define GPIO_RENESAS_RAFW_LATCH_REG_LATCH_OFFSET   0x00U
#define GPIO_RENESAS_RAFW_LATCH_REG_SET_OFFSET     0x04U
#define GPIO_RENESAS_RAFW_LATCH_REG_RESET_OFFSET   0x08U
#define GPIO_RENESAS_RAFW_WKUP_REG_POL_OFFSET      0x00U
#define GPIO_RENESAS_RAFW_WKUP_REG_SELECT_OFFSET   0x0CU
#define GPIO_RENESAS_RAFW_WKUP_REG_STATUS_OFFSET   0x18U
#define GPIO_RENESAS_RAFW_WKUP_REG_SEL_OFFSET      0x24U
#define GPIO_RENESAS_RAFW_WKUP_REG_EDGE_EN_OFFSET  0x30U

#define GPIO_RENESAS_RAFW_CLK_PD_SLP_REG_OFFSET    0x18U

static inline uint32_t gpio_renesas_rafw_read_reg(const volatile uint32_t *base, size_t offset)
{
	return sys_read32((mem_addr_t)((uintptr_t)base + offset));
}

static inline void gpio_renesas_rafw_write_reg(volatile uint32_t *base, size_t offset,
									 uint32_t value)
{
	sys_write32(value, (mem_addr_t)((uintptr_t)base + offset));
}

struct gpio_renesas_rafw_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* Pins that are configured for both edges (handled by software) */
	gpio_port_pins_t both_edges_pins;
	sys_slist_t callbacks;
};

struct gpio_renesas_rafw_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	volatile uint32_t *data_regs;
	volatile uint32_t *mode_regs;
	volatile uint32_t *wkup_regs;
	volatile uint32_t *latch_regs;
	uint8_t port;
};

static int gpio_renesas_rafw_pin_configure(const struct device *dev, gpio_pin_t pin,
					   gpio_flags_t flags)
{
	const struct gpio_renesas_rafw_config *config = dev->config;

	if (flags == GPIO_DISCONNECTED) {
		/* Set pin as input with no resistors selected */
		config->mode_regs[pin] = GPIO_PUPD_INPUT << GPIO_P0_00_MODE_REG_PUPD_Pos;
		gpio_renesas_rafw_write_reg(config->latch_regs,
					   GPIO_RENESAS_RAFW_LATCH_REG_RESET_OFFSET,
					   BIT(pin));

		return 0;
	}

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		/* Simultaneous in/out is not supported */
		return -ENOTSUP;
	}

	gpio_renesas_rafw_write_reg(config->latch_regs,
				   GPIO_RENESAS_RAFW_LATCH_REG_SET_OFFSET,
				   BIT(pin));

	if (flags & GPIO_OUTPUT) {
		config->mode_regs[pin] = GPIO_PUPD_OUTPUT << GPIO_P0_00_MODE_REG_PUPD_Pos;

		if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_renesas_rafw_write_reg(config->data_regs,
						   GPIO_RENESAS_RAFW_DATA_REG_RESET_OFFSET,
						   BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_renesas_rafw_write_reg(config->data_regs,
						   GPIO_RENESAS_RAFW_DATA_REG_SET_OFFSET,
						   BIT(pin));
		}

		return 0;
	}

	if (flags & GPIO_PULL_DOWN) {
		config->mode_regs[pin] = GPIO_PUPD_INPUT_PD << GPIO_P0_00_MODE_REG_PUPD_Pos;
	} else if (flags & GPIO_PULL_UP) {
		config->mode_regs[pin] = GPIO_PUPD_INPUT_PU << GPIO_P0_00_MODE_REG_PUPD_Pos;
	} else {
		config->mode_regs[pin] = GPIO_PUPD_INPUT << GPIO_P0_00_MODE_REG_PUPD_Pos;
	}

	return 0;
}

static int gpio_renesas_rafw_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_renesas_rafw_config *config = dev->config;

	*value = gpio_renesas_rafw_read_reg(config->data_regs,
					   GPIO_RENESAS_RAFW_DATA_REG_DATA_OFFSET);

	return 0;
}

static int gpio_renesas_rafw_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						 gpio_port_value_t value)
{
	const struct gpio_renesas_rafw_config *config = dev->config;
	uint32_t val = value & mask;
	uint32_t clear = ~value & mask;

	gpio_renesas_rafw_write_reg(config->data_regs,
				   GPIO_RENESAS_RAFW_DATA_REG_SET_OFFSET,
				   val);
	gpio_renesas_rafw_write_reg(config->data_regs,
				   GPIO_RENESAS_RAFW_DATA_REG_RESET_OFFSET,
				   clear);

	return 0;
}

static int gpio_renesas_rafw_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_renesas_rafw_config *config = dev->config;

	gpio_renesas_rafw_write_reg(config->data_regs,
				   GPIO_RENESAS_RAFW_DATA_REG_SET_OFFSET,
				   pins);

	return 0;
}

static int gpio_renesas_rafw_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_renesas_rafw_config *config = dev->config;

	gpio_renesas_rafw_write_reg(config->data_regs,
				   GPIO_RENESAS_RAFW_DATA_REG_RESET_OFFSET,
				   pins);

	return 0;
}

static int gpio_renesas_rafw_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_renesas_rafw_config *config = dev->config;
	uint32_t val = gpio_renesas_rafw_read_reg(config->data_regs,
						   GPIO_RENESAS_RAFW_DATA_REG_DATA_OFFSET);

	gpio_renesas_rafw_write_reg(config->data_regs,
				   GPIO_RENESAS_RAFW_DATA_REG_DATA_OFFSET,
				   val ^ mask);

	return 0;
}

static void gpio_renesas_rafw_arm_next_edge_interrupt(const struct device *dev, uint32_t pin_mask)
{
	const struct gpio_renesas_rafw_config *config = dev->config;
	uint32_t pin_value;
	uint32_t pol;

	do {
		pin_value = gpio_renesas_rafw_read_reg(config->data_regs,
						GPIO_RENESAS_RAFW_DATA_REG_DATA_OFFSET) & pin_mask;
		pol = gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_POL_OFFSET);
		if (pin_value) {
			pol |= pin_mask;
		} else {
			pol &= ~pin_mask;
		}
		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_POL_OFFSET,
					    pol);
	} while (pin_value != (gpio_renesas_rafw_read_reg(config->data_regs,
					GPIO_RENESAS_RAFW_DATA_REG_DATA_OFFSET) & pin_mask));
}

static int gpio_renesas_rafw_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						     enum gpio_int_mode mode,
						     enum gpio_int_trig trig)
{
	const struct gpio_renesas_rafw_config *config = dev->config;
	struct gpio_renesas_rafw_data *data = dev->data;
	uint32_t pin_mask = BIT(pin);

	/* Not supported by hardware */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SEL_OFFSET,
					    gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SEL_OFFSET) & ~pin_mask);
		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SELECT_OFFSET,
					    gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SELECT_OFFSET) & ~pin_mask);
		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_STATUS_OFFSET,
					    pin_mask);
	} else {
		uint32_t pol = gpio_renesas_rafw_read_reg(config->wkup_regs,
					      GPIO_RENESAS_RAFW_WKUP_REG_POL_OFFSET);

		if (trig == GPIO_INT_TRIG_BOTH) {
			/* Not supported by hardware */
			data->both_edges_pins |= pin_mask;
			gpio_renesas_rafw_arm_next_edge_interrupt(dev, pin_mask);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			pol &= ~pin_mask;
		} else {
			pol |= pin_mask;
		}
		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_POL_OFFSET,
					    pol);

		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SEL_OFFSET,
					    gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SEL_OFFSET) | pin_mask);
		gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SELECT_OFFSET,
					    gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_SELECT_OFFSET) | pin_mask);

		if ((mode & GPIO_INT_EDGE) != 0)  {
			gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_EDGE_EN_OFFSET,
					    gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_EDGE_EN_OFFSET) | pin_mask);
		} else {
			gpio_renesas_rafw_write_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_EDGE_EN_OFFSET,
					    gpio_renesas_rafw_read_reg(config->wkup_regs,
					    GPIO_RENESAS_RAFW_WKUP_REG_EDGE_EN_OFFSET) & ~pin_mask);
		}
	}

	return 0;
}

static int gpio_renesas_rafw_manage_callback(const struct device *dev,
					     struct gpio_callback *callback, bool set)
{
	struct gpio_renesas_rafw_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_renesas_rafw_isr(const struct device *dev)
{
	const struct gpio_renesas_rafw_config *config = dev->config;
	struct gpio_renesas_rafw_data *data = dev->data;
	uint32_t stat;
	uint32_t two_edge_triggered;

	stat = gpio_renesas_rafw_read_reg(config->wkup_regs,
					     GPIO_RENESAS_RAFW_WKUP_REG_STATUS_OFFSET);

	two_edge_triggered = stat & data->both_edges_pins;
	while (two_edge_triggered) {
		int pos = find_lsb_set(two_edge_triggered) - 1;

		two_edge_triggered &= ~BIT(pos);
		/* Re-arm for other edge */
		gpio_renesas_rafw_arm_next_edge_interrupt(dev, BIT(pos));
	}

	gpio_renesas_rafw_write_reg(config->wkup_regs,
				    GPIO_RENESAS_RAFW_WKUP_REG_STATUS_OFFSET,
				    stat);
	R_BSP_IrqStatusClear(R_FSP_CurrentIrqGet());

	gpio_fire_callbacks(&data->callbacks, dev, stat);
}

static void gpio_renesas_rafw_enable_wakeup_controller(void)
{
	uint32_t key = irq_lock();
	uint32_t reg_value = gpio_renesas_rafw_read_reg((volatile uint32_t *)CRG_TOP_BASE,
							GPIO_RENESAS_RAFW_CLK_PD_SLP_REG_OFFSET);

	reg_value |= 1;
	gpio_renesas_rafw_write_reg((volatile uint32_t *)CRG_TOP_BASE,
				    GPIO_RENESAS_RAFW_CLK_PD_SLP_REG_OFFSET,
				    reg_value);
	irq_unlock(key);
}

#define ENABLE_GPIO_POWER_DOMAIN(pd) bsp_pd_use((pd))
#define DISABLE_GPIO_POWER_DOMAIN(pd) bsp_pd_unuse((pd))

/* GPIO driver registration */
static DEVICE_API(gpio, gpio_renesas_rafw_drv_api_funcs) = {
	.pin_configure = gpio_renesas_rafw_pin_configure,
	.port_get_raw = gpio_renesas_rafw_port_get_raw,
	.port_set_masked_raw = gpio_renesas_rafw_port_set_masked_raw,
	.port_set_bits_raw = gpio_renesas_rafw_port_set_bits_raw,
	.port_clear_bits_raw = gpio_renesas_rafw_port_clear_bits_raw,
	.port_toggle_bits = gpio_renesas_rafw_port_toggle_bits,
	.pin_interrupt_configure = gpio_renesas_rafw_pin_interrupt_configure,
	.manage_callback = gpio_renesas_rafw_manage_callback,
};

#define _ICU_EVENT_WKUP_PX_IRQ(port) ICU_EVENT_WKUPCW_P##port##_IRQ
#define ICU_EVENT_WKUP_PX_IRQ(port) _ICU_EVENT_WKUP_PX_IRQ(port)

#define ENABLE_ICU_EVENTS(id)									   \
		volatile uint32_t *ielsr_reg = ICU_IELSRn_REG(DT_INST_IRQN(id));		   \
												   \
		*ielsr_reg = ICU_EVENT_WKUP_PX_IRQ(DT_INST_PROP(id, port));

#define GPIO_RENESAS_RAFW_DEVICE(id)								   \
	static const struct gpio_renesas_rafw_config gpio_renesas_rafw_config_##id = {		   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(id),					   \
		.data_regs = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(id, data),		   \
		.mode_regs = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(id, mode),		   \
		.latch_regs = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(id, latch),		   \
		.wkup_regs = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(id, wkup),		   \
		.port = DT_INST_PROP(id, port),							   \
	};											   \
												   \
	static struct gpio_renesas_rafw_data gpio_renesas_rafw_data_##id;			   \
												   \
	static int gpio_renesas_rafw_init_##id(const struct device *dev)			   \
	{											   \
		ENABLE_GPIO_POWER_DOMAIN(BSP_PD_COM);						   \
		gpio_renesas_rafw_enable_wakeup_controller();					   \
		IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(icu)),					   \
			(BUILD_ASSERT(DT_INST_IRQ(id, irq) < CONFIG_NUM_IRQS,			   \
			"Please configure a valid ICU event in the interrupts property");))	   \
		ENABLE_ICU_EVENTS(id);								   \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), gpio_renesas_rafw_isr,	   \
			DEVICE_DT_INST_GET(id), 0);						   \
		irq_enable(DT_INST_IRQN(id));							   \
		return 0;									   \
	}											   \
												   \
	DEVICE_DT_INST_DEFINE(id, gpio_renesas_rafw_init_##id, NULL,				   \
			&gpio_renesas_rafw_data_##id,						   \
			&gpio_renesas_rafw_config_##id, PRE_KERNEL_1,				   \
			CONFIG_GPIO_INIT_PRIORITY, &gpio_renesas_rafw_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RENESAS_RAFW_DEVICE)
