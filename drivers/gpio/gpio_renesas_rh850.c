/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rh850_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include "gpio_renesas_rh850.h"
#include "r_ioport.h"
#include <soc.h>

struct gpio_rh850_config {
	struct gpio_driver_config common;
	uint8_t port_num;
	uint8_t port_type;
	uint8_t protect_bit_disable;
	R_PORT_Type *port;
	const ioport_cfg_t *fsp_cfg;
	const ioport_api_t *fsp_api;
	const struct device *gpio_int_dev;
	/* Contains list of interrupt index corresponding to pin: int_num[pin_num] = irq_id */
	uint8_t int_num[GPIO_RH850_MAX_EXT_INT_NUM];
#if defined(CONFIG_RENESAS_RH850_EXT_IRQ)
	/* Contains list of ext_irq device */
	const struct device *ext_irq_dev[GPIO_RH850_MAX_EXT_INT_NUM];
	/* Contains list of callback */
	void (*callback_list[GPIO_RH850_MAX_EXT_INT_NUM])(void *arg);
#endif
};

struct gpio_rh850_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	ioport_instance_ctrl_t *fsp_ctrl;
	struct k_spinlock lock;
#ifdef CONFIG_RENESAS_RH850_EXT_IRQ
	/* Contains list of pins corresponding to interrupt index: pin[irq_id] = pin */
	uint8_t pin[GPIO_RH850_MAX_EXT_INT_NUM];
#endif
};

#ifdef CONFIG_RENESAS_RH850_EXT_IRQ
static int gpio_rh850_int_enable(const struct device *gpio_int_dev, const struct device *gpio_dev,
				   uint8_t int_num, uint8_t irq_type, gpio_pin_t pin)
{
	if (irq_type == GPIO_RH850_INT_UNSUPPORTED) {
		return -ENOTSUP;
	}

	ARG_UNUSED(gpio_int_dev);
	const struct gpio_rh850_config *gpio_config = gpio_dev->config;
	const struct device *ext_irq_dev = gpio_config->ext_irq_dev[pin];
	struct gpio_rh850_data *gpio_data = gpio_dev->data;

	gpio_data->pin[int_num] = pin;
	if (device_is_ready(ext_irq_dev)) {
		intc_rh850_ext_irq_set_type(ext_irq_dev, irq_type);
		intc_rh850_ext_irq_set_callback(ext_irq_dev, gpio_config->callback_list[int_num],
						  (void *)gpio_dev);
		intc_rh850_ext_irq_enable(ext_irq_dev);
	}

	return 0;
}

static int gpio_rh850_int_disable(const struct device *dev, const struct device *gpio_dev,
				    uint8_t irq_type, gpio_pin_t pin)
{
	ARG_UNUSED(dev);
	const struct gpio_rh850_config *gpio_config = gpio_dev->config;
	const struct device *ext_irq_dev = gpio_config->ext_irq_dev[pin];

	if (device_is_ready(ext_irq_dev)) {
		intc_rh850_ext_irq_set_type(ext_irq_dev, irq_type);
		intc_rh850_ext_irq_disable(ext_irq_dev);
	}

	return 0;
}

static void gpio_rh850_isr(uint16_t irq, void *param)
{
	const struct device *gpio_dev = (const struct device *)param;
	struct gpio_rh850_data *gpio_data = gpio_dev->data;
	uint8_t pin = gpio_data->pin[irq];

	gpio_fire_callbacks(&gpio_data->callbacks, gpio_dev, BIT(pin));
}
#endif

