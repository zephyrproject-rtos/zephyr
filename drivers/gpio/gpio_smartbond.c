/*
 * Copyright (c) 2022, Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_gpio

#include <zephyr/drivers/gpio/gpio_utils.h>

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

#include <DA1469xAB.h>
#include <da1469x_pdc.h>

#define GPIO_MODE_RESET		0x200

#define GPIO_PUPD_INPUT		0
#define GPIO_PUPD_INPUT_PU	1
#define GPIO_PUPD_INPUT_PD	2
#define GPIO_PUPD_OUTPUT	3

/* GPIO P0 and P1 share single GPIO and WKUP peripheral instance with separate
 * set registers for P0 and P1 interleaved. The starting registers for direct
 * data access, bit access, mode, latch and wake-up controller are defined in
 * device tree.
 */

struct gpio_smartbond_data_regs {
	uint32_t data;
	uint32_t _reserved0;
	uint32_t set;
	uint32_t _reserved1;
	uint32_t reset;
};

struct gpio_smartbond_latch_regs {
	uint32_t latch;
	uint32_t set;
	uint32_t reset;
};

struct gpio_smartbond_wkup_regs {
	uint32_t select;
	uint32_t _reserved0[4];
	uint32_t pol;
	uint32_t _reserved1[4];
	uint32_t status;
	uint32_t _reserved2[2];
	uint32_t clear;
	uint32_t _reserved3[2];
	uint32_t sel;
};

struct gpio_smartbond_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* Pins that are configured for both edges (handled by software) */
	gpio_port_pins_t both_edges_pins;
	sys_slist_t callbacks;
};

struct gpio_smartbond_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	volatile struct gpio_smartbond_data_regs *data_regs;
	volatile uint32_t *mode_regs;
	volatile struct gpio_smartbond_latch_regs *latch_regs;
	volatile struct gpio_smartbond_wkup_regs *wkup_regs;
	/* Value of TRIG_SELECT for PDC_CTRLx_REG entry */
	uint8_t wkup_trig_select;
};

static void gpio_smartbond_wkup_init(void)
{
	static bool wkup_init;

	/* Wakeup controller is shared for both GPIO ports and should
	 * be initialized only once.
	 */
	if (!wkup_init) {
		WAKEUP->WKUP_CTRL_REG = 0;
		WAKEUP->WKUP_CLEAR_P0_REG = 0xffffffff;
		WAKEUP->WKUP_CLEAR_P1_REG = 0xffffffff;
		WAKEUP->WKUP_SELECT_P0_REG = 0;
		WAKEUP->WKUP_SELECT_P1_REG = 0;
		WAKEUP->WKUP_SEL_GPIO_P0_REG = 0;
		WAKEUP->WKUP_SEL_GPIO_P1_REG = 0;
		WAKEUP->WKUP_RESET_IRQ_REG = 0;

		CRG_TOP->CLK_TMR_REG |= CRG_TOP_CLK_TMR_REG_WAKEUPCT_ENABLE_Msk;

		WAKEUP->WKUP_CTRL_REG = WAKEUP_WKUP_CTRL_REG_WKUP_ENABLE_IRQ_Msk;

		wkup_init = true;
	}
}

static int gpio_smartbond_pin_configure(const struct device *dev,
				      gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_smartbond_config *config = dev->config;

	if (flags == GPIO_DISCONNECTED) {
		/* Reset to default value */
		config->mode_regs[pin] = GPIO_MODE_RESET;
		return 0;
	}

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		/* Simultaneous in/out is not supported */
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		config->mode_regs[pin] = GPIO_PUPD_OUTPUT << GPIO_P0_00_MODE_REG_PUPD_Pos;

		if (flags & GPIO_OUTPUT_INIT_LOW) {
			config->data_regs->reset = BIT(pin);
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			config->data_regs->set = BIT(pin);
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

static int gpio_smartbond_port_get_raw(const struct device *dev,
				     gpio_port_value_t *value)
{
	const struct gpio_smartbond_config *config = dev->config;

	*value = config->data_regs->data;

	return 0;
}

static int gpio_smartbond_port_set_masked_raw(const struct device *dev,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_smartbond_config *config = dev->config;

	config->data_regs->data = value & mask;

	return 0;
}

static int gpio_smartbond_port_set_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_smartbond_config *config = dev->config;

	config->data_regs->set = pins;

	return 0;
}

static int gpio_smartbond_port_clear_bits_raw(const struct device *dev,
					    gpio_port_pins_t pins)
{
	const struct gpio_smartbond_config *config = dev->config;

	config->data_regs->reset = pins;

	return 0;
}

static int gpio_smartbond_port_toggle_bits(const struct device *dev,
					 gpio_port_pins_t mask)
{
	const struct gpio_smartbond_config *config = dev->config;
	volatile uint32_t *reg = &config->data_regs->data;

	*reg = *reg ^ mask;

	return 0;
}

static void gpio_smartbond_arm_next_edge_interrupt(const struct device *dev,
						   uint32_t pin_mask)
{
	const struct gpio_smartbond_config *config = dev->config;
	uint32_t pin_value;

	do {
		pin_value = config->data_regs->data & pin_mask;
		if (pin_value) {
			config->wkup_regs->pol |= pin_mask;
		} else {
			config->wkup_regs->pol &= ~pin_mask;
		}
	} while (pin_value != (config->data_regs->data & pin_mask));
}

static int gpio_smartbond_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	const struct gpio_smartbond_config *config = dev->config;
	struct gpio_smartbond_data *data = dev->data;
	uint32_t pin_mask = BIT(pin);
#if CONFIG_PM
	int trig_select_id = (config->wkup_trig_select << 5) | pin;
	int pdc_ix;
#endif

	/* Not supported by hardware */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

#if CONFIG_PM
	pdc_ix = da1469x_pdc_find(trig_select_id, MCU_PDC_MASTER_M33, MCU_PDC_EN_XTAL);
#endif
	if (mode == GPIO_INT_MODE_DISABLED) {
		config->wkup_regs->sel &= ~pin_mask;
		config->wkup_regs->clear = pin_mask;
		data->both_edges_pins &= ~pin_mask;
#if CONFIG_PM
		da1469x_pdc_del(pdc_ix);
#endif
	} else {
		if (trig == GPIO_INT_TRIG_BOTH) {
			/* Not supported by hardware */
			data->both_edges_pins |= pin_mask;
			gpio_smartbond_arm_next_edge_interrupt(dev, pin_mask);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			config->wkup_regs->pol &= ~pin_mask;
		} else {
			config->wkup_regs->pol |= pin_mask;
		}

		config->wkup_regs->sel |= pin_mask;
#if CONFIG_PM
		if (pdc_ix < 0) {
			pdc_ix = da1469x_pdc_add(trig_select_id, MCU_PDC_MASTER_M33,
						 MCU_PDC_EN_XTAL);
		}
		if (pdc_ix < 0) {
			return -ENOMEM;
		}
#endif
	}

	return 0;
}

