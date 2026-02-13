/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_gpio

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include "r_ioport.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_RENESAS_RZ_TINT)
#include <zephyr/drivers/interrupt_controller/intc_rz_tint.h>
#endif
#if defined(CONFIG_RENESAS_RZ_EXT_IRQ)
#include "r_icu.h"
#include <zephyr/drivers/interrupt_controller/intc_rz_ext_irq.h>
#endif
#include "pinctrl_soc.h"
LOG_MODULE_REGISTER(rz_gpio, CONFIG_GPIO_LOG_LEVEL);

#define GPIO_RZ_MODE_HIZ    (0x0U)
#define GPIO_RZ_MODE_IN     (0x1U)
#define GPIO_RZ_MODE_OUT    (0x2U)
#define GPIO_RZ_MODE_OUT_IN (0x3U)

#define GPIO_RZ_INT_EDGE_FALLING (0x0U)
#define GPIO_RZ_INT_EDGE_RISING  (0x1U)
#define GPIO_RZ_INT_BOTH_EDGE    (0x2U)
#define GPIO_RZ_INT_LEVEL_LOW    (0x3U)
#define GPIO_RZ_INT_LEVEL_HIGH   (0x4U)

#define REG_P_READ(base, port)          sys_read8((base) + regs.p + (port))
#define REG_P_WRITE(base, port, v)      sys_write8((v), (base) + regs.p + (port))
#define REG_PM_READ(base, port)         sys_read16((base) + regs.pm + (port) * 2)
#define REG_PM_WRITE(base, port, v)     sys_write16((v), (base) + regs.pm + (port) * 2)
#define REG_PFC_READ(base, port)        sys_read32((base) + regs.pfc + (port) * 4)
#define REG_PFC_WRITE(base, port, v)    sys_write32((v), (base) + regs.pfc + (port) * 4)
#define REG_RSELP_READ(rselp, port)     sys_read8(rselp + (port))
#define REG_RSELP_WRITE(rselp, port, v) sys_write8((v), (rselp) + (port))

#define PORT(port_pin) FIELD_GET(BIT_MASK(8) << 8, port_pin)
#define PIN(port_pin)  FIELD_GET(BIT_MASK(8), port_pin)

struct gpio_rz_config {
	struct gpio_driver_config common;
	uint8_t ngpios;
	uint8_t port_num;
	bsp_io_port_t fsp_port;
	const ioport_cfg_t *fsp_cfg;
	const ioport_api_t *fsp_api;
	struct gpio_rz_irq_info *irq_info;
	uint8_t irq_info_size;
};

struct gpio_rz_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
	ioport_ctrl_t *fsp_ctrl;
	struct k_spinlock lock;
};

struct gpio_rz_pin {
	uint32_t mode;
	uint32_t func;
	uint32_t out_state;
};
struct gpio_rz_irq_info {
	const struct device *irq_dev;
	const struct device *gpio_dev;
	uint8_t pin;
};

struct gpio_rz_regs {
	mem_addr_t p;
	mem_addr_t pm;
	mem_addr_t pfc;
#ifdef CONFIG_GPIO_RENESAS_RZ_TYPE1
	mem_addr_t base;
#endif
#ifdef CONFIG_GPIO_RENESAS_RZ_TYPE2
	mem_addr_t rselp;
	struct {
		mem_addr_t ns;
		mem_addr_t s;
	} base;
#endif
} regs = {
	.p = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), p),
	.pm = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), pm),
	.pfc = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), pfc),
#ifdef CONFIG_GPIO_RENESAS_RZ_TYPE1
	.base = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), base),
#endif
#ifdef CONFIG_GPIO_RENESAS_RZ_TYPE2
	.rselp = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), rselp),
	.base = {
		.ns = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), base_ns),
		.s = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio), base_s),
	},
#endif
};

/* Helper function to get current pin state from P register */
static inline uint32_t gpio_rz_get_out_state(mem_addr_t base, uint8_t port, gpio_pin_t pin)
{
	return FIELD_GET(BIT_MASK(1) << pin, REG_P_READ(base, port));
}

