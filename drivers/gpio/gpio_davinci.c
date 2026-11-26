/*
 * Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 * Copyright (C) 2025 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_davinci_gpio

#include <errno.h>

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/spinlock.h>

LOG_MODULE_REGISTER(gpio_davinci, CONFIG_GPIO_LOG_LEVEL);

/* Helper Macros for GPIO */
#define DEV_CFG(dev) \
		((const struct gpio_davinci_config *)((dev)->config))
#define DEV_DATA(dev) ((struct gpio_davinci_data *)(dev)->data)
#define DEV_REGS(dev) ((struct gpio_davinci_regs *)DEVICE_MMIO_NAMED_GET(dev, port_base))

#define GPIO_DAVINCI_DIR_RESET_VAL	(0xFFFFFFFF)

#define GPIO_DAVINCI_INTERRUPTS DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)

#define GPIO_DAVINCI_LINES_PER_BANK	16U
#define GPIO_DAVINCI_SET_OFFSET		0x10U
#define GPIO_DAVINCI_SET_STRIDE		0x28U
#define GPIO_DAVINCI_BINTEN_OFFSET	0x08U

struct gpio_davinci_regs {
	uint32_t dir;
	uint32_t out_data;
	uint32_t set_data;
	uint32_t clr_data;
	uint32_t in_data;
	uint32_t set_ris_trig;
	uint32_t clr_ris_trig;
	uint32_t set_fal_trig;
	uint32_t clr_fal_trig;
	uint32_t intstat;
};

struct gpio_davinci_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(port_base);
#if GPIO_DAVINCI_INTERRUPTS
	DEVICE_MMIO_NAMED_RAM(binten);
	struct k_spinlock lock;
	uint32_t ris_mask;
	uint32_t fal_mask;
	uint32_t enabled_mask;
#endif

	sys_slist_t cb;
	uint32_t saved_dir;
	uint32_t saved_out;
	bool saved;
};

struct gpio_davinci_config {
	struct gpio_driver_config common;
	void (*bank_config)(const struct device *dev);

	DEVICE_MMIO_NAMED_ROM(port_base);

	uint32_t port_num;
	const struct pinctrl_dev_config *pcfg;
#if GPIO_DAVINCI_INTERRUPTS
	DEVICE_MMIO_NAMED_ROM(binten);
	uint32_t bank_mask;
	void (*irq_config)(const struct device *dev);
#endif
};

static int gpio_davinci_configure(const struct device *dev, gpio_pin_t pin,
					gpio_flags_t flags)
{
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			regs->set_data = BIT(pin);
		} else {
			regs->clr_data = BIT(pin);
		}
		regs->dir &= ~(BIT(pin));
	} else {
		regs->dir |= BIT(pin);
	}

	return 0;
}

static int gpio_davinci_port_get_raw(const struct device *dev,
					gpio_port_value_t *value)
{
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	*value = regs->in_data;

	return 0;
}

static int gpio_davinci_port_set_masked_raw(const struct device *dev,
		gpio_port_pins_t mask, gpio_port_value_t value)
{
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	regs->out_data = (regs->out_data & (~mask)) | (mask & value);

	return 0;
}

static int gpio_davinci_port_set_bits_raw(const struct device *dev,
						gpio_port_pins_t mask)
{
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	regs->set_data = mask;

	return 0;
}

static int gpio_davinci_port_clear_bits_raw(const struct device *dev,
						gpio_port_pins_t mask)
{
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	regs->clr_data = mask;

	return 0;
}

static int gpio_davinci_port_toggle_bits(const struct device *dev,
						gpio_port_pins_t mask)
{
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	regs->out_data ^= mask;

	return 0;
}

#if GPIO_DAVINCI_INTERRUPTS
#define DEV_BINTEN(dev) ((mm_reg_t)DEVICE_MMIO_NAMED_GET(dev, binten))

static struct k_spinlock gpio_davinci_binten_lock;

