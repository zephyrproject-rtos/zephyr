/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 * Copyright (c) 2023 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_gpio_bank

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include <am_mcu_apollo.h>

typedef void (*ambiq_gpio_cfg_func_t)(void);

struct ambiq_gpio_config {
	struct gpio_driver_config common;
	uint32_t base;
	uint32_t offset;
	uint32_t irq_num;
	ambiq_gpio_cfg_func_t cfg_func;
	uint8_t ngpios;
};

struct ambiq_gpio_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
	struct k_spinlock lock;
};

static int ambiq_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	pin += (dev_cfg->offset >> 2);

	am_hal_gpio_pincfg_t pincfg = am_hal_gpio_pincfg_default;

	if (flags & GPIO_INPUT) {
		pincfg = am_hal_gpio_pincfg_input;
		if (flags & GPIO_PULL_UP) {
			pincfg.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLUP_50K;
		} else if (flags & GPIO_PULL_DOWN) {
			pincfg.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLDOWN_50K;
		}
	}
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				pincfg.GP.cfg_b.eGPOutCfg = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN;
			}
		} else {
			pincfg.GP.cfg_b.eGPOutCfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
		}
	}
	if (flags & GPIO_DISCONNECTED) {
		pincfg = am_hal_gpio_pincfg_disabled;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		pincfg.GP.cfg_b.eCEpol = AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH;
		am_hal_gpio_state_write(pin, AM_HAL_GPIO_OUTPUT_SET);

	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		pincfg.GP.cfg_b.eCEpol = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW;
		am_hal_gpio_state_write(pin, AM_HAL_GPIO_OUTPUT_CLEAR);
	}

	am_hal_gpio_pinconfig(pin, pincfg);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int ambiq_gpio_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	am_hal_gpio_pincfg_t pincfg;

	pin += (dev_cfg->offset >> 2);

	am_hal_gpio_pinconfig_get(pin, &pincfg);

	if (pincfg.GP.cfg_b.eGPOutCfg == AM_HAL_GPIO_PIN_OUTCFG_DISABLE &&
	    pincfg.GP.cfg_b.eGPInput == AM_HAL_GPIO_PIN_INPUT_NONE) {
		*out_flags = GPIO_DISCONNECTED;
	}
	if (pincfg.GP.cfg_b.eGPInput == AM_HAL_GPIO_PIN_INPUT_ENABLE) {
		*out_flags = GPIO_INPUT;
		if (pincfg.GP.cfg_b.ePullup == AM_HAL_GPIO_PIN_PULLUP_50K) {
			*out_flags |= GPIO_PULL_UP;
		} else if (pincfg.GP.cfg_b.ePullup == AM_HAL_GPIO_PIN_PULLDOWN_50K) {
			*out_flags |= GPIO_PULL_DOWN;
		}
	}
	if (pincfg.GP.cfg_b.eGPOutCfg == AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL) {
		*out_flags = GPIO_OUTPUT | GPIO_PUSH_PULL;
		if (pincfg.GP.cfg_b.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH) {
			*out_flags |= GPIO_OUTPUT_HIGH;
		} else if (pincfg.GP.cfg_b.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW) {
			*out_flags |= GPIO_OUTPUT_LOW;
		}
	}
	if (pincfg.GP.cfg_b.eGPOutCfg == AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN) {
		*out_flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;
		if (pincfg.GP.cfg_b.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH) {
			*out_flags |= GPIO_OUTPUT_HIGH;
		} else if (pincfg.GP.cfg_b.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW) {
			*out_flags |= GPIO_OUTPUT_LOW;
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GET_DIRECTION
static int ambiq_gpio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					 gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	am_hal_gpio_pincfg_t pincfg;
	gpio_port_pins_t ip = 0;
	gpio_port_pins_t op = 0;
	uint32_t pin_offset = dev_cfg->offset >> 2;

	if (inputs != NULL) {
		for (int i = 0; i < dev_cfg->ngpios; i++) {
			if ((map >> i) & 1) {
				am_hal_gpio_pinconfig_get(i + pin_offset, &pincfg);
				if (pincfg.GP.cfg_b.eGPInput == AM_HAL_GPIO_PIN_INPUT_ENABLE) {
					ip |= BIT(i);
				}
			}
		}
		*inputs = ip;
	}
	if (outputs != NULL) {
		for (int i = 0; i < dev_cfg->ngpios; i++) {
			if ((map >> i) & 1) {
				am_hal_gpio_pinconfig_get(i + pin_offset, &pincfg);
				if (pincfg.GP.cfg_b.eGPOutCfg == AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL ||
				    pincfg.GP.cfg_b.eGPOutCfg == AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN) {
					op |= BIT(i);
				}
			}
		}
		*outputs = op;
	}

	return 0;
}
#endif

static int ambiq_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	*value = (*AM_HAL_GPIO_RDn(dev_cfg->offset >> 2));

	return 0;
}

static int ambiq_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	uint32_t pin_offset = dev_cfg->offset >> 2;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((mask >> i) & 1) {
			am_hal_gpio_state_write(i + pin_offset, ((value >> i) & 1));
		}
	}

	return 0;
}

static int ambiq_gpio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	uint32_t pin_offset = dev_cfg->offset >> 2;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((pins >> i) & 1) {
			am_hal_gpio_state_write(i + pin_offset, AM_HAL_GPIO_OUTPUT_SET);
		}
	}

	return 0;
}