/* Helper function to get pin mode from PM register */
static inline uint32_t gpio_rz_get_mode(mem_addr_t base, uint8_t port, gpio_pin_t pin)
{
	return FIELD_GET(BIT_MASK(2) << (pin * 2), REG_PM_READ(base, port));
}

/* Helper function to get pin function from PFC register */
static inline uint32_t gpio_rz_get_func(mem_addr_t base, uint8_t port, gpio_pin_t pin)
{
	return FIELD_GET(BIT_MASK(4) << (pin * 4), REG_PFC_READ(base, port));
}

static __maybe_unused int gpio_rz_pincfg_to_flags(const struct gpio_rz_pin *pincfg,
						  gpio_flags_t *out_flags)
{
	gpio_flags_t flags = 0;

	if (pincfg->mode == GPIO_RZ_MODE_OUT || pincfg->mode == GPIO_RZ_MODE_OUT_IN) {
		if (pincfg->out_state != 0) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	}

	if (pincfg->mode == GPIO_RZ_MODE_IN || pincfg->mode == GPIO_RZ_MODE_OUT_IN) {
		flags |= GPIO_INPUT;
	}

	*out_flags = flags;

	return 0;
}

/* Get pin's configuration, used by pin_configure/pin_interrupt_configure api */
static int gpio_rz_pin_config_get_raw(bsp_io_port_pin_t port_pin, struct gpio_rz_pin *pincfg)
{
	uint8_t port = PORT(port_pin);
	gpio_pin_t pin = PIN(port_pin);

#ifdef CONFIG_GPIO_RENESAS_RZ_TYPE1
	pincfg->mode = gpio_rz_get_mode(regs.base, port, pin);
	pincfg->func = gpio_rz_get_func(regs.base, port, pin);
	pincfg->out_state = gpio_rz_get_out_state(regs.base, port, pin);
#else /* CONFIG_GPIO_RENESAS_RZ_TYPE2 */
	pincfg->mode = gpio_rz_get_mode(regs.base.ns, port, pin);
	pincfg->mode |= gpio_rz_get_mode(regs.base.s, port, pin);

	pincfg->func = gpio_rz_get_func(regs.base.ns, port, pin);
	pincfg->func |= gpio_rz_get_func(regs.base.s, port, pin);

	pincfg->out_state = gpio_rz_get_out_state(regs.base.ns, port, pin);
	pincfg->out_state |= gpio_rz_get_out_state(regs.base.s, port, pin);
#endif

	return 0;
}