static int gpio_smartbond_manage_callback(const struct device *dev,
				       struct gpio_callback *callback, bool set)
{
	struct gpio_smartbond_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_smartbond_isr(const struct device *dev)
{
	const struct gpio_smartbond_config *config = dev->config;
	struct gpio_smartbond_data *data = dev->data;
	uint32_t stat;
	uint32_t two_edge_triggered;

	WAKEUP->WKUP_RESET_IRQ_REG = WAKEUP_WKUP_RESET_IRQ_REG_WKUP_IRQ_RST_Msk;

	stat = config->wkup_regs->status;

	two_edge_triggered = stat & data->both_edges_pins;
	while (two_edge_triggered) {
		int pos = find_lsb_set(two_edge_triggered) - 1;

		two_edge_triggered &= ~BIT(pos);
		/* Re-arm for other edge */
		gpio_smartbond_arm_next_edge_interrupt(dev, BIT(pos));
	}

	config->wkup_regs->clear = stat;

	gpio_fire_callbacks(&data->callbacks, dev, stat);
}

/* GPIO driver registration */
static const struct gpio_driver_api gpio_smartbond_drv_api_funcs = {
	.pin_configure = gpio_smartbond_pin_configure,
	.port_get_raw = gpio_smartbond_port_get_raw,
	.port_set_masked_raw = gpio_smartbond_port_set_masked_raw,
	.port_set_bits_raw = gpio_smartbond_port_set_bits_raw,
	.port_clear_bits_raw = gpio_smartbond_port_clear_bits_raw,
	.port_toggle_bits = gpio_smartbond_port_toggle_bits,
	.pin_interrupt_configure = gpio_smartbond_pin_interrupt_configure,
	.manage_callback = gpio_smartbond_manage_callback,
};

#define GPIO_SMARTBOND_DEVICE(id)							\
	static const struct gpio_smartbond_config gpio_smartbond_p##id##_config = {	\
		.common = {								\
			.port_pin_mask =						\
			GPIO_PORT_PIN_MASK_FROM_DT_INST(id),				\
		},									\
		.data_regs = (volatile struct gpio_smartbond_data_regs *)		\
						DT_INST_REG_ADDR_BY_NAME(id, data),	\
		.mode_regs = (volatile uint32_t *)DT_INST_REG_ADDR_BY_NAME(id, mode),	\
		.latch_regs = (volatile struct gpio_smartbond_latch_regs *)		\
						DT_INST_REG_ADDR_BY_NAME(id, latch),	\
		.wkup_regs = (volatile struct gpio_smartbond_wkup_regs *)		\
						DT_INST_REG_ADDR_BY_NAME(id, wkup),	\
		.wkup_trig_select = id,							\
	};										\
											\
	static struct gpio_smartbond_data gpio_smartbond_p##id##_data;			\
											\
	static int gpio_smartbond_##id##_init(const struct device *dev)			\
	{										\
		gpio_smartbond_wkup_init();						\
		IRQ_CONNECT(DT_INST_IRQN(id),						\
			    DT_INST_IRQ(id, priority),					\
			    gpio_smartbond_isr,						\
			    DEVICE_DT_INST_GET(id), 0);					\
		irq_enable(DT_INST_IRQN(id));						\
		return 0;								\
	}										\
											\
	DEVICE_DT_INST_DEFINE(id, gpio_smartbond_##id##_init,				\
			      NULL,							\
			      &gpio_smartbond_p##id##_data,				\
			      &gpio_smartbond_p##id##_config,				\
			      PRE_KERNEL_1,						\
			      CONFIG_GPIO_INIT_PRIORITY,				\
			      &gpio_smartbond_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SMARTBOND_DEVICE)