static int gpio_rh850_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rh850_config *config = dev->config;
	struct rh850_pinctrl_soc_pin pincfg;
	bsp_io_port_pin_t port_pin;
	uint32_t ioport_config_data;

	/* Check pin number is valid*/
	if (pin >= RH850_PINCTRL_PIN_MAX) {
		return -EINVAL;
	}

	/* Store Alternative mode value and current pin driver level */
	ioport_config_data = R_BSP_PORT_TYPE[config->port_type]->PCRn_m[config->port_num].PCR[pin] &
							GPIO_RH850_CURRENT_PORT_CONFIG_MASK;

	/* Calculate port identifier */
	pincfg.port_id = ((config->protect_bit_disable << RH850_FSP_PORT_PWE_POS) |
			  (config->port_type << RH850_FSP_PORT_TYPE_POS) |
			  (config->port_num << RH850_FSP_PORT_GROUP_POS));
	pincfg.pin_num = pin;
	port_pin = pincfg.port_id | pincfg.pin_num;

	if (!flags) {
		/* Disable Pin: Set default pin mode is input mode */
		WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PM_Pos, 1);
	} else if (!(flags & (GPIO_OPEN_DRAIN | GPIO_OPEN_SOURCE))) {
		/* Input/Output mode */
		if (flags & GPIO_OUTPUT) {
			/* Pull-up/Pull-down only support in Input-mode */
			if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
				return -ENOTSUP;
			}
			/* PM register: Set input/output port mode */
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PM_Pos, 0);

			/* Set Push-Pull mode*/
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PODCE_Pos, 0);
			if (flags & GPIO_INPUT) {
				/* Set Bi-direction mode*/
				WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PBDC_Pos, 1);
			}
		} else if (flags & GPIO_INPUT) {
			/* PM register: Set input/output port mode */
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PM_Pos, 1);

			/* PIBC register: Enable input buffer*/
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PIBC_Pos, 1);

			/* Pull-up/down only support in Input Mode */
			/* PU/PD register: Pull-up/Pull-down mode */
			if (flags & GPIO_PULL_UP) {
				WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PU_Pos, 1);
			} else if (flags & GPIO_PULL_DOWN) {
				WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PD_Pos, 1);
			}
		}

		/* P register */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_P_Pos, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_P_Pos, 0);
		}
		/*
		 * PMC register
		 * Interrupt is alternative mode
		 * Normal is port mode
		 * Input Buffer need to disable
		 */
		if (flags & GPIO_INT_ENABLE) {
			/* If Port mode configure GPIO_OUTPUT then return ENOTSUP */
			/* Due to GPIO Output mode does not support in Interrupt mode */
			if (!(ioport_config_data & R_PORT_PCRn_m_PCR_PM_Msk)) {
				return -ENOTSUP;
			}
			/* Config Alternative port mode */
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PMC_Pos, 1);
			/* Disable input buffer */
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PIBC_Pos, 0);
		} else if (flags & GPIO_INT_DISABLE) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PMC_Pos, 0);
		}
	} else if (flags & GPIO_OPEN_SOURCE) {
		/* Pull-up/Pull-down only support in Input-mode */
		if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			return -ENOTSUP;
		}

		/* In Open-source mode, It does not support pull-up/pull down by software */
		/* Pull-up/Pull-down need to use external */
		/* Configure P-Channel Open Drain */
		WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PODCE_Pos, 3);

		/* PM register: Set input/output port mode */
		WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PM_Pos, 0);

		/* P register: High/Low level port */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_P_Pos, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_P_Pos, 0);
		}
	} else {
		/* Pull-up/Pull-down only support in Input-mode */
		if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			return -ENOTSUP;
		}

		/* In Open-drain mode, It does not support pull-up/pull down by software */
		/* Pull-up/Pull-down need to use external */
		/* Configure N-Channel Open Drain */
		WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PODCE_Pos, 2);

		/* PM register: Set input/output port mode */
		WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_PM_Pos, 0);

		/* P register: High/Low level port */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_P_Pos, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			WRITE_BIT(ioport_config_data, R_PORT_PCRn_m_PCR_P_Pos, 0);
		}
	}

	pincfg.cfg = ioport_config_data;

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