static int gpio_davinci_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_davinci_config *config = DEV_CFG(dev);
	struct gpio_davinci_data *data = DEV_DATA(dev);
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);
	const uint32_t bit = BIT(pin);
	k_spinlock_key_t key;

	if (config->bank_mask == 0U) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (mode != GPIO_INT_MODE_DISABLED && mode != GPIO_INT_MODE_EDGE
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	    && mode != GPIO_INT_MODE_DISABLE_ONLY && mode != GPIO_INT_MODE_ENABLE_ONLY
#endif
	) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		regs->clr_ris_trig = bit;
		regs->clr_fal_trig = bit;
		regs->intstat = bit;
		data->enabled_mask &= ~bit;
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		regs->intstat = bit;
		data->enabled_mask |= bit & (data->ris_mask | data->fal_mask);
		if ((data->ris_mask & bit) != 0U) {
			regs->set_ris_trig = bit;
		}
		if ((data->fal_mask & bit) != 0U) {
			regs->set_fal_trig = bit;
		}
		k_spin_unlock(&data->lock, key);
		return 0;
	}
#endif

	regs->clr_ris_trig = bit;
	regs->clr_fal_trig = bit;
	regs->intstat = bit;
	data->ris_mask &= ~bit;
	data->fal_mask &= ~bit;
	data->enabled_mask &= ~bit;

	if (mode == GPIO_INT_MODE_EDGE) {
		if ((trig & GPIO_INT_TRIG_HIGH) != 0U) {
			data->ris_mask |= bit;
			regs->set_ris_trig = bit;
		}
		if ((trig & GPIO_INT_TRIG_LOW) != 0U) {
			data->fal_mask |= bit;
			regs->set_fal_trig = bit;
		}
		data->enabled_mask |= bit & (data->ris_mask | data->fal_mask);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_davinci_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct gpio_davinci_data *data = DEV_DATA(dev);

	return gpio_manage_callback(&data->cb, callback, set);
}

static uint32_t gpio_davinci_get_pending_int(const struct device *dev)
{
	struct gpio_davinci_data *data = DEV_DATA(dev);
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);

	return regs->intstat & data->enabled_mask;
}

static void gpio_davinci_isr(const struct device *dev)
{
	struct gpio_davinci_data *data = DEV_DATA(dev);
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);
	uint32_t status;

	status = regs->intstat & data->enabled_mask;

	regs->intstat = status;

	gpio_fire_callbacks(&data->cb, dev, status);
}

static void gpio_davinci_irq_hw_init(const struct device *dev)
{
	const struct gpio_davinci_config *config = DEV_CFG(dev);
	struct gpio_davinci_data *data = DEV_DATA(dev);
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);
	k_spinlock_key_t key;

	if (config->bank_mask == 0U) {
		return;
	}

	if (!data->saved) {
		regs->clr_ris_trig = UINT32_MAX;
		regs->clr_fal_trig = UINT32_MAX;
		regs->intstat = UINT32_MAX;
	}

	key = k_spin_lock(&data->lock);
	regs->set_ris_trig = data->enabled_mask & data->ris_mask;
	regs->set_fal_trig = data->enabled_mask & data->fal_mask;
	k_spin_unlock(&data->lock, key);

	key = k_spin_lock(&gpio_davinci_binten_lock);
	sys_write32(sys_read32(DEV_BINTEN(dev)) | config->bank_mask, DEV_BINTEN(dev));
	k_spin_unlock(&gpio_davinci_binten_lock, key);
}
#endif

static DEVICE_API(gpio, gpio_davinci_driver_api) = {
	.pin_configure = gpio_davinci_configure,
	.port_get_raw = gpio_davinci_port_get_raw,
	.port_set_masked_raw = gpio_davinci_port_set_masked_raw,
	.port_set_bits_raw = gpio_davinci_port_set_bits_raw,
	.port_clear_bits_raw = gpio_davinci_port_clear_bits_raw,
	.port_toggle_bits = gpio_davinci_port_toggle_bits,
#if GPIO_DAVINCI_INTERRUPTS
	.pin_interrupt_configure = gpio_davinci_pin_interrupt_configure,
	.manage_callback = gpio_davinci_manage_callback,
	.get_pending_int = gpio_davinci_get_pending_int,
#endif
};