static int gpio_rz_flags_to_cfg(const gpio_flags_t flags, const struct gpio_rz_pin *curr_pincfg,
				uint32_t *out_cfg)
{
	if (flags & GPIO_OPEN_DRAIN) {
		return -ENOTSUP;
	}

	pinctrl_cfg_data_t pincfg;

	pincfg.cfg = 0;
	if (flags & GPIO_INPUT) {
		if (flags & GPIO_OUTPUT) {
			pincfg.cfg |= IOPORT_CFG_PORT_DIRECTION_OUTPUT_INPUT;
		} else {
			pincfg.cfg |= IOPORT_CFG_PORT_DIRECTION_INPUT;
		}
	} else if (flags & GPIO_OUTPUT) {
		pincfg.cfg |= IOPORT_CFG_PORT_DIRECTION_OUTPUT;
	} else {
		pincfg.cfg |= IOPORT_CFG_PORT_DIRECTION_HIZ;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		pincfg.cfg |= IOPORT_CFG_PORT_OUTPUT_HIGH;
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		pincfg.cfg &= ~IOPORT_CFG_PORT_OUTPUT_HIGH;
	} else {
		/* Keep the current state */
		pincfg.cfg_b.p_reg = curr_pincfg->out_state;
	}

	if (flags & GPIO_PULL_UP) {
		pincfg.cfg |= IOPORT_CFG_PULLUP_ENABLE;
	} else if (flags & GPIO_PULL_DOWN) {
		pincfg.cfg |= IOPORT_CFG_PULLDOWN_ENABLE;
	}

	if (flags & GPIO_INT_ENABLE) {
		IF_ENABLED(CONFIG_GPIO_RENESAS_RZ_TYPE1, (
			pincfg.cfg_b.isel_reg = 1;
		))

		IF_ENABLED(CONFIG_GPIO_RENESAS_RZ_TYPE2, (
			pincfg.cfg_b.pmc_reg = 1;
		))
	} else if (flags & GPIO_INT_DISABLE) {
		IF_ENABLED(CONFIG_GPIO_RENESAS_RZ_TYPE1, (
			pincfg.cfg_b.isel_reg = 0;
		))

		IF_ENABLED(CONFIG_GPIO_RENESAS_RZ_TYPE2, (
			pincfg.cfg_b.pmc_reg = 0;
		))
	}

	/* Type 1 (RZ/A,G,V): IOLH, FILONOFF, FILNUM, FILCLKSEL
	 * Type 2 (RZ/T,N): DRCTL, RSELP
	 */
	IF_ENABLED(CONFIG_GPIO_RENESAS_RZ_TYPE1, (
		pincfg.cfg_b.iolh_reg = FIELD_GET(GENMASK(9, 8), flags);
		pincfg.cfg_b.filonoff_reg = FIELD_GET(BIT(10), flags);
		pincfg.cfg_b.filnum_reg = FIELD_GET(GENMASK(12, 11), flags);
		pincfg.cfg_b.filclksel_reg = FIELD_GET(GENMASK(14, 13), flags);
	))

	IF_ENABLED(CONFIG_GPIO_RENESAS_RZ_TYPE2, (
		/* Must use OR for DRCTL since it is already updated for pull-up/down above */
		pincfg.cfg_b.drct_reg |= FIELD_GET(GENMASK(9, 8), flags);
		pincfg.cfg_b.rsel_reg = FIELD_GET(BIT(14), flags);
	))

	/* PFC */
	pincfg.cfg_b.pfc_reg = curr_pincfg->func;

	*out_cfg = pincfg.cfg;

	return 0;
}

static __maybe_unused int gpio_rz_pin_get_config(const struct device *dev, gpio_pin_t pin,
						 gpio_flags_t *flags)
{
	const struct gpio_rz_config *config = dev->config;
	bsp_io_port_pin_t port_pin = config->fsp_port | pin;
	struct gpio_rz_pin pincfg;

	/* Get the current pin config */
	gpio_rz_pin_config_get_raw(port_pin, &pincfg);

	/* Convert the pin config to flags */
	gpio_rz_pincfg_to_flags(&pincfg, flags);

	return 0;
}

static int gpio_rz_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	bsp_io_port_pin_t port_pin = config->fsp_port | pin;
	struct gpio_rz_pin curr_pincfg;
	uint32_t cfg = 0;
	fsp_err_t err;
	int ret;

	/* Get the current pin config */
	gpio_rz_pin_config_get_raw(port_pin, &curr_pincfg);

	/* Convert flags and current pin config to pin config */
	ret = gpio_rz_flags_to_cfg(flags, &curr_pincfg, &cfg);
	if (ret < 0) {
		return ret;
	}

	err = config->fsp_api->pinCfg(data->fsp_ctrl, port_pin, cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rz_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	fsp_err_t err;
	ioport_size_t port_value;

	err = config->fsp_api->portRead(data->fsp_ctrl, config->fsp_port, &port_value);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*value = (gpio_port_value_t)port_value;

	return 0;
}

static int gpio_rz_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	ioport_size_t port_mask = (ioport_size_t)mask;
	ioport_size_t port_value = (ioport_size_t)value;
	fsp_err_t err;

	err = config->fsp_api->portWrite(data->fsp_ctrl, config->fsp_port, port_value, port_mask);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rz_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	ioport_size_t mask = (ioport_size_t)pins;
	ioport_size_t value = (ioport_size_t)pins;
	fsp_err_t err;

	err = config->fsp_api->portWrite(data->fsp_ctrl, config->fsp_port, value, mask);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rz_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	ioport_size_t mask = (ioport_size_t)pins;
	ioport_size_t value = 0x00;
	fsp_err_t err;

	err = config->fsp_api->portWrite(data->fsp_ctrl, config->fsp_port, value, mask);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rz_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rz_config *config = dev->config;
	uint8_t val;
	uint8_t port = PORT(config->fsp_port);

