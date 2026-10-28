/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_ioss_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define IFX_IOSS_GPIO_OUT		0x00U
#define IFX_IOSS_GPIO_OUT_CLR		0x04U
#define IFX_IOSS_GPIO_OUT_SET		0x08U
#define IFX_IOSS_GPIO_OUT_INV		0x0CU
#define IFX_IOSS_GPIO_IN		0x10U
#define IFX_IOSS_GPIO_INTR		0x14U
#define IFX_IOSS_GPIO_INTR_MASK		0x18U
#define IFX_IOSS_GPIO_INTR_MASKED	0x1CU
#define IFX_IOSS_GPIO_INTR_CFG		0x40U
#define IFX_IOSS_GPIO_CFG		0x44U

#define IFX_IOSS_GPIO_CFG_SHIFT(pin)	((pin) * 4U)
#define IFX_IOSS_GPIO_CFG_MSK		GENMASK(3, 0)
#define IFX_IOSS_GPIO_CFG_DM_MSK	GENMASK(2, 0)
#define IFX_IOSS_GPIO_CFG_IN_EN		BIT(3)

#define IFX_IOSS_GPIO_DM_HIGHZ		0U
#define IFX_IOSS_GPIO_DM_PULLUP		2U
#define IFX_IOSS_GPIO_DM_PULLDOWN	3U
#define IFX_IOSS_GPIO_DM_OD_DRIVESLOW	4U
#define IFX_IOSS_GPIO_DM_OD_DRIVESHIGH	5U
#define IFX_IOSS_GPIO_DM_STRONG		6U
#define IFX_IOSS_GPIO_DM_PULLUP_DOWN	7U

#define IFX_IOSS_GPIO_EDGE_SHIFT(pin)	((pin) * 2U)
#define IFX_IOSS_GPIO_EDGE_MSK		GENMASK(1, 0)
#define IFX_IOSS_GPIO_EDGE_DISABLE	0U
#define IFX_IOSS_GPIO_EDGE_RISING	1U
#define IFX_IOSS_GPIO_EDGE_FALLING	2U
#define IFX_IOSS_GPIO_EDGE_BOTH		3U

#define IFX_IOSS_HSIOM_PORT_SEL(pin)	(((pin) / 4U) * 4U)
#define IFX_IOSS_HSIOM_SEL_SHIFT(pin)	(((pin) % 4U) * 8U)
#define IFX_IOSS_HSIOM_SEL_MSK		GENMASK(4, 0)
#define IFX_IOSS_HSIOM_SEL_GPIO		0U

struct ifx_ioss_gpio_config {
	struct gpio_driver_config common;
	mem_addr_t gpio;
	mem_addr_t hsiom;
	void (*irq_config)(void);
};

struct ifx_ioss_gpio_data {
	struct gpio_driver_data common;
	struct k_spinlock lock;
	sys_slist_t callbacks;
};

static void ifx_ioss_gpio_rmw(mem_addr_t addr, uint32_t clr, uint32_t set)
{
	uint32_t val = sys_read32(addr);

	val &= ~clr;
	val |= set;
	sys_write32(val, addr);
}

static void ifx_ioss_gpio_clear_intr(const struct ifx_ioss_gpio_config *cfg,
				     uint32_t pins)
{
	(void)sys_read32(cfg->gpio + IFX_IOSS_GPIO_INTR);
	sys_write32(pins, cfg->gpio + IFX_IOSS_GPIO_INTR);
	(void)sys_read32(cfg->gpio + IFX_IOSS_GPIO_INTR);
}