static int ambiq_gpio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	uint32_t pin_offset = dev_cfg->offset >> 2;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((pins >> i) & 1) {
			am_hal_gpio_state_write(i + pin_offset, AM_HAL_GPIO_OUTPUT_CLEAR);
		}
	}

	return 0;
}

static int ambiq_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	uint32_t pin_offset = dev_cfg->offset >> 2;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((pins >> i) & 1) {
			am_hal_gpio_state_write(i + pin_offset, AM_HAL_GPIO_OUTPUT_TOGGLE);
		}
	}

	return 0;
}

static void ambiq_gpio_isr(const struct device *dev)
{
	struct ambiq_gpio_data *const data = dev->data;
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	uint32_t int_status;

	am_hal_gpio_interrupt_irq_status_get(dev_cfg->irq_num, false, &int_status);
	am_hal_gpio_interrupt_irq_clear(dev_cfg->irq_num, int_status);

	gpio_fire_callbacks(&data->cb, dev, int_status);
}

static int ambiq_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	struct ambiq_gpio_data *const data = dev->data;

	am_hal_gpio_pincfg_t pincfg;
	int gpio_pin = pin + (dev_cfg->offset >> 2);
	uint32_t int_status;
	int ret;

	ret = am_hal_gpio_pinconfig_get(gpio_pin, &pincfg);

	if (mode == GPIO_INT_MODE_DISABLED) {
		pincfg.GP.cfg_b.eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE;
		ret = am_hal_gpio_pinconfig(gpio_pin, pincfg);

		k_spinlock_key_t key = k_spin_lock(&data->lock);

		ret = am_hal_gpio_interrupt_irq_status_get(dev_cfg->irq_num, false, &int_status);
		ret = am_hal_gpio_interrupt_irq_clear(dev_cfg->irq_num, int_status);
		ret = am_hal_gpio_interrupt_control(AM_HAL_GPIO_INT_CHANNEL_0,
						    AM_HAL_GPIO_INT_CTRL_INDV_DISABLE,
						    (void *)&gpio_pin);
		k_spin_unlock(&data->lock, key);

	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			return -ENOTSUP;
		}
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			pincfg.GP.cfg_b.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
			break;
		case GPIO_INT_TRIG_HIGH:
			pincfg.GP.cfg_b.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI;
			break;
		case GPIO_INT_TRIG_BOTH:
			/*
			 * GPIO_INT_TRIG_BOTH is not supported on Ambiq Apollo4 Plus Platform
			 * ERR008: GPIO: Dual-edge interrupts are not vectoring
			 */
			return -ENOTSUP;
		default:
			return -EINVAL;
		}
		ret = am_hal_gpio_pinconfig(gpio_pin, pincfg);

		irq_enable(dev_cfg->irq_num);

		k_spinlock_key_t key = k_spin_lock(&data->lock);

		ret = am_hal_gpio_interrupt_irq_status_get(dev_cfg->irq_num, false, &int_status);
		ret = am_hal_gpio_interrupt_irq_clear(dev_cfg->irq_num, int_status);
		ret = am_hal_gpio_interrupt_control(AM_HAL_GPIO_INT_CHANNEL_0,
						    AM_HAL_GPIO_INT_CTRL_INDV_ENABLE,
						    (void *)&gpio_pin);
		k_spin_unlock(&data->lock, key);
	}
	return ret;
}

static int ambiq_gpio_manage_callback(const struct device *dev, struct gpio_callback *callback,
				      bool set)
{
	struct ambiq_gpio_data *const data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static int ambiq_gpio_init(const struct device *port)
{
	const struct ambiq_gpio_config *const dev_cfg = port->config;

	NVIC_ClearPendingIRQ(dev_cfg->irq_num);

	dev_cfg->cfg_func();

	return 0;
}

static const struct gpio_driver_api ambiq_gpio_drv_api = {
	.pin_configure = ambiq_gpio_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = ambiq_gpio_get_config,
#endif
	.port_get_raw = ambiq_gpio_port_get_raw,
	.port_set_masked_raw = ambiq_gpio_port_set_masked_raw,
	.port_set_bits_raw = ambiq_gpio_port_set_bits_raw,
	.port_clear_bits_raw = ambiq_gpio_port_clear_bits_raw,
	.port_toggle_bits = ambiq_gpio_port_toggle_bits,
	.pin_interrupt_configure = ambiq_gpio_pin_interrupt_configure,
	.manage_callback = ambiq_gpio_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = ambiq_gpio_port_get_direction,
#endif
};

#define AMBIQ_GPIO_DEFINE(n)                                                                       \
	static struct ambiq_gpio_data ambiq_gpio_data_##n;                                         \
	static void ambiq_gpio_cfg_func_##n(void);                                                 \
                                                                                                   \
	static const struct ambiq_gpio_config ambiq_gpio_config_##n = {                            \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.offset = DT_INST_REG_ADDR(n),                                                     \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.cfg_func = ambiq_gpio_cfg_func_##n};                                              \
	static void ambiq_gpio_cfg_func_##n(void)                                                  \
	{                                                                                          \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ambiq_gpio_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		return;                                                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ambiq_gpio_init, NULL, &ambiq_gpio_data_##n,                     \
			      &ambiq_gpio_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,     \
			      &ambiq_gpio_drv_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_GPIO_DEFINE)
