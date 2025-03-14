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
#include "gpio_renesas_rz.h"
#include <zephyr/logging/log.h>
#if defined(CONFIG_SOC_SERIES_RZN2L)
#include "r_icu.h"
#include <zephyr/drivers/interrupt_controller/intc_rz_ext_irq.h>
#endif
LOG_MODULE_REGISTER(rz_gpio, CONFIG_GPIO_LOG_LEVEL);

#define LOG_DEV_ERR(dev, format, ...) LOG_ERR("%s:" #format, (dev)->name, ##__VA_ARGS__)
#define LOG_DEV_DBG(dev, format, ...) LOG_DBG("%s:" #format, (dev)->name, ##__VA_ARGS__)

struct gpio_rz_config {
	struct gpio_driver_config common;
	uint8_t ngpios;
	uint8_t port_num;
	bsp_io_port_t fsp_port;
	const ioport_cfg_t *fsp_cfg;
	const ioport_api_t *fsp_api;
	const struct device *int_dev;
	uint8_t int_num[GPIO_RZ_MAX_INT_NUM];
#if defined(CONFIG_SOC_SERIES_RZN2L)
	const struct device *eirq_dev[GPIO_RZ_MAX_INT_NUM];

	void (*cb_list[GPIO_RZ_MAX_INT_NUM])(void *arg);
#endif
};

struct gpio_rz_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
	ioport_instance_ctrl_t *fsp_ctrl;
	struct k_spinlock lock;
#if defined(CONFIG_SOC_SERIES_RZN2L)
	uint8_t pin[GPIO_RZ_MAX_INT_NUM];
#endif
};

struct gpio_rz_isr_data {
	const struct device *gpio_dev;
	gpio_pin_t pin;
};

struct gpio_rz_int_data {
	struct gpio_rz_isr_data gpio_mapping[GPIO_RZ_MAX_INT_NUM];
	uint32_t irq_set_edge;
};

struct gpio_rz_tint_config {
	void (*gpio_int_init)(void);
};

static int gpio_rz_pin_config_get_raw(bsp_io_port_pin_t port_pin, uint32_t *flags);

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_rz_pin_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_rz_config *config = dev->config;
	bsp_io_port_pin_t port_pin = config->fsp_port | pin;

	gpio_rz_pin_config_get_raw(port_pin, flags);
	return 0;
}
#endif

/* Get previous pin's configuration, used by pin_configure/pin_interrupt_configure api */
static int gpio_rz_pin_config_get_raw(bsp_io_port_pin_t port_pin, uint32_t *flags)
{
	bsp_io_port_t port = (port_pin >> 8U) & 0xFF;
	gpio_pin_t pin = port_pin & 0xFF;
	volatile uint8_t *p_p = GPIO_RZ_IOPORT_P_REG_GET(port, pin);
	volatile uint16_t *p_pm = GPIO_RZ_IOPORT_PM_REG_GET(port, pin);

	uint8_t p_value;
	uint16_t pm_value;

	p_value = GPIO_RZ_P_VALUE_GET(*p_p, pin);
	pm_value = GPIO_RZ_PM_VALUE_GET(*p_pm, pin);

	if (p_value) {
		*flags |= GPIO_OUTPUT_INIT_HIGH;
	} else {
		*flags |= GPIO_OUTPUT_INIT_LOW;
	}

	*flags |= ((pm_value << 16));
	return 0;
}

