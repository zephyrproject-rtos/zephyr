/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nuvoton-npcx-gpio.h>
#include <soc.h>

#include <zephyr/drivers/gpio/gpio_utils.h>
#include "soc_gpio.h"
#include "soc_miwu.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_npcx, CONFIG_GPIO_LOG_LEVEL);

/* GPIO module instances */
#define NPCX_GPIO_DEV(inst) DEVICE_DT_INST_GET(inst),
static const struct device *const gpio_devs[] = {
	DT_INST_FOREACH_STATUS_OKAY(NPCX_GPIO_DEV)
};

/* Driver config */
struct gpio_npcx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* GPIO controller base address */
	uintptr_t base;
	/* IO port */
	int port;
	/* Mapping table between gpio bits and wui */
	struct npcx_wui wui_maps[NPCX_GPIO_PORT_PIN_NUM];
	/* Mapping table between gpio bits and lvol */
	struct npcx_lvol lvol_maps[NPCX_GPIO_PORT_PIN_NUM];
};

/* Driver data */
struct gpio_npcx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev)                                                                          \
	((struct gpio_reg *)((const struct gpio_npcx_config *)(dev)->config)->base)

/* Platform specific GPIO functions */
const struct device *npcx_get_gpio_dev(int port)
{
	if (port >= ARRAY_SIZE(gpio_devs)) {
		return NULL;
	}

	return gpio_devs[port];
}

void npcx_gpio_enable_io_pads(const struct device *dev, int pin)
{
	const struct gpio_npcx_config *const config = dev->config;
	const struct npcx_wui *io_wui = &config->wui_maps[pin];

	if (io_wui->table == NPCX_MIWU_TABLE_NONE) {
		LOG_ERR("Cannot enable GPIO(%x, %d) pad", config->port, pin);
		return;
	}

	/*
	 * If this pin is configured as a GPIO interrupt source, do not
	 * implement bypass. Or ec cannot wake up via this event.
	 */
	if (pin < NPCX_GPIO_PORT_PIN_NUM && !npcx_miwu_irq_get_state(io_wui)) {
		npcx_miwu_io_enable(io_wui);
	}
}

void npcx_gpio_disable_io_pads(const struct device *dev, int pin)
{
	const struct gpio_npcx_config *const config = dev->config;
	const struct npcx_wui *io_wui = &config->wui_maps[pin];

	if (io_wui->table == NPCX_MIWU_TABLE_NONE) {
		LOG_ERR("Cannot disable GPIO(%x, %d) pad", config->port, pin);
		return;
	}

	/*
	 * If this pin is configured as a GPIO interrupt source, do not
	 * implement bypass. Or ec cannot wake up via this event.
	 */
	if (pin < NPCX_GPIO_PORT_PIN_NUM && !npcx_miwu_irq_get_state(io_wui)) {
		npcx_miwu_io_disable(io_wui);
	}
}