static int ifx_ioss_gpio_drive_mode(gpio_flags_t flags, uint32_t *dm)
{
	gpio_flags_t pull = flags & (GPIO_PULL_UP | GPIO_PULL_DOWN);

	if ((flags & GPIO_OUTPUT) == 0U) {
		if (pull == (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			*dm = IFX_IOSS_GPIO_DM_PULLUP_DOWN;
		} else if (pull == GPIO_PULL_UP) {
			*dm = IFX_IOSS_GPIO_DM_PULLUP;
		} else if (pull == GPIO_PULL_DOWN) {
			*dm = IFX_IOSS_GPIO_DM_PULLDOWN;
		} else {
			*dm = IFX_IOSS_GPIO_DM_HIGHZ;
		}

		return 0;
	}

	if ((flags & GPIO_SINGLE_ENDED) == 0U) {
		if (pull != 0U) {
			return -ENOTSUP;
		}

		*dm = IFX_IOSS_GPIO_DM_STRONG;

		return 0;
	}

	if ((flags & GPIO_LINE_OPEN_DRAIN) != 0U) {
		if ((pull & GPIO_PULL_DOWN) != 0U) {
			return -ENOTSUP;
		}

		*dm = (pull == GPIO_PULL_UP) ? IFX_IOSS_GPIO_DM_PULLUP
					     : IFX_IOSS_GPIO_DM_OD_DRIVESLOW;
	} else {
		if ((pull & GPIO_PULL_UP) != 0U) {
			return -ENOTSUP;
		}

		*dm = (pull == GPIO_PULL_DOWN) ? IFX_IOSS_GPIO_DM_PULLDOWN
					       : IFX_IOSS_GPIO_DM_OD_DRIVESHIGH;
	}

	return 0;
}

static int ifx_ioss_gpio_pin_configure(const struct device *dev,
				       gpio_pin_t pin, gpio_flags_t flags)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;
	struct ifx_ioss_gpio_data *data = dev->data;
	uint32_t cfg_shift = IFX_IOSS_GPIO_CFG_SHIFT(pin);
	uint32_t sel_shift = IFX_IOSS_HSIOM_SEL_SHIFT(pin);
	mem_addr_t sel = cfg->hsiom + IFX_IOSS_HSIOM_PORT_SEL(pin);
	uint32_t dm = IFX_IOSS_GPIO_DM_HIGHZ;
	k_spinlock_key_t key;
	uint32_t out;
	int ret;

	if ((cfg->common.port_pin_mask & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) != 0U) {
		ret = ifx_ioss_gpio_drive_mode(flags, &dm);
		if (ret != 0) {
			return ret;
		}
	}

	if ((flags & GPIO_INPUT) != 0U) {
		dm |= IFX_IOSS_GPIO_CFG_IN_EN;
	}

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		out = IFX_IOSS_GPIO_OUT_SET;
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		out = IFX_IOSS_GPIO_OUT_CLR;
	} else if ((flags & GPIO_OUTPUT) != 0U) {
		out = 0U;
	} else if ((flags & GPIO_PULL_UP) != 0U) {
		out = IFX_IOSS_GPIO_OUT_SET;
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		out = IFX_IOSS_GPIO_OUT_CLR;
	} else {
		out = 0U;
	}

	key = k_spin_lock(&data->lock);

	if (out != 0U) {
		sys_write32(BIT(pin), cfg->gpio + out);
	}

	ifx_ioss_gpio_rmw(cfg->gpio + IFX_IOSS_GPIO_CFG,
			  IFX_IOSS_GPIO_CFG_MSK << cfg_shift, dm << cfg_shift);
	ifx_ioss_gpio_rmw(sel, IFX_IOSS_HSIOM_SEL_MSK << sel_shift,
			  IFX_IOSS_HSIOM_SEL_GPIO << sel_shift);

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int ifx_ioss_gpio_pin_get_config(const struct device *dev,
					gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;
	gpio_flags_t level = GPIO_OUTPUT_LOW;
	gpio_flags_t flags;
	uint32_t sel;
	uint32_t val;

	if ((cfg->common.port_pin_mask & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	sel = sys_read32(cfg->hsiom + IFX_IOSS_HSIOM_PORT_SEL(pin));
	sel = (sel >> IFX_IOSS_HSIOM_SEL_SHIFT(pin)) & IFX_IOSS_HSIOM_SEL_MSK;
	if (sel != IFX_IOSS_HSIOM_SEL_GPIO) {
		*out_flags = 0U;
		return 0;
	}

	if ((sys_read32(cfg->gpio + IFX_IOSS_GPIO_OUT) & BIT(pin)) != 0U) {
		level = GPIO_OUTPUT_HIGH;
	}

	val = sys_read32(cfg->gpio + IFX_IOSS_GPIO_CFG);
	val = (val >> IFX_IOSS_GPIO_CFG_SHIFT(pin)) & IFX_IOSS_GPIO_CFG_MSK;

	switch (val & IFX_IOSS_GPIO_CFG_DM_MSK) {
	case IFX_IOSS_GPIO_DM_HIGHZ:
		flags = GPIO_DISCONNECTED;
		break;
	case IFX_IOSS_GPIO_DM_PULLUP:
		flags = GPIO_PULL_UP;
		break;
	case IFX_IOSS_GPIO_DM_PULLDOWN:
		flags = GPIO_PULL_DOWN;
		break;
	case IFX_IOSS_GPIO_DM_PULLUP_DOWN:
		flags = GPIO_PULL_UP | GPIO_PULL_DOWN;
		break;
	case IFX_IOSS_GPIO_DM_OD_DRIVESLOW:
		flags = GPIO_OPEN_DRAIN | level;
		break;
	case IFX_IOSS_GPIO_DM_OD_DRIVESHIGH:
		flags = GPIO_OPEN_SOURCE | level;
		break;
	case IFX_IOSS_GPIO_DM_STRONG:
		flags = level;
		break;
	default:
		return -ENOTSUP;
	}

	if ((val & IFX_IOSS_GPIO_CFG_IN_EN) != 0U) {
		flags |= GPIO_INPUT;
	}

	*out_flags = flags;

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

static int ifx_ioss_gpio_port_get_raw(const struct device *dev,
				      gpio_port_value_t *value)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;

	*value = sys_read32(cfg->gpio + IFX_IOSS_GPIO_IN) &
		 cfg->common.port_pin_mask;

	return 0;
}

static int ifx_ioss_gpio_port_set_masked_raw(const struct device *dev,
					     gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;
	struct ifx_ioss_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	mask &= cfg->common.port_pin_mask;

	key = k_spin_lock(&data->lock);
	ifx_ioss_gpio_rmw(cfg->gpio + IFX_IOSS_GPIO_OUT, mask, value & mask);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int ifx_ioss_gpio_port_set_bits_raw(const struct device *dev,
					   gpio_port_pins_t pins)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;

	sys_write32(pins & cfg->common.port_pin_mask,
		    cfg->gpio + IFX_IOSS_GPIO_OUT_SET);

	return 0;
}

static int ifx_ioss_gpio_port_clear_bits_raw(const struct device *dev,
					     gpio_port_pins_t pins)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;

	sys_write32(pins & cfg->common.port_pin_mask,
		    cfg->gpio + IFX_IOSS_GPIO_OUT_CLR);

	return 0;
}

static int ifx_ioss_gpio_port_toggle_bits(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;

	sys_write32(pins & cfg->common.port_pin_mask,
		    cfg->gpio + IFX_IOSS_GPIO_OUT_INV);

	return 0;
}

static int ifx_ioss_gpio_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;
	struct ifx_ioss_gpio_data *data = dev->data;
	uint32_t edge_shift = IFX_IOSS_GPIO_EDGE_SHIFT(pin);
	uint32_t edge;
	k_spinlock_key_t key;

	if ((cfg->common.port_pin_mask & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		edge = IFX_IOSS_GPIO_EDGE_DISABLE;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			edge = IFX_IOSS_GPIO_EDGE_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			edge = IFX_IOSS_GPIO_EDGE_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			edge = IFX_IOSS_GPIO_EDGE_BOTH;
			break;
		default:
			return -ENOTSUP;
		}
	} else {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	ifx_ioss_gpio_rmw(cfg->gpio + IFX_IOSS_GPIO_INTR_MASK, BIT(pin), 0U);
	ifx_ioss_gpio_rmw(cfg->gpio + IFX_IOSS_GPIO_INTR_CFG,
			  IFX_IOSS_GPIO_EDGE_MSK << edge_shift,
			  edge << edge_shift);
	ifx_ioss_gpio_clear_intr(cfg, BIT(pin));

	if (edge != IFX_IOSS_GPIO_EDGE_DISABLE) {
		ifx_ioss_gpio_rmw(cfg->gpio + IFX_IOSS_GPIO_INTR_MASK, 0U,
				  BIT(pin));
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int ifx_ioss_gpio_manage_callback(const struct device *dev,
					 struct gpio_callback *callback,
					 bool set)
{
	struct ifx_ioss_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t ifx_ioss_gpio_get_pending_int(const struct device *dev)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;

	return sys_read32(cfg->gpio + IFX_IOSS_GPIO_INTR_MASKED) &
	       cfg->common.port_pin_mask;
}

static void ifx_ioss_gpio_isr(const struct device *dev)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;
	struct ifx_ioss_gpio_data *data = dev->data;
	uint32_t pending = ifx_ioss_gpio_get_pending_int(dev);

	if (pending == 0U) {
		return;
	}

	ifx_ioss_gpio_clear_intr(cfg, pending);
	gpio_fire_callbacks(&data->callbacks, dev, pending);
}

static int ifx_ioss_gpio_init(const struct device *dev)
{
	const struct ifx_ioss_gpio_config *cfg = dev->config;

	if (cfg->irq_config != NULL) {
		cfg->irq_config();
	}

	return 0;
}

static DEVICE_API(gpio, ifx_ioss_gpio_api) = {
	.pin_configure = ifx_ioss_gpio_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = ifx_ioss_gpio_pin_get_config,
#endif /* CONFIG_GPIO_GET_CONFIG */
	.port_get_raw = ifx_ioss_gpio_port_get_raw,
	.port_set_masked_raw = ifx_ioss_gpio_port_set_masked_raw,
	.port_set_bits_raw = ifx_ioss_gpio_port_set_bits_raw,
	.port_clear_bits_raw = ifx_ioss_gpio_port_clear_bits_raw,
	.port_toggle_bits = ifx_ioss_gpio_port_toggle_bits,
	.pin_interrupt_configure = ifx_ioss_gpio_pin_interrupt_configure,
	.manage_callback = ifx_ioss_gpio_manage_callback,
	.get_pending_int = ifx_ioss_gpio_get_pending_int,
};

#define IFX_IOSS_GPIO_IRQ_CONFIG(n)                                            \
	static void ifx_ioss_gpio_irq_config_##n(void)                         \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),         \
			    ifx_ioss_gpio_isr, DEVICE_DT_INST_GET(n), 0);      \
		irq_enable(DT_INST_IRQN(n));                                   \
	}

#define IFX_IOSS_GPIO_INIT(n)                                                  \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (IFX_IOSS_GPIO_IRQ_CONFIG(n)))   \
                                                                               \
	static const struct ifx_ioss_gpio_config ifx_ioss_gpio_config_##n = {  \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                  \
		.gpio = DT_INST_REG_ADDR_BY_NAME(n, gpio),                     \
		.hsiom = DT_INST_REG_ADDR_BY_NAME(n, hsiom),                   \
		.irq_config = COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0),           \
					  (ifx_ioss_gpio_irq_config_##n),      \
					  (NULL)),                             \
	};                                                                     \
                                                                               \
	static struct ifx_ioss_gpio_data ifx_ioss_gpio_data_##n;               \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, ifx_ioss_gpio_init, NULL,                     \
			      &ifx_ioss_gpio_data_##n,                         \
			      &ifx_ioss_gpio_config_##n, PRE_KERNEL_1,         \
			      CONFIG_GPIO_INIT_PRIORITY, &ifx_ioss_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_IOSS_GPIO_INIT)