static int gpio_rz_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	bsp_io_port_pin_t port_pin = config->fsp_port | pin;
	uint32_t ioport_config_data = 0;
	gpio_flags_t pre_flags;
	fsp_err_t err;

	gpio_rz_pin_config_get_raw(port_pin, &pre_flags);

	if (!flags) {
		/* Disconnect mode */
#if defined(CONFIG_SOC_SERIES_RZN2L)
		GPIO_RZ_PIN_DISCONNECT(config->fsp_port, pin);
#elif defined(CONFIG_SOC_SERIES_RZG3S)
		ioport_config_data = 0;
#endif /* CONFIG_SOC_SERIES_* */
	} else if (!(flags & GPIO_OPEN_DRAIN)) {
		/* PM register */
		ioport_config_data &= GPIO_RZ_PIN_CONFIGURE_INPUT_OUTPUT_RESET;
		if (flags & GPIO_INPUT) {
			if (flags & GPIO_OUTPUT) {
				ioport_config_data |= IOPORT_CFG_PORT_DIRECTION_OUTPUT_INPUT;
			} else {
				ioport_config_data |= IOPORT_CFG_PORT_DIRECTION_INPUT;
			}
		} else if (flags & GPIO_OUTPUT) {
			ioport_config_data &= GPIO_RZ_PIN_CONFIGURE_INPUT_OUTPUT_RESET;
			ioport_config_data |= IOPORT_CFG_PORT_DIRECTION_OUTPUT;
		}
		/* P register */
		if (!(flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW))) {
			flags |= pre_flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW);
		}

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			ioport_config_data |= IOPORT_CFG_PORT_OUTPUT_HIGH;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			ioport_config_data &= ~(IOPORT_CFG_PORT_OUTPUT_HIGH);
		}
		/* PUPD register */
		if (flags & GPIO_PULL_UP) {
			ioport_config_data |= IOPORT_CFG_PULLUP_ENABLE;
		} else if (flags & GPIO_PULL_DOWN) {
			ioport_config_data |= IOPORT_CFG_PULLDOWN_ENABLE;
		}

		/* ISEL register */
		if (flags & GPIO_INT_ENABLE) {
			ioport_config_data |= GPIO_RZ_PIN_CONFIGURE_INT_ENABLE;
		} else if (flags & GPIO_INT_DISABLE) {
			ioport_config_data &= GPIO_RZ_PIN_CONFIGURE_INT_DISABLE;
		}

		/* Drive Ability register */
		ioport_config_data |= GPIO_RZ_PIN_CONFIGURE_GET(flags);
#if defined(CONFIG_SOC_SERIES_RZG3S)
		/* Filter register, see in renesas-rz-gpio-ioport.h */
		ioport_config_data |= GPIO_RZ_PIN_CONFIGURE_GET_FILTER(flags);
#elif defined(CONFIG_SOC_SERIES_RZN2L)
		/* RSEL reg */
		ioport_config_data |= IOPORT_CFG_REGION_NSAFETY;
#endif /* CONFIG_SOC_SERIES_* */
	} else {
		return -ENOTSUP;
	}

	err = config->fsp_api->pinCfg(data->fsp_ctrl, port_pin, ioport_config_data);
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
	struct gpio_rz_data *data = dev->data;
	bsp_io_port_pin_t port_pin;
	gpio_flags_t pre_flags;
	ioport_size_t value = 0;
	fsp_err_t err;

	for (uint8_t idx = 0; idx < config->ngpios; idx++) {
		if (pins & (1U << idx)) {
			port_pin = config->fsp_port | idx;
			gpio_rz_pin_config_get_raw(port_pin, &pre_flags);
			if (pre_flags & GPIO_OUTPUT_INIT_HIGH) {
				value &= (1U << idx);
			} else if (pre_flags & GPIO_OUTPUT_INIT_LOW) {
				value |= (1U << idx);
			}
		}
	}
	err = config->fsp_api->portWrite(data->fsp_ctrl, config->fsp_port, value,
					 (ioport_size_t)pins);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	return 0;
}

#define GPIO_RZ_HAS_INTERRUPT                                                                      \
	DT_HAS_COMPAT_STATUS_OKAY(renesas_rz_gpio_int) |                                           \
		DT_HAS_COMPAT_STATUS_OKAY(renesas_rz_ext_irq)