/* GPIO api functions */
static int gpio_npcx_config(const struct device *dev,
			     gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_npcx_config *const config = dev->config;
	const struct npcx_lvol *lvol = &config->lvol_maps[pin];
	struct gpio_reg *const inst = HAL_INSTANCE(dev);
	uint32_t mask = BIT(pin);

	/* Don't support simultaneous in/out mode */
	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0) &&
	    ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) {
		return -ENOTSUP;
	}

	/*
	 * Configure pin as input, if requested. Output is configured only
	 * after setting all other attributes, so as not to create a
	 * temporary incorrect logic state 0:input 1:output
	 */
	if ((flags & GPIO_OUTPUT) == 0) {
		inst->PDIR &= ~mask;
	}

	/* Does this IO pad support low-voltage input (1.8V) detection? */
	if (lvol->ctrl != NPCX_DT_LVOL_CTRL_NONE) {
		gpio_flags_t volt = flags & NPCX_GPIO_VOLTAGE_MASK;

		/*
		 * If this IO pad is configured for low-voltage input detection,
		 * the related drive type must select to open-drain also.
		 */
		if (volt == NPCX_GPIO_VOLTAGE_1P8) {
			flags |= GPIO_OPEN_DRAIN;
			npcx_lvol_set_detect_level(lvol->ctrl, lvol->bit, true);
		} else {
			npcx_lvol_set_detect_level(lvol->ctrl, lvol->bit, false);
		}
	}

	/* Select open drain 0:push-pull 1:open-drain */
	if ((flags & GPIO_OPEN_DRAIN) != 0) {
		inst->PTYPE |= mask;
	} else {
		inst->PTYPE &= ~mask;
	}

	/* Select pull-up/down of GPIO 0:pull-up 1:pull-down */
	if ((flags & GPIO_PULL_UP) != 0) {
		inst->PPUD  &= ~mask;
		inst->PPULL |= mask;
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		inst->PPUD  |= mask;
		inst->PPULL |= mask;
	} else {
		/* disable pull down/up */
		inst->PPULL &= ~mask;
	}

	/* Set level 0:low 1:high */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		inst->PDOUT |= mask;
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		inst->PDOUT &= ~mask;
	}

	/* Configure pin as output, if requested 0:input 1:output */
	if ((flags & GPIO_OUTPUT) != 0) {
		inst->PDIR |= mask;
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_npcx_pin_get_config(const struct device *port, gpio_pin_t pin,
				    gpio_flags_t *out_flags)
{
	const struct gpio_npcx_config *const config = port->config;
	const struct npcx_lvol *lvol = &config->lvol_maps[pin];
	struct gpio_reg *const inst = HAL_INSTANCE(port);
	uint32_t mask = BIT(pin);
	gpio_flags_t flags = 0;

	/* 0:input 1:output */
	if (inst->PDIR & mask) {
		flags |= GPIO_OUTPUT;

		/* 0:push-pull 1:open-drain */
		if (inst->PTYPE & mask) {
			flags |= GPIO_OPEN_DRAIN;
		}

		/* 0:low 1:high */
		if (inst->PDOUT & mask) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	} else {
		flags |= GPIO_INPUT;

		/* 0:disabled 1:enabled pull */
		if (inst->PPULL & mask) {
			/* 0:pull-up 1:pull-down */
			if (inst->PPUD & mask) {
				flags |= GPIO_PULL_DOWN;
			} else {
				flags |= GPIO_PULL_UP;
			}
		}
	}

	/* Enable low-voltage detection? */
	if (lvol->ctrl != NPCX_DT_LVOL_CTRL_NONE &&
		npcx_lvol_get_detect_level(lvol->ctrl, lvol->bit)) {
		flags |= NPCX_GPIO_VOLTAGE_1P8;
	};

	*out_flags = flags;

	return 0;
}
#endif

static int gpio_npcx_port_get_raw(const struct device *dev,
				  gpio_port_value_t *value)
{
	struct gpio_reg *const inst = HAL_INSTANCE(dev);

	/* Get raw bits of GPIO input registers */
	*value = inst->PDIN;

	return 0;
}

static int gpio_npcx_port_set_masked_raw(const struct device *dev,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	struct gpio_reg *const inst = HAL_INSTANCE(dev);
	uint8_t out = inst->PDOUT;

	inst->PDOUT = ((out & ~mask) | (value & mask));

	return 0;
}

static int gpio_npcx_port_set_bits_raw(const struct device *dev,
				       gpio_port_pins_t mask)
{
	struct gpio_reg *const inst = HAL_INSTANCE(dev);

	/* Set raw bits of GPIO output registers */
	inst->PDOUT |= mask;

	return 0;
}

static int gpio_npcx_port_clear_bits_raw(const struct device *dev,
					 gpio_port_pins_t mask)
{
	struct gpio_reg *const inst = HAL_INSTANCE(dev);

	/* Clear raw bits of GPIO output registers */
	inst->PDOUT &= ~mask;

	return 0;
}

static int gpio_npcx_port_toggle_bits(const struct device *dev,
				      gpio_port_pins_t mask)
{
	struct gpio_reg *const inst = HAL_INSTANCE(dev);

	/* Toggle raw bits of GPIO output registers */
	inst->PDOUT ^= mask;

	return 0;
}

static int gpio_npcx_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_npcx_config *const config = dev->config;

	if (config->wui_maps[pin].table == NPCX_MIWU_TABLE_NONE) {
		LOG_ERR("Cannot configure GPIO(%x, %d)", config->port, pin);
		return -EINVAL;
	}

	LOG_DBG("pin_int_conf (%d, %d) match (%d, %d, %d)!!!",
			config->port, pin, config->wui_maps[pin].table,
			config->wui_maps[pin].group,
			config->wui_maps[pin].bit);
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		npcx_miwu_irq_disable(&config->wui_maps[pin]);
		return 0;
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		npcx_miwu_irq_enable(&config->wui_maps[pin]);
		return 0;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	/* Disable irq of wake-up input io-pads before configuring them */
	npcx_miwu_irq_disable(&config->wui_maps[pin]);

	/* Configure and enable interrupt? */
	if (mode != GPIO_INT_MODE_DISABLED) {
		enum miwu_int_mode miwu_mode;
		enum miwu_int_trig miwu_trig;
		int ret = 0;

		/* Determine interrupt is level or edge mode? */
		if (mode == GPIO_INT_MODE_EDGE) {
			miwu_mode = NPCX_MIWU_MODE_EDGE;
		} else {
			miwu_mode = NPCX_MIWU_MODE_LEVEL;
		}

		/* Determine trigger mode is low, high or both? */
		if (trig == GPIO_INT_TRIG_LOW) {
			miwu_trig = NPCX_MIWU_TRIG_LOW;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			miwu_trig = NPCX_MIWU_TRIG_HIGH;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			miwu_trig = NPCX_MIWU_TRIG_BOTH;
		} else {
			LOG_ERR("Invalid interrupt trigger type %d", trig);
			return -EINVAL;
		}

		/* Call MIWU routine to setup interrupt configuration */
		ret = npcx_miwu_interrupt_configure(&config->wui_maps[pin],
						miwu_mode, miwu_trig);
		if (ret != 0) {
			LOG_ERR("Configure MIWU interrupt failed");
			return ret;
		}

		/* Enable it after configuration is completed */
		npcx_miwu_irq_enable(&config->wui_maps[pin]);
	}

	return 0;
}