static int gpio_davinci_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct gpio_davinci_config *config = DEV_CFG(dev);
	struct gpio_davinci_data *data = DEV_DATA(dev);
	volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (data->saved) {
			regs->out_data = data->saved_out;
			regs->dir = data->saved_dir;
		} else {
			config->bank_config(dev);
		}

#if GPIO_DAVINCI_INTERRUPTS
		gpio_davinci_irq_hw_init(dev);
#endif

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("failed to apply pinctrl");
			return ret;
		}
		return 0;
	case PM_DEVICE_ACTION_SUSPEND:
		data->saved_dir = regs->dir;
		data->saved_out = regs->out_data;
		data->saved = true;
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int gpio_davinci_init(const struct device *dev)
{
#if GPIO_DAVINCI_INTERRUPTS
	const struct gpio_davinci_config *config = DEV_CFG(dev);
#endif
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, port_base, K_MEM_CACHE_NONE);

#if GPIO_DAVINCI_INTERRUPTS
	if (config->bank_mask != 0U) {
		DEVICE_MMIO_NAMED_MAP(dev, binten, K_MEM_CACHE_NONE);
	}
#endif

	ret = pm_device_driver_init(dev, gpio_davinci_pm_action);
	if (ret < 0) {
		return ret;
	}

#if GPIO_DAVINCI_INTERRUPTS
	if (config->bank_mask != 0U) {
		config->irq_config(dev);
	}
#endif

	return 0;
}

#define GPIO_DAVINCI_INIT_FUNC(n)                                                                  \
	static void gpio_davinci_bank_##n##_config(const struct device *dev)                       \
	{                                                                                          \
		volatile struct gpio_davinci_regs *regs = DEV_REGS(dev);                           \
		regs->dir = GPIO_DAVINCI_DIR_RESET_VAL;                                            \
	}

#if GPIO_DAVINCI_INTERRUPTS
#define GPIO_DAVINCI_SET_POS(n)                                                                    \
	(DT_INST_REG_ADDR_BY_NAME(n, port) -                                                       \
	 (DT_INST_REG_ADDR_BY_NAME(n, binten) - GPIO_DAVINCI_BINTEN_OFFSET) -                      \
	 GPIO_DAVINCI_SET_OFFSET)

#define GPIO_DAVINCI_FIRST_BANK(n) (GPIO_DAVINCI_SET_POS(n) / GPIO_DAVINCI_SET_STRIDE * 2)

#define GPIO_DAVINCI_NUM_BANKS(n)                                                                  \
	DIV_ROUND_UP(DT_INST_PROP(n, ngpios), GPIO_DAVINCI_LINES_PER_BANK)

#define GPIO_DAVINCI_IRQ_CONNECT(i, n)                                                             \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, i), DT_INST_IRQ_BY_IDX(n, i, priority),                 \
		    gpio_davinci_isr, DEVICE_DT_INST_GET(n), DT_INST_IRQ_BY_IDX(n, i, flags));     \
	irq_enable(DT_INST_IRQN_BY_IDX(n, i));

#define GPIO_DAVINCI_IRQ_FUNC(n)                                                                   \
	BUILD_ASSERT(DT_INST_REG_HAS_NAME(n, binten),                                              \
		     "ti,davinci-gpio with interrupts needs a binten reg entry");                  \
	BUILD_ASSERT(DT_INST_NUM_IRQS(n) == GPIO_DAVINCI_NUM_BANKS(n),                             \
		     "ti,davinci-gpio needs one interrupt per 16-line bank");                      \
	BUILD_ASSERT(DT_INST_REG_ADDR_BY_IDX(n, 0) == DT_INST_REG_ADDR_BY_NAME(n, port),           \
		     "ti,davinci-gpio reg \"port\" must be the first entry");                     \
	BUILD_ASSERT(GPIO_DAVINCI_FIRST_BANK(n) + GPIO_DAVINCI_NUM_BANKS(n) <= 16,                 \
		     "ti,davinci-gpio banks exceed GPIO_BINTEN");                                  \
	BUILD_ASSERT((GPIO_DAVINCI_SET_POS(n) % GPIO_DAVINCI_SET_STRIDE) == 0,                     \
		     "ti,davinci-gpio port reg is not a register set of its binten module");       \
	static void gpio_davinci_irq_config_##n(const struct device *dev)                          \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		LISTIFY(DT_INST_NUM_IRQS(n), GPIO_DAVINCI_IRQ_CONNECT, (), n)                      \
	}