#if GPIO_RZ_HAS_INTERRUPT
static int gpio_rz_int_disable(const struct device *dev, const struct device *gpio_dev,
			       uint8_t int_num, gpio_pin_t pin)
{
#if defined(CONFIG_SOC_SERIES_RZG3S)
	volatile uint32_t *tssr = &R_INTC_IM33->TSSR0;
	volatile uint32_t *titsr = &R_INTC_IM33->TITSR0;
	volatile uint32_t *tscr = &R_INTC_IM33->TSCR;
	struct gpio_rz_int_data *data = dev->data;

	/* Get register offset base on interrupt number. */
	tssr = &tssr[int_num / 4];
	titsr = &titsr[int_num / 16];

	irq_disable(GPIO_RZ_TINT_IRQ_GET(int_num));
	/* Disable interrupt and clear interrupt source. */
	*tssr &= ~(0xFF << GPIO_RZ_TSSR_OFFSET(int_num));
	/* Reset interrupt dectect type to default. */
	*titsr &= ~(0x3 << GPIO_RZ_TITSR_OFFSET(int_num));

	/* Clear interrupt detection status. */
	if (data->irq_set_edge & BIT(int_num)) {
		*tscr &= ~BIT(int_num);
		data->irq_set_edge &= ~BIT(int_num);
	}

	data->gpio_mapping[int_num].gpio_dev = NULL;
	data->gpio_mapping[int_num].pin = UINT8_MAX;
#elif defined(CONFIG_SOC_SERIES_RZN2L)
	const struct gpio_rz_config *gpio_config = gpio_dev->config;
	const struct device *eirq_dev = gpio_config->eirq_dev[pin];

	if (device_is_ready(eirq_dev)) {
		intc_rz_ext_irq_disable(eirq_dev);
	}
#endif /* CONFIG_SOC_SERIES_* */

	return 0;
}

static int gpio_rz_int_enable(const struct device *int_dev, const struct device *gpio_dev,
			      uint8_t int_num, uint8_t irq_type, gpio_pin_t pin)
{
	if (irq_type == GPIO_RZ_INT_UNSUPPORTED) {
		return -ENOTSUP;
	}

	const struct gpio_rz_config *gpio_config = gpio_dev->config;

#if defined(CONFIG_SOC_SERIES_RZG3S)
	volatile uint32_t *tssr = &R_INTC_IM33->TSSR0;
	volatile uint32_t *titsr = &R_INTC_IM33->TITSR0;
	struct gpio_rz_int_data *int_data = int_dev->data;

	tssr = &tssr[int_num / 4];
	titsr = &titsr[int_num / 16];
	/* Select interrupt detect type. */
	*titsr &= ~(3U << GPIO_RZ_TITSR_OFFSET(int_num));
	*titsr |= (irq_type << GPIO_RZ_TITSR_OFFSET(int_num));
	/* Select interrupt source base on port and pin number.*/
	*tssr |= (GPIO_RZ_TSSR_VAL(gpio_config->port_num, pin)) << GPIO_RZ_TSSR_OFFSET(int_num);

	if (irq_type == GPIO_RZ_INT_EDGE_RISING || irq_type == GPIO_RZ_INT_EDGE_FALLING) {
		int_data->irq_set_edge |= BIT(int_num);
		/* Clear interrupt status. */
		R_INTC_IM33->TSCR &= ~BIT(int_num);
	}
	irq_enable(GPIO_RZ_TINT_IRQ_GET(int_num));
	int_data->gpio_mapping[int_num].gpio_dev = gpio_dev;
	int_data->gpio_mapping[int_num].pin = pin;
#elif defined(CONFIG_SOC_SERIES_RZN2L)
	const struct device *eirq_dev = gpio_config->eirq_dev[pin];
	struct gpio_rz_data *gpio_data = gpio_dev->data;

	gpio_data->pin[int_num] = pin;
	if (device_is_ready(eirq_dev)) {
		intc_rz_ext_irq_set_type(eirq_dev, irq_type);
		intc_rz_ext_irq_enable(eirq_dev);
		intc_rz_ext_irq_set_callback(eirq_dev, gpio_config->cb_list[int_num],
					     (void *)gpio_dev);
	}
#endif /* CONFIG_SOC_SERIES_* */

	return 0;
}