static int gpio_npcx_manage_callback(const struct device *dev,
				      struct gpio_callback *callback, bool set)
{
	const struct gpio_npcx_config *const config = dev->config;
	struct miwu_callback *miwu_cb = (struct miwu_callback *)callback;
	int pin = find_lsb_set(callback->pin_mask) - 1;

	/* pin_mask should not be zero */
	if (pin < 0) {
		return -EINVAL;
	}

	/* Has the IO pin valid MIWU input source? */
	if (config->wui_maps[pin].table == NPCX_MIWU_TABLE_NONE) {
		LOG_ERR("Cannot manage GPIO(%x, %d) callback!", config->port,
				pin);
		return -EINVAL;
	}

	/* Initialize WUI information in unused bits field */
	npcx_miwu_init_gpio_callback(miwu_cb, &config->wui_maps[pin],
			config->port);

	/* Insert or remove a IO callback which being called in MIWU ISRs */
	return npcx_miwu_manage_callback(miwu_cb, set);
}

/* GPIO driver registration */
static DEVICE_API(gpio, gpio_npcx_driver) = {
	.pin_configure = gpio_npcx_config,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_npcx_pin_get_config,
#endif
	.port_get_raw = gpio_npcx_port_get_raw,
	.port_set_masked_raw = gpio_npcx_port_set_masked_raw,
	.port_set_bits_raw = gpio_npcx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_npcx_port_clear_bits_raw,
	.port_toggle_bits = gpio_npcx_port_toggle_bits,
	.pin_interrupt_configure = gpio_npcx_pin_interrupt_configure,
	.manage_callback = gpio_npcx_manage_callback,
};

int gpio_npcx_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define NPCX_GPIO_DEVICE_INIT(inst)                                            \
	static const struct gpio_npcx_config gpio_npcx_cfg_##inst = {          \
		.common = {						       \
			.port_pin_mask =                                       \
			GPIO_PORT_PIN_MASK_FROM_NGPIOS(NPCX_GPIO_PORT_PIN_NUM),\
		},                                                             \
		.base = DT_INST_REG_ADDR(inst),                                \
		.port = inst,                                                  \
		.wui_maps = NPCX_DT_WUI_ITEMS_LIST(inst),                      \
		.lvol_maps = NPCX_DT_LVOL_ITEMS_LIST(inst),                    \
	};                                                                     \
	BUILD_ASSERT(NPCX_DT_WUI_ITEMS_LEN(inst) == NPCX_GPIO_PORT_PIN_NUM,    \
			"size of prop. wui-maps must equal to pin number!");   \
	BUILD_ASSERT(NPCX_DT_LVOL_ITEMS_LEN(inst) == NPCX_GPIO_PORT_PIN_NUM,   \
			"size of prop. lvol-maps must equal to pin number!");  \
									       \
	static struct gpio_npcx_data gpio_npcx_data_##inst;	               \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			    gpio_npcx_init,                                    \
			    NULL,					       \
			    &gpio_npcx_data_##inst,                            \
			    &gpio_npcx_cfg_##inst,                             \
			    PRE_KERNEL_1,                                       \
			    CONFIG_GPIO_INIT_PRIORITY,                         \
			    &gpio_npcx_driver);

DT_INST_FOREACH_STATUS_OKAY(NPCX_GPIO_DEVICE_INIT)