__maybe_unused static int gpio_rh850_pin_get_config(const struct device *dev, gpio_pin_t pin,
						    gpio_flags_t *flags)
{
	const struct gpio_rh850_config *config = dev->config;
	uint32_t pincfg;

	if (pin >= RH850_PINCTRL_PIN_MAX) {
		return -EINVAL;
	}

	memset(flags, 0, sizeof(gpio_flags_t));

	pincfg = R_BSP_PORT_TYPE[config->port_type]->PCRn_m[config->port_num].PCR[pin];

	/* Get Input/Output Pin Mode */
	if (!(pincfg & BIT(R_PORT_PCRn_m_PCR_PM_Pos))) {
		*flags |= GPIO_OUTPUT;
		if (pincfg & BIT(R_PORT_PCRn_m_PCR_P_Pos)) {
			*flags |= GPIO_OUTPUT_INIT_HIGH;
		} else {
			*flags |= GPIO_OUTPUT_INIT_LOW;
		}
	} else {
		*flags |= GPIO_INPUT;
	}

	if (pincfg & BIT(R_PORT_PCRn_m_PCR_PU_Pos)) {
		*flags |= GPIO_PULL_UP;
	} else if (pincfg & BIT(R_PORT_PCRn_m_PCR_PD_Pos)) {
		*flags |= GPIO_PULL_DOWN;
	}

	/* Extract PODCE field (2-bit) in PCR register
	 *
	 * PODCE[1:0]:
	 *   0b00 : Push-pull output
	 *   0b10 : N-channel open-drain (pull-down only)
	 *   0b11 : P-channel open-drain / open-source (pull-up only)
	 */
	if (((pincfg >> R_PORT_PCRn_m_PCR_PODCE_Pos) & 0x3U) == 2U) {
		*flags |= GPIO_LINE_OPEN_DRAIN;
	} else if (((pincfg >> R_PORT_PCRn_m_PCR_PODCE_Pos) & 0x3U) == 3U) {
		*flags |= GPIO_OPEN_SOURCE;
	} else if (((pincfg >> R_PORT_PCRn_m_PCR_PODCE_Pos) & 0x3U) == 0U) {
		*flags |= GPIO_PUSH_PULL;
	}

	return 0;
}

static int gpio_rh850_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_rh850_config *config = dev->config;
	struct gpio_rh850_data *data = dev->data;
	ioport_size_t port_value;
	bsp_io_port_t port_pin;
	fsp_err_t err;

	port_pin = ((config->protect_bit_disable << RH850_FSP_PORT_PWE_POS) |
		    (config->port_type << RH850_FSP_PORT_TYPE_POS) |
		    (config->port_num << RH850_FSP_PORT_GROUP_POS));

	err = config->fsp_api->portRead(data->fsp_ctrl, port_pin, &port_value);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*value = (gpio_port_value_t)port_value;

	return 0;
}

static int gpio_rh850_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_rh850_config *config = dev->config;
	struct gpio_rh850_data *data = dev->data;
	ioport_size_t port_mask = (ioport_size_t)mask;
	ioport_size_t port_value = (ioport_size_t)value;
	bsp_io_port_t port_pin;
	fsp_err_t err;

	port_pin = ((config->protect_bit_disable << RH850_FSP_PORT_PWE_POS) |
		    (config->port_type << RH850_FSP_PORT_TYPE_POS) |
		    (config->port_num << RH850_FSP_PORT_GROUP_POS));

	err = config->fsp_api->portWrite(data->fsp_ctrl, port_pin, port_value, port_mask);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rh850_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rh850_config *config = dev->config;
	struct gpio_rh850_data *data = dev->data;
	ioport_size_t mask = (ioport_size_t)pins;
	ioport_size_t value = (ioport_size_t)pins;
	bsp_io_port_t port_pin;
	fsp_err_t err;

	port_pin = ((config->protect_bit_disable << RH850_FSP_PORT_PWE_POS) |
		    (config->port_type << RH850_FSP_PORT_TYPE_POS) |
		    (config->port_num << RH850_FSP_PORT_GROUP_POS));

	err = config->fsp_api->portWrite(data->fsp_ctrl, port_pin, value, mask);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rh850_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rh850_config *config = dev->config;
	struct gpio_rh850_data *data = dev->data;
	ioport_size_t mask = (ioport_size_t)pins;
	ioport_size_t value = 0x00;
	bsp_io_port_t port_pin;
	fsp_err_t err;

	port_pin = ((config->protect_bit_disable << RH850_FSP_PORT_PWE_POS) |
		    (config->port_type << RH850_FSP_PORT_TYPE_POS) |
		    (config->port_num << RH850_FSP_PORT_GROUP_POS));

	err = config->fsp_api->portWrite(data->fsp_ctrl, port_pin, value, mask);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int gpio_rh850_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rh850_config *config = dev->config;
	R_PORT_Type *port = config->port;
	bsp_io_port_t port_pin;

	/* Get port pin value */
	port_pin = ((config->protect_bit_disable << RH850_FSP_PORT_PWE_POS) |
		    (config->port_type << RH850_FSP_PORT_TYPE_POS) |
		    (config->port_num << RH850_FSP_PORT_GROUP_POS));

	/* Enable access to Protected Write Enable register */
	R_BSP_PinAccessEnable();

	/* Disable port protection for enable write register */
	R_BSP_PortWriteEnable(port_pin);

	port->PORT_CTR[0].PNOT = pins;

	/* Enable port protection for disable write register */
	R_BSP_PortWriteDisable(port_pin);

	/* Disable access to Protected Write Enable register */
	R_BSP_PinAccessDisable();

	return 0;
}

