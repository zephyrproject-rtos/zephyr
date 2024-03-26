/*
 * Copyright (c) 2024 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/wakeup_source.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/types.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <stm32_ll_system.h>
#include <stm32_ll_pwr.h>

#include <zephyr/dt-bindings/power/stm32_pwr.h>

#include "wkup_pins_ctrl.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_pwr

#define STM32_PWR_NODE DT_NODELABEL(pwr)

#define PWR_STM32_MAX_NB_WKUP_PINS DT_INST_PROP(0, wkup_pins_nb)

#define PWR_STM32_WKUP_PIN_SRCS_NB DT_INST_PROP_OR(0, wkup_pin_srcs, 1)

#define PWR_STM32_WKUP_PINS_POLARITY DT_INST_PROP_OR(0, wkup_pins_pol, 0)

#define PWR_STM32_WKUP_PINS_PULL_CFG DT_INST_PROP_OR(0, wkup_pins_pull, 0)

/**
 * @name flags for wake-up pin polarity configuration
 * @{
 */

/* detection of wake-up event on the high level : rising edge */
#define STM32_PWR_WKUP_PIN_P_RISING	0
/* detection of wake-up event on the low level : falling edge */
#define STM32_PWR_WKUP_PIN_P_FALLING	1

/** @} */

/**
 * @name flags for wake-up pin pull configuration
 * @{
 */

#define STM32_PWR_WKUP_PIN_NOPULL	0
#define STM32_PWR_WKUP_PIN_PULLUP	1
#define STM32_PWR_WKUP_PIN_PULLDOWN	(1 << 2)

/** @} */

/**
 * @brief Structure for storing the DT configuration of a wake-up pin.
 */
struct wkup_pin_dt_cfg_t {
	uint32_t wkup_pin_id;
	struct gpio_dt_spec
		gpios_cfgs[PWR_STM32_WKUP_PIN_SRCS_NB]; /* GPIO(s) associated with wake-up pin */
};

#define WKUP_PIN_NODE_LABEL(i) wkup_pin_##i

#define WKUP_PIN_NODE_ID_BY_IDX(idx) DT_CHILD(STM32_PWR_NODE, WKUP_PIN_NODE_LABEL(idx))

static const struct gpio_dt_spec empty_gpio = {.port = NULL, .pin = 0, .dt_flags = 0};

/* cell_idx starts from 0 */
#define WKUP_PIN_GPIOS_CFG_DT_BY_IDX(cell_idx, node_id)                                            \
	GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, wkup_gpios, cell_idx, empty_gpio)

#define NB_DEF(i, _) i

/**
 * @brief Get wake-up pin configuration from a given devicetree node.
 *
 * This returns a static initializer for a <tt>struct wkup_pin_dt_cfg_t</tt>
 * filled with data from a given devicetree node.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a wkup_pin_dt_cfg_t structure.
 */
#define WKUP_PIN_CFG_DT(node_id)                                                                   \
	{                                                                                          \
		.wkup_pin_id = DT_REG_ADDR(node_id),                                               \
		.gpios_cfgs =                                                                      \
		{                                                                                  \
		FOR_EACH_FIXED_ARG(WKUP_PIN_GPIOS_CFG_DT_BY_IDX, (,), node_id,                     \
			LISTIFY(PWR_STM32_WKUP_PIN_SRCS_NB, NB_DEF, (,)))                          \
		}                                                                                  \
	}

/* wkup_pin idx starts from 1 */
#define WKUP_PIN_CFG_DT_BY_IDX(idx) WKUP_PIN_CFG_DT(WKUP_PIN_NODE_ID_BY_IDX(idx))

#define PWR_STM32_WKUP_PIN_DEF(i, _) LL_PWR_WAKEUP_PIN##i

/**
 * @brief Structure for passing the configuration of a wake-up pin.
 */
struct wkup_pin_cfg_t {
	uint32_t wkup_pin_id;
#if PWR_STM32_WKUP_PINS_POLARITY
	bool polarity; /* True if detection on the low level : falling edge */
#endif                 /* PWR_STM32_WKUP_PINS_POLARITY */
#if PWR_STM32_WKUP_PINS_PULL_CFG
	uint32_t pull_cfg;      /* wake-up pin pull config */
	uint32_t gpio_port;    /* wake-up pin GPIO port */
	gpio_pin_t gpio_pin;   /* wake-up pin GPIO pin number */
#endif                         /* PWR_STM32_WKUP_PINS_PULL_CFG */
	uint32_t src_selection; /* The source signal to use with this wkup pin */
};

/*
 * LookUp Table to store the LL_PWR_WAKEUP_PINx for each pin.
 */
static const uint32_t table_wakeup_pins[PWR_STM32_MAX_NB_WKUP_PINS + 1] = {
	LISTIFY(PWR_STM32_MAX_NB_WKUP_PINS, PWR_STM32_WKUP_PIN_DEF, (,))};

#define WKUP_PIN_CFG_DT_COMMA(wkup_pin_id) WKUP_PIN_CFG_DT(wkup_pin_id),