#ifdef CONFIG_GPIO_RENESAS_RZ_TYPE1
	val = REG_P_READ(regs.base, port);
	REG_P_WRITE(regs.base, port, val ^ pins);
#else /* CONFIG_GPIO_RENESAS_RZ_TYPE2 */
	uint8_t rselp = REG_RSELP_READ(regs.rselp, port);

	uint8_t pins_ns = rselp & pins;
	uint8_t pins_s = ~rselp & pins;

	val = REG_P_READ(regs.base.ns, port);
	REG_P_WRITE(regs.base.ns, port, val ^ pins_ns);

	val = REG_P_READ(regs.base.s, port);
	REG_P_WRITE(regs.base.s, port, val ^ pins_s);
#endif /* CONFIG_GPIO_RENESAS_RZ_TYPE1 */

	return 0;
}

#if defined(CONFIG_RENESAS_RZ_TINT) || defined(CONFIG_RENESAS_RZ_EXT_IRQ)
static int gpio_rz_int_disable(struct gpio_rz_irq_info *irq_info)
{
#if defined(CONFIG_RENESAS_RZ_TINT)
	if (strstr(irq_info->irq_dev->name, "tint") != NULL) {
		return intc_rz_tint_disable(irq_info->irq_dev);
	}
#endif /* CONFIG_RENESAS_RZ_TINT */

#if defined(CONFIG_RENESAS_RZ_EXT_IRQ)
	if (strstr(irq_info->irq_dev->name, "irq") != NULL) {
		return intc_rz_ext_irq_disable(irq_info->irq_dev);
	}
#endif /* CONFIG_RENESAS_RZ_EXT_IRQ */

	return 0;
}

static void gpio_rz_callback(void *arg);

static int gpio_rz_int_enable(struct gpio_rz_irq_info *irq_info, uint8_t irq_type)
{
	int ret = 0;
	const struct device *irq_dev = irq_info->irq_dev;

#if defined(CONFIG_RENESAS_RZ_TINT)
	if (strstr(irq_dev->name, "tint") != NULL) {
		const struct gpio_rz_config *gpio_config = irq_info->gpio_dev->config;

		ret = intc_rz_tint_connect(irq_dev, gpio_config->port_num, irq_info->pin);
		if (ret < 0) {
			return ret;
		}

		ret = intc_rz_tint_set_type(irq_dev, irq_type);
		if (ret < 0) {
			return ret;
		}

		intc_rz_tint_enable(irq_dev);
		intc_rz_tint_set_callback(irq_dev, gpio_rz_callback, (void *)irq_info);
	}
#endif /* CONFIG_RENESAS_RZ_TINT */

#if defined(CONFIG_RENESAS_RZ_EXT_IRQ)
	if (strstr(irq_dev->name, "irq") != NULL) {
		ret = intc_rz_ext_irq_set_type(irq_dev, irq_type);
		if (ret < 0) {
			return ret;
		}

		intc_rz_ext_irq_enable(irq_dev);
		intc_rz_ext_irq_set_callback(irq_dev, gpio_rz_callback, (void *)irq_info);
	}
#endif /* CONFIG_RENESAS_RZ_EXT_IRQ */

	return ret;
}

static void gpio_rz_callback(void *arg)
{
	const struct gpio_rz_irq_info *irq_info = (const struct gpio_rz_irq_info *)arg;
	const struct device *gpio_dev = irq_info->gpio_dev;
	struct gpio_rz_data *gpio_data = gpio_dev->data;

	gpio_fire_callbacks(&gpio_data->cb, gpio_dev, BIT(irq_info->pin));
}

static struct gpio_rz_irq_info *gpio_rz_get_irq_info(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_irq_info *irq_info = config->irq_info;

	for (int i = 0; i < config->irq_info_size; i++) {
		if (irq_info[i].pin == pin) {
			return &irq_info[i];
		}
	}

	return NULL;
}