#ifdef CONFIG_RENESAS_RH850_EXT_IRQ
static int gpio_rh850_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rh850_config *config = dev->config;
	struct gpio_rh850_data *data = dev->data;
	uint32_t rh850_flags;
	k_spinlock_key_t key;
	uint8_t int_num;
	uint8_t irq_type = 0;
	uint32_t pcr;
	int ret = 0;

	if (pin >= RH850_PINCTRL_PIN_MAX) {
		return -EINVAL;
	}

	int_num = config->int_num[pin];

	/*
	 * External interrupt is an alternative input function (PMC = 1), so it
	 * cannot be used while the pin is configured as output.
	 */
	pcr = R_BSP_PORT_TYPE[config->port_type]->PCRn_m[config->port_num].PCR[pin];
	if ((mode != GPIO_INT_MODE_DISABLED) && ((pcr & R_PORT_PCRn_m_PCR_PM_Msk) == 0U)) {
		return -ENOTSUP;
	}

	/* Check mapping interrupt number value is valid */
	if ((int_num >= GPIO_RH850_MAX_EXT_INT_NUM) || (int_num == GPIO_RH850_INT_UNMAPPED)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		ret = gpio_rh850_pin_get_config(dev, pin, &rh850_flags);
		if (ret) {
			ret = -EIO;
			break;
		}

		gpio_rh850_int_disable(config->gpio_int_dev, dev, irq_type, pin);
		rh850_flags |= GPIO_INT_DISABLE;
		gpio_rh850_pin_configure(dev, pin, rh850_flags);
		goto exit_unlock;
	case GPIO_INT_MODE_EDGE:
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			irq_type = GPIO_RH850_INT_EDGE_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			irq_type = GPIO_RH850_INT_EDGE_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			irq_type = GPIO_RH850_INT_BOTH_EDGE;
			break;
		default:
			ret = -EINVAL;
			goto exit_unlock;
		}
		break;
	case GPIO_INT_MODE_LEVEL:
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			irq_type = GPIO_RH850_INT_LEVEL_LOW;
			break;
		case GPIO_INT_TRIG_HIGH:
			irq_type = GPIO_RH850_INT_LEVEL_HIGH;
			break;
		default:
			ret = -EINVAL;
			goto exit_unlock;
		}
		break;
	default:
		ret = -EINVAL;
		goto exit_unlock;
	}

	ret = gpio_rh850_pin_get_config(dev, pin, &rh850_flags);
	if (ret == 0) {
		rh850_flags |= GPIO_INT_ENABLE;
		ret = gpio_rh850_pin_configure(dev, pin, rh850_flags);
		if (ret) {
			goto exit_unlock;
		}
		ret = gpio_rh850_int_enable(config->gpio_int_dev, dev, int_num, irq_type, pin);
	}

exit_unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int gpio_rh850_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct gpio_rh850_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}
#endif

static DEVICE_API(gpio, gpio_rh850_driver_api) = {
	.pin_configure = gpio_rh850_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_rh850_pin_get_config,
#endif
	.port_get_raw = gpio_rh850_port_get_raw,
	.port_set_masked_raw = gpio_rh850_port_set_masked_raw,
	.port_set_bits_raw = gpio_rh850_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rh850_port_clear_bits_raw,
	.port_toggle_bits = gpio_rh850_port_toggle_bits,
#ifdef CONFIG_RENESAS_RH850_EXT_IRQ
	.pin_interrupt_configure = gpio_rh850_pin_interrupt_configure,
	.manage_callback = gpio_rh850_manage_callback,
#endif
};

/*
 * ************************* GPIO DEFINITION ***************************
 */