static struct wkup_pin_dt_cfg_t wkup_pins_cfgs[] = {
	DT_FOREACH_CHILD(STM32_PWR_NODE, WKUP_PIN_CFG_DT_COMMA)};

#if PWR_STM32_WKUP_PINS_PULL_CFG

#define STM32_GPIOS_NB               16

#if defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
#define PWR_STM32_GPIO_BIT_DEF(i, _) LL_PWR_GPIO_PIN_##i
#else
#define PWR_STM32_GPIO_BIT_DEF(i, _) LL_PWR_GPIO_BIT_##i
#endif /* CONFIG_SOC_SERIES_STM32U5X or CONFIG_SOC_SERIES_STM32WBAX */

/*
 * LookUp Table to store the LL_PWR_GPIO_BIT_x for each pin.
 * In case of U5 & WBA series, it's LL_PWR_GPIO_PIN_x
 */
static const uint32_t table_pwr_gpio_bits[STM32_GPIOS_NB] = {
	LISTIFY(STM32_GPIOS_NB, PWR_STM32_GPIO_BIT_DEF, (,))};

/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 */
static const struct device *const gpio_ports[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioa)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiob)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioc)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiod)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioe)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiof)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiog)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioh)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioi)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioj)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiok)),
};

/** Number of GPIO ports. */
static const size_t gpio_ports_cnt = ARRAY_SIZE(gpio_ports);

static const uint32_t table_pwr_gpio_ports[] = {
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	(uint32_t)LL_PWR_GPIO_PORTA, (uint32_t)LL_PWR_GPIO_PORTB, (uint32_t)LL_PWR_GPIO_PORTC,
	(uint32_t)LL_PWR_GPIO_PORTD, (uint32_t)LL_PWR_GPIO_PORTE, (uint32_t)LL_PWR_GPIO_PORTF,
	(uint32_t)LL_PWR_GPIO_PORTG, (uint32_t)LL_PWR_GPIO_PORTH, (uint32_t)LL_PWR_GPIO_PORTI,
	(uint32_t)LL_PWR_GPIO_PORTJ
#else
	LL_PWR_GPIO_A, LL_PWR_GPIO_B, LL_PWR_GPIO_C, LL_PWR_GPIO_D, LL_PWR_GPIO_E,
	LL_PWR_GPIO_F, LL_PWR_GPIO_G, LL_PWR_GPIO_H, LL_PWR_GPIO_I, LL_PWR_GPIO_J
#endif /* CONFIG_SOC_SERIES_STM32U5X */
};

static const size_t pwr_gpio_ports_cnt = ARRAY_SIZE(table_pwr_gpio_ports);

#endif /* PWR_STM32_WKUP_PINS_PULL_CFG */

/**
 * @brief Configure a wake-up pin.
 *
 * It is required to call this function and configure each wake-up pin before
 * it is selected for a wake-up request.
 *
 * @param wakeup_pin_cfg  wake-up pin configuration.
 *
 * @retval 0       On success.
 * @retval -EINVAL If a parameter with an invalid value has been provided.
 */
static int wkup_pin_setup(const struct wkup_pin_cfg_t *wakeup_pin_cfg)
{
	uint32_t wkup_pin_index = wakeup_pin_cfg->wkup_pin_id;

#if PWR_STM32_WKUP_PINS_POLARITY
	/* Set wake-up pin polarity */
	if (wakeup_pin_cfg->polarity == STM32_PWR_WKUP_PIN_P_FALLING) {
		LL_PWR_SetWakeUpPinPolarityLow(table_wakeup_pins[wkup_pin_index]);
	} else {
		LL_PWR_SetWakeUpPinPolarityHigh(table_wakeup_pins[wkup_pin_index]);
	}
#endif /* PWR_STM32_WKUP_PINS_POLARITY */

#if PWR_STM32_WKUP_PINS_PULL_CFG
	if (wakeup_pin_cfg->pull_cfg == STM32_PWR_WKUP_PIN_NOPULL) {
		LL_PWR_DisableGPIOPullUp(wakeup_pin_cfg->gpio_port,
					table_pwr_gpio_bits[wakeup_pin_cfg->gpio_pin]);
		LL_PWR_DisableGPIOPullDown(wakeup_pin_cfg->gpio_port,
					table_pwr_gpio_bits[wakeup_pin_cfg->gpio_pin]);

	} else if ((wakeup_pin_cfg->pull_cfg == STM32_PWR_WKUP_PIN_PULLUP) &&
						wakeup_pin_cfg->gpio_port) {
		LL_PWR_EnableGPIOPullUp(wakeup_pin_cfg->gpio_port,
					table_pwr_gpio_bits[wakeup_pin_cfg->gpio_pin]);

	} else if ((wakeup_pin_cfg->pull_cfg == STM32_PWR_WKUP_PIN_PULLDOWN) &&
						wakeup_pin_cfg->gpio_port) {
		LL_PWR_EnableGPIOPullDown(wakeup_pin_cfg->gpio_port,
					  table_pwr_gpio_bits[wakeup_pin_cfg->gpio_pin]);
	}
#endif /* PWR_STM32_WKUP_PINS_PULL_CFG */

#if defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* Select the proper wake-up signal source */
	if (wakeup_pin_cfg->src_selection & STM32_PWR_WKUP_PIN_SRC_0) {
		LL_PWR_SetWakeUpPinSignal0Selection(table_wakeup_pins[wkup_pin_index]);
	} else if (wakeup_pin_cfg->src_selection & STM32_PWR_WKUP_PIN_SRC_1) {
		LL_PWR_SetWakeUpPinSignal1Selection(table_wakeup_pins[wkup_pin_index]);
	} else if (wakeup_pin_cfg->src_selection & STM32_PWR_WKUP_PIN_SRC_2) {
		LL_PWR_SetWakeUpPinSignal2Selection(table_wakeup_pins[wkup_pin_index]);
	} else {
		LL_PWR_SetWakeUpPinSignal3Selection(table_wakeup_pins[wkup_pin_index]);
	}
#endif /* CONFIG_SOC_SERIES_STM32U5X or CONFIG_SOC_SERIES_STM32WBAX */

	LL_PWR_EnableWakeUpPin(table_wakeup_pins[wkup_pin_index]);

	return 0;
}