#define GPIO_DAVINCI_IRQ_CFG(n)                                                                    \
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(binten, DT_DRV_INST(n)),                                \
	.bank_mask = BIT_MASK(GPIO_DAVINCI_NUM_BANKS(n)) << GPIO_DAVINCI_FIRST_BANK(n),            \
	.irq_config = gpio_davinci_irq_config_##n,

#define GPIO_DAVINCI_IRQ_DEFINE(n)                                                                 \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (GPIO_DAVINCI_IRQ_FUNC(n)), ())
#define GPIO_DAVINCI_IRQ_INIT(n)                                                                   \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (GPIO_DAVINCI_IRQ_CFG(n)), ())
#else
#define GPIO_DAVINCI_IRQ_DEFINE(n)
#define GPIO_DAVINCI_IRQ_INIT(n)
#endif

#define GPIO_DAVINCI_DOMAIN_HAS_NO_DEVICE(n)						  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, power_domains),				  \
		    (DT_NODE_HAS_COMPAT(DT_INST_PHANDLE(n, power_domains),		  \
					arm_scmi_power)), (0))

#define GPIO_DAVINCI_PM_DEFINE(n)							  \
	COND_CODE_1(GPIO_DAVINCI_DOMAIN_HAS_NO_DEVICE(n),				  \
		    (PM_DEVICE_DEFINE(gpio_davinci_pm_##n, gpio_davinci_pm_action)),	  \
		    (PM_DEVICE_DT_INST_DEFINE(n, gpio_davinci_pm_action)))

#define GPIO_DAVINCI_PM_GET(n)								  \
	COND_CODE_1(GPIO_DAVINCI_DOMAIN_HAS_NO_DEVICE(n),				  \
		    (PM_DEVICE_GET(gpio_davinci_pm_##n)), (PM_DEVICE_DT_INST_GET(n)))

#define GPIO_DAVINCI_INIT(n)							  \
	PINCTRL_DT_INST_DEFINE(n);						  \
	GPIO_DAVINCI_INIT_FUNC(n);						  \
	GPIO_DAVINCI_IRQ_DEFINE(n)						  \
	static const struct gpio_davinci_config gpio_davinci_##n##_config = {	  \
		.bank_config = gpio_davinci_bank_##n##_config,			  \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),			  \
		DEVICE_MMIO_NAMED_ROM_INIT(port_base, DT_DRV_INST(n)),		  \
		.port_num = n,							  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			  \
		GPIO_DAVINCI_IRQ_INIT(n)					  \
	};									  \
										  \
	static struct gpio_davinci_data gpio_davinci_##n##_data;		  \
										  \
	BUILD_ASSERT(!DT_INST_PROP(n, zephyr_pm_device_runtime_auto) &&		  \
		     !IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME_DEFAULT_ENABLE),	  \
		     "ti,davinci-gpio has no device runtime PM");		  \
	BUILD_ASSERT(!COND_CODE_1(DT_INST_NODE_HAS_PROP(n, power_domains),	  \
		     (DT_PROP(DT_INST_PHANDLE(n, power_domains),		  \
			      zephyr_pm_device_runtime_auto)), (0)),		  \
		     "ti,davinci-gpio needs its power domain always on");	  \
	GPIO_DAVINCI_PM_DEFINE(n);						  \
										  \
	DEVICE_DT_INST_DEFINE(n,						  \
		gpio_davinci_init,						  \
		GPIO_DAVINCI_PM_GET(n),						  \
		&gpio_davinci_##n##_data,					  \
		&gpio_davinci_##n##_config,					  \
		PRE_KERNEL_2,							  \
		CONFIG_GPIO_INIT_PRIORITY,					  \
		&gpio_davinci_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_DAVINCI_INIT)