#if defined(CONFIG_RENESAS_RH850_EXT_IRQ)
/* ISR definitions */
#define GPIO_RH850_ISR_DEFINE(idx, _)                                                    \
	static void rh850_gpio_isr##idx(void *param)                                         \
	{                                                                                    \
		gpio_rh850_isr(idx, param);                                                      \
	}

#define GPIO_RH850_ALL_ISR_DEFINE(irq_num) LISTIFY(irq_num, GPIO_RH850_ISR_DEFINE, ())

GPIO_RH850_ALL_ISR_DEFINE(GPIO_RH850_MAX_EXT_INT_NUM)

/* [ext_irq] = isr_function */
#define EXT_IRQ_CALLBACK_GET(ext_irq, _) [ext_irq] = rh850_gpio_isr##ext_irq

/* [pin] = ext_irq_device_ptr */
#define EXT_IRQ_DEV_GET(idx, inst)                                            \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, irqs, idx),                        \
		([DT_INST_PHA_BY_IDX(inst, irqs, idx, pin)] =                         \
			DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE_BY_IDX(inst, irqs, idx)),), \
		())

#define ALL_EXT_IRQ_DEV_GET(inst) LISTIFY(DT_INST_PROP_LEN_OR(inst, irqs, 0), \
				EXT_IRQ_DEV_GET, (), inst)

/* [pin] = irq_number */
#define PIN_IRQ_GET(idx, inst)                                                         \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, irqs, idx),                                 \
		([DT_INST_PHA_BY_IDX(inst, irqs, idx, pin)] =                                  \
			DT_REG_ADDR(DT_INST_PHANDLE_BY_IDX(inst, irqs, idx)),),                    \
		())

#define GPIO_RH850_INT_DEFINE(inst)                                                    \
	.ext_irq_dev = {ALL_EXT_IRQ_DEV_GET(inst)},                                        \
	.callback_list = {LISTIFY(GPIO_RH850_MAX_EXT_INT_NUM, EXT_IRQ_CALLBACK_GET, (,)) }
#else
#define GPIO_RH850_INT_DEFINE(inst)
#endif  /* CONFIG_RENESAS_RH850_EXT_IRQ */
/*
 * ************************* DRIVER INITIALIZE ***************************
 */
#define PIN_IRQS_GET(inst) LISTIFY(DT_INST_PROP_LEN_OR(inst, irqs, 0), PIN_IRQ_GET, (), inst)

#define RH850_GPIO_PORT_INIT(inst)                                                     \
	static ioport_cfg_t g_ioport_##inst##_cfg = {                                      \
		.number_of_pins = 0,                                                           \
		.p_pin_cfg_data = NULL,                                                        \
		.p_extend = NULL,                                                              \
	};                                                                                 \
	static const struct gpio_rh850_config gpio_rh850_##inst##_config = {               \
		.common =                                                                      \
			{                                                                          \
				.port_pin_mask =                                            \
					(gpio_port_pins_t)GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),\
			},                                                                         \
		.port_num = (uint8_t)DT_INST_PROP(inst, port),                                 \
		.port_type = DT_INST_ENUM_IDX(inst, port_type),                                \
		.protect_bit_disable = (uint8_t)DT_INST_PROP(inst, protected_write_mask),      \
		.port = (R_PORT_Type *)DT_INST_REG_ADDR(inst),                                 \
		.fsp_cfg = &g_ioport_##inst##_cfg,                                             \
		.fsp_api = &g_ioport_on_ioport,                                                \
		.int_num = {[0 ...(GPIO_RH850_MAX_EXT_INT_NUM - 1)] = GPIO_RH850_INT_UNMAPPED, \
		PIN_IRQS_GET(inst)},                                                           \
		GPIO_RH850_INT_DEFINE(inst)                                                    \
	};                                                                                 \
	static ioport_instance_ctrl_t g_ioport_##inst##_ctrl;                              \
	static struct gpio_rh850_data gpio_rh850_##inst##_data = {                         \
		.fsp_ctrl = &g_ioport_##inst##_ctrl,                                           \
	};                                                                                 \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &gpio_rh850_##inst##_data,                 \
			      &gpio_rh850_##inst##_config, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, \
			      &gpio_rh850_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RH850_GPIO_PORT_INIT)