void z_sys_wakeup_source_enable_gpio(const struct gpio_dt_spec *gpio)
{
	struct wkup_pin_cfg_t wakeup_pin_cfg;
	struct wkup_pin_dt_cfg_t *wkup_pin_dt_cfg;
	struct gpio_dt_spec *wkup_pin_gpio_cfg;
	bool found_gpio = false;
	int i;
	int j;

	UNUSED(empty_gpio);

	for (i = 0; i < PWR_STM32_MAX_NB_WKUP_PINS; i++) {
		wkup_pin_dt_cfg = &(wkup_pins_cfgs[i]);
		for (j = 0; j < PWR_STM32_WKUP_PIN_SRCS_NB; j++) {
			wkup_pin_gpio_cfg = &(wkup_pin_dt_cfg->gpios_cfgs[j]);
			if (wkup_pin_gpio_cfg->port == gpio->port) {
				if (wkup_pin_gpio_cfg->pin == gpio->pin) {
					found_gpio = true;
					break;
				}
			}
		}
		if (found_gpio) {
			break;
		};
	}

	if (found_gpio) {
		wakeup_pin_cfg.wkup_pin_id = wkup_pin_dt_cfg->wkup_pin_id;

#if defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
		wakeup_pin_cfg.src_selection = wkup_pin_gpio_cfg->dt_flags &
						(STM32_PWR_WKUP_PIN_SRC_0 |
						STM32_PWR_WKUP_PIN_SRC_1 |
						STM32_PWR_WKUP_PIN_SRC_2);
#else
		wakeup_pin_cfg.src_selection = 0;
#endif /* CONFIG_SOC_SERIES_STM32U5X or CONFIG_SOC_SERIES_STM32WBAX */

#if PWR_STM32_WKUP_PINS_POLARITY
		/*
		 * get polarity config from GPIO flags specified in "gpios" property
		 */
		if (gpio->dt_flags & GPIO_ACTIVE_LOW) {
			wakeup_pin_cfg.polarity = STM32_PWR_WKUP_PIN_P_FALLING;
		} else {
			wakeup_pin_cfg.polarity = STM32_PWR_WKUP_PIN_P_RISING;
		}
#endif /* PWR_STM32_WKUP_PINS_POLARITY */

#if PWR_STM32_WKUP_PINS_PULL_CFG
		wakeup_pin_cfg.gpio_port = 0;
		for (i = 0; i < gpio_ports_cnt; i++) {
			if ((gpio_ports[i] == gpio->port) && (i < pwr_gpio_ports_cnt)) {
				wakeup_pin_cfg.gpio_port = table_pwr_gpio_ports[i];
				break;
			}
		}

		wakeup_pin_cfg.gpio_pin = gpio->pin;

		/*
		 * get pull config from GPIO flags specified in "gpios" property
		 */
		if (gpio->dt_flags & GPIO_PULL_UP) {
			wakeup_pin_cfg.pull_cfg = STM32_PWR_WKUP_PIN_PULLUP;
		} else if (gpio->dt_flags & GPIO_PULL_DOWN) {
			wakeup_pin_cfg.pull_cfg = STM32_PWR_WKUP_PIN_PULLDOWN;
		} else {
			wakeup_pin_cfg.pull_cfg = STM32_PWR_WKUP_PIN_NOPULL;
		}
#endif /* PWR_STM32_WKUP_PINS_PULL_CFG */

		wkup_pin_setup(&wakeup_pin_cfg);
	} else {
		LOG_DBG("Couldn't find a wake-up source correspending to GPIO %s pin %d\n",
			gpio->port->name, gpio->pin);
		LOG_DBG("=> It cannot be used as a wake-up source\n");
	}
}