static int gpio_rz_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rz_config *config = dev->config;
	struct gpio_rz_data *data = dev->data;
	bsp_io_port_pin_t port_pin = config->fsp_port | pin;
	uint8_t int_num = config->int_num[pin];
	uint8_t irq_type = 0;
	gpio_flags_t pre_flags = 0;
	k_spinlock_key_t key;
	int ret = 0;

	if (int_num >= GPIO_RZ_MAX_INT_NUM) {
		LOG_DEV_ERR(dev, "Invalid interrupt:%d >= %d", int_num, GPIO_RZ_MAX_INT_NUM);
	}

	if (pin > config->ngpios) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (mode == GPIO_INT_MODE_DISABLED) {
		gpio_rz_pin_config_get_raw(port_pin, &pre_flags);
		pre_flags |= GPIO_INT_DISABLE;
		gpio_rz_pin_configure(dev, pin, pre_flags);
		gpio_rz_int_disable(config->int_dev, dev, int_num, pin);
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

	ret = gpio_rz_int_enable(config->int_dev, dev, int_num, irq_type, pin);
	if (ret == 0) {
		gpio_rz_pin_config_get_raw(port_pin, &pre_flags);
		pre_flags |= GPIO_INT_ENABLE;
		gpio_rz_pin_configure(dev, pin, pre_flags);
	}

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

static void gpio_rz_isr(uint16_t irq, void *param)
{
#if defined(CONFIG_SOC_SERIES_RZG3S)
	const struct device *dev = param;
	struct gpio_rz_int_data *int_data = dev->data;
	volatile uint32_t *tscr = &R_INTC_IM33->TSCR;

	if (!(*tscr & BIT(irq))) {
		LOG_DEV_DBG(dev, "tint:%u spurious irq, status 0", irq);
		return;
	}

	if (int_data->irq_set_edge & BIT(irq)) {
		*tscr &= ~BIT(irq);
	}

	uint8_t pin = int_data->gpio_mapping[irq].pin;
	const struct device *gpio_dev = int_data->gpio_mapping[irq].gpio_dev;
	struct gpio_rz_data *gpio_data = gpio_dev->data;

	gpio_fire_callbacks(&gpio_data->cb, gpio_dev, BIT(pin));
#elif defined(CONFIG_SOC_SERIES_RZN2L)
	const struct device *gpio_dev = (const struct device *)param;
	struct gpio_rz_data *gpio_data = gpio_dev->data;
	uint8_t pin = gpio_data->pin[irq];

	gpio_fire_callbacks(&gpio_data->cb, gpio_dev, BIT(pin));
#endif /* CONFIG_SOC_SERIES_* */
}

#endif /* GPIO_RZ_HAS_INTERRUPT */

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
#if GPIO_RZ_HAS_INTERRUPT
	.pin_interrupt_configure = gpio_rz_pin_interrupt_configure,
	.manage_callback = gpio_rz_manage_callback,
#endif
};

/*Initialize GPIO interrupt device*/
#define GPIO_RZ_ISR_DEFINE(irq_num, _)                                                             \
	static void rz_gpio_isr##irq_num(void *param)                                              \
	{                                                                                          \
		gpio_rz_isr(irq_num, param);                                                       \
	}

#define GPIO_RZ_ALL_ISR_DEFINE(irq_num) LISTIFY(irq_num, GPIO_RZ_ISR_DEFINE, ())

#if defined(CONFIG_SOC_SERIES_RZG3S)

#define GPIO_RZ_INT_DEFINE(inst) .int_dev = DEVICE_DT_GET_OR_NULL(DT_INST(0, renesas_rz_gpio_int))

static int gpio_rz_int_init(const struct device *dev)
{
	const struct gpio_rz_tint_config *config = dev->config;

	config->gpio_int_init();
	return 0;
}

#define GPIO_RZ_TINT_CONNECT(irq_num, node_id)                                                     \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, irq_num, irq),                                          \
		    DT_IRQ_BY_IDX(node_id, irq_num, priority), rz_gpio_isr##irq_num,               \
		    DEVICE_DT_GET(node_id), 0);

#define GPIO_RZ_TINT_CONNECT_FUNC(node_id)                                                         \
	static void rz_gpio_tint_connect_func##node_id(void)                                       \
	{                                                                                          \
		LISTIFY(DT_NUM_IRQS(node_id),                    \
			GPIO_RZ_TINT_CONNECT, (;),               \
			node_id)                                 \
	}
/* Initialize GPIO device*/
#define GPIO_RZ_INT_INIT(node_id)                                                                  \
	GPIO_RZ_ALL_ISR_DEFINE(DT_NUM_IRQS(node_id))                                               \
	GPIO_RZ_TINT_CONNECT_FUNC(node_id)                                                         \
	static const struct gpio_rz_tint_config rz_gpio_tint_cfg_##node_id = {                     \
		.gpio_int_init = rz_gpio_tint_connect_func##node_id,                               \
	};                                                                                         \
	static struct gpio_rz_int_data rz_gpio_tint_data_##node_id = {};                           \
	DEVICE_DT_DEFINE(node_id, gpio_rz_int_init, NULL, &rz_gpio_tint_data_##node_id,            \
			 &rz_gpio_tint_cfg_##node_id, POST_KERNEL,                                 \
			 UTIL_DEC(CONFIG_GPIO_INIT_PRIORITY), NULL);