static int gpio_rz_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	uint8_t irq_type = 0;
	gpio_flags_t flags;
	k_spinlock_key_t key;
	int ret = 0;

	if (pin >= config->ngpios) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	gpio_rz_pin_get_config(dev, pin, &flags);
	ret = gpio_rz_pin_configure(dev, pin, flags | mode);
	if (ret < 0) {
		goto exit_unlock;
	}

	struct gpio_rz_irq_info *irq_info = gpio_rz_get_irq_info(dev, pin);

	if (!irq_info || !device_is_ready(irq_info->irq_dev)) {
		/* No interrupt to configure */
		goto exit_unlock;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		ret = gpio_rz_int_disable(irq_info);
		goto exit_unlock;
	}

	if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_LOW) {
			irq_type = GPIO_RZ_INT_EDGE_FALLING;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irq_type = GPIO_RZ_INT_EDGE_RISING;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			irq_type = GPIO_RZ_INT_BOTH_EDGE;
		}
	} else {
		if (trig == GPIO_INT_TRIG_LOW) {
			irq_type = GPIO_RZ_INT_LEVEL_LOW;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irq_type = GPIO_RZ_INT_LEVEL_HIGH;
		}
	}

	ret = gpio_rz_int_enable(irq_info, irq_type);

exit_unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int gpio_rz_manage_callback(const struct device *dev, struct gpio_callback *callback,
				   bool set)
{
	struct gpio_rz_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

#endif /* CONFIG_RENESAS_RZ_TINT || CONFIG_RENESAS_RZ_EXT_IRQ */

static DEVICE_API(gpio, gpio_rz_driver_api) = {
	.pin_configure = gpio_rz_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_rz_pin_get_config,
#endif
	.port_get_raw = gpio_rz_port_get_raw,
	.port_set_masked_raw = gpio_rz_port_set_masked_raw,
	.port_set_bits_raw = gpio_rz_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rz_port_clear_bits_raw,
	.port_toggle_bits = gpio_rz_port_toggle_bits,
#if defined(CONFIG_RENESAS_RZ_TINT) || defined(CONFIG_RENESAS_RZ_EXT_IRQ)
	.pin_interrupt_configure = gpio_rz_pin_interrupt_configure,
	.manage_callback = gpio_rz_manage_callback,
#endif
};

#define IRQ_INFO_GET_BY_IDX(node_id, prop, idx, inst)                                              \
	{                                                                                          \
		.irq_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_IDX(node_id, irqs, idx)),           \
		.gpio_dev = DEVICE_DT_INST_GET(inst),                                              \
		.pin = DT_PHA_BY_IDX(node_id, irqs, idx, pin),                                     \
	}

#define IRQ_INFO_GET(inst)                                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irqs),                                             \
		    (DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(inst, irqs,                               \
							 IRQ_INFO_GET_BY_IDX, (,), inst)), ())

#define RZ_GPIO_PORT_INIT(inst)                                                                    \
	static struct gpio_rz_irq_info gpio_rz_irq_info_##inst[] = {IRQ_INFO_GET(inst)};           \
	static ioport_cfg_t g_ioport_##inst##_cfg = {                                              \
		.number_of_pins = 0,                                                               \
		.p_pin_cfg_data = NULL,                                                            \
		.p_extend = NULL,                                                                  \
	};                                                                                         \
	static const struct gpio_rz_config gpio_rz_##inst##_config = {                             \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.fsp_port = (uint32_t)DT_INST_REG_ADDR(inst),                                      \
		.port_num = (uint8_t)DT_NODE_CHILD_IDX(DT_DRV_INST(inst)),                         \
		.ngpios = (uint8_t)DT_INST_PROP(inst, ngpios),                                     \
		.fsp_cfg = &g_ioport_##inst##_cfg,                                                 \
		.fsp_api = &g_ioport_on_ioport,                                                    \
		.irq_info = gpio_rz_irq_info_##inst,                                               \
		.irq_info_size = ARRAY_SIZE(gpio_rz_irq_info_##inst),                              \
	};                                                                                         \
	static ioport_instance_ctrl_t g_ioport_##inst##_ctrl;                                      \
	static struct gpio_rz_data gpio_rz_##inst##_data = {                                       \
		.fsp_ctrl = &g_ioport_##inst##_ctrl,                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &gpio_rz_##inst##_data, &gpio_rz_##inst##_config,  \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_rz_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RZ_GPIO_PORT_INIT)