DT_FOREACH_STATUS_OKAY(renesas_rz_gpio_int, GPIO_RZ_INT_INIT)

#elif defined(CONFIG_SOC_SERIES_RZN2L) && GPIO_RZ_HAS_INTERRUPT

GPIO_RZ_ALL_ISR_DEFINE(GPIO_RZ_MAX_INT_NUM)

#define EIRQ_CB_GET(eirq_line, _) [eirq_line] = rz_gpio_isr##eirq_line

#define EIRQ_DEV_LABEL_GET(inst, idx) CONCAT(irq, DT_INST_PROP_BY_IDX(inst, irqs, UTIL_INC(idx)))

#define EIRQ_DEV_GET(idx, inst)                                                                    \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, irqs, idx),                                         \
		    ([DT_INST_PROP_BY_IDX(inst, irqs, idx)] =                                      \
			     DEVICE_DT_GET_OR_NULL(DT_NODELABEL(EIRQ_DEV_LABEL_GET(inst, idx))),), \
		    ())

#define ALL_EIRQ_DEV_GET(inst)                                                                     \
	FOR_EACH_FIXED_ARG(EIRQ_DEV_GET, (), inst,                                                 \
			   LISTIFY(DT_INST_PROP_LEN_OR(inst, irqs, 0), VALUE_2X, (,)))

#define GPIO_RZ_INT_DEFINE(inst)                                                                   \
	.eirq_dev = {ALL_EIRQ_DEV_GET(inst)},                                                      \
	.cb_list = {LISTIFY(GPIO_RZ_MAX_INT_NUM, EIRQ_CB_GET, (,))}
#else
#define GPIO_RZ_INT_DEFINE(inst)
#endif /* CONFIG_SOC_SERIES_* */

#define VALUE_2X(i, _) UTIL_X2(i)
#define PIN_IRQ_GET(idx, inst)                                                                     \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, irqs, idx),                                         \
		    ([DT_INST_PROP_BY_IDX(inst, irqs, idx)] =                                      \
			     DT_INST_PROP_BY_IDX(inst, irqs, UTIL_INC(idx)),),                     \
		    ())

#define PIN_IRQS_GET(inst)                                                                         \
	FOR_EACH_FIXED_ARG(PIN_IRQ_GET, (), inst,                                                  \
			   LISTIFY(DT_INST_PROP_LEN_OR(inst, irqs, 0), VALUE_2X, (,)))

#define RZG_GPIO_PORT_INIT(inst)                                                                   \
	static ioport_cfg_t g_ioport_##inst##_cfg = {                                              \
		.number_of_pins = 0,                                                               \
		.p_pin_cfg_data = NULL,                                                            \
		.p_extend = NULL,                                                                  \
	};                                                                                         \
	static const struct gpio_rz_config gpio_rz_##inst##_config = {                             \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask =                                                   \
					(gpio_port_pins_t)GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),   \
			},                                                                         \
		.fsp_port = (uint32_t)DT_INST_REG_ADDR(inst),                                      \
		.port_num = (uint8_t)DT_NODE_CHILD_IDX(DT_DRV_INST(inst)),                         \
		.ngpios = (uint8_t)DT_INST_PROP(inst, ngpios),                                     \
		.fsp_cfg = &g_ioport_##inst##_cfg,                                                 \
		.fsp_api = &g_ioport_on_ioport,                                                    \
		.int_num = {PIN_IRQS_GET(inst)},                                                   \
		GPIO_RZ_INT_DEFINE(inst)};                                                         \
	static ioport_instance_ctrl_t g_ioport_##inst##_ctrl;                                      \
	static struct gpio_rz_data gpio_rz_##inst##_data = {                                       \
		.fsp_ctrl = &g_ioport_##inst##_ctrl,                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &gpio_rz_##inst##_data, &gpio_rz_##inst##_config,  \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_rz_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RZG_GPIO_PORT_INIT)
