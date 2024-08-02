/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/types.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <stm32_ll_system.h>
#include <stm32_ll_pwr.h>

#include <zephyr/dt-bindings/power/stm32_pwr.h>

#include "stm32_wkup_pins.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_pwr

#define STM32_PWR_NODE DT_NODELABEL(pwr)

#define PWR_STM32_MAX_NB_WKUP_PINS DT_INST_PROP(0, wkup_pins_nb)

#define PWR_STM32_WKUP_PIN_SRCS_NB DT_INST_PROP_OR(0, wkup_pin_srcs, 1)

#define PWR_STM32_WKUP_PINS_POLARITY DT_INST_PROP_OR(0, wkup_pins_pol, 0)

#define PWR_STM32_WKUP_PINS_PUPD_CFG DT_INST_PROP_OR(0, wkup_pins_pupd, 0)

/** @cond INTERNAL_HIDDEN */

/**
 * @brief flags for wake-up pin polarity configuration
 * @{
 */

/* detection of wake-up event on the high level : rising edge */
#define STM32_PWR_WKUP_PIN_P_RISING	0
/* detection of wake-up event on the low level : falling edge */
#define STM32_PWR_WKUP_PIN_P_FALLING	1

/** @} */

/**
 * @brief flags for configuration of pull-ups & pull-downs of GPIO ports
 * that are associated with wake-up pins
 * @{
 */

#define STM32_PWR_WKUP_PIN_NOPULL	0
#define STM32_PWR_WKUP_PIN_PULLUP	1
#define STM32_PWR_WKUP_PIN_PULLDOWN	(1 << 2)

/** @} */

/**
 * @brief Structure for storing the devicetree configuration of a wake-up pin.
 */
struct wkup_pin_dt_cfg_t {
	/* starts from 1 */
	uint32_t wkup_pin_id;
	/* GPIO pin(s) associated with wake-up pin */
	struct gpio_dt_spec gpios_cfgs[PWR_STM32_WKUP_PIN_SRCS_NB];
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

#define WKUP_PIN_CFG_DT_COMMA(wkup_pin_id) WKUP_PIN_CFG_DT(wkup_pin_id),

/** @endcond */

/**
 * @brief Structure for passing the runtime configuration of a given wake-up pin.
 */
struct wkup_pin_cfg_t {
	uint32_t wkup_pin_id;
#if PWR_STM32_WKUP_PINS_POLARITY
	bool polarity; /* True if detection on the low level : falling edge */
#endif                 /* PWR_STM32_WKUP_PINS_POLARITY */
#if PWR_STM32_WKUP_PINS_PUPD_CFG
	uint32_t pupd_cfg;	/* pull-up/down config of GPIO port associated w/ this wkup pin */
	uint32_t ll_gpio_port;	/* GPIO port associated with this wake-up pin */
	uint32_t ll_gpio_pin;	/* GPIO pin associated with this wake-up pin */
#endif /* PWR_STM32_WKUP_PINS_PUPD_CFG */
	uint32_t src_selection; /* The source signal to use with this wake-up pin */
};

/**
 * @brief LookUp Table to store LL_PWR_WAKEUP_PINx for each wake-up pin.
 */
static const uint32_t table_wakeup_pins[PWR_STM32_MAX_NB_WKUP_PINS + 1] = {
	LISTIFY(PWR_STM32_MAX_NB_WKUP_PINS, PWR_STM32_WKUP_PIN_DEF, (,))};

static struct wkup_pin_dt_cfg_t wkup_pins_cfgs[] = {
	DT_FOREACH_CHILD(STM32_PWR_NODE, WKUP_PIN_CFG_DT_COMMA)};

#if PWR_STM32_WKUP_PINS_PUPD_CFG

/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entry will be NULL if the GPIO port is not enabled.
 * Also used to get GPIO port index from device ptr, so having
 * NULL entries for unused GPIO ports is desired.
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

/* Number of GPIO ports. */
static const size_t gpio_ports_cnt = ARRAY_SIZE(gpio_ports);

/**
 * @brief LookUp Table to store LL_PWR_GPIO_x of each GPIO port.
 */
static const uint32_t table_ll_pwr_gpio_ports[] = {
#ifdef CONFIG_SOC_SERIES_STM32U5X
	(uint32_t)LL_PWR_GPIO_PORTA, (uint32_t)LL_PWR_GPIO_PORTB, (uint32_t)LL_PWR_GPIO_PORTC,
	(uint32_t)LL_PWR_GPIO_PORTD, (uint32_t)LL_PWR_GPIO_PORTE, (uint32_t)LL_PWR_GPIO_PORTF,
	(uint32_t)LL_PWR_GPIO_PORTG, (uint32_t)LL_PWR_GPIO_PORTH, (uint32_t)LL_PWR_GPIO_PORTI,
	(uint32_t)LL_PWR_GPIO_PORTJ
#else
	LL_PWR_GPIO_A, LL_PWR_GPIO_B, LL_PWR_GPIO_C, LL_PWR_GPIO_D, LL_PWR_GPIO_E,
	LL_PWR_GPIO_F, LL_PWR_GPIO_G, LL_PWR_GPIO_H, LL_PWR_GPIO_I, LL_PWR_GPIO_J
#endif /* CONFIG_SOC_SERIES_STM32U5X */
};

static const size_t pwr_ll_gpio_ports_cnt = ARRAY_SIZE(table_ll_pwr_gpio_ports);

#endif /* PWR_STM32_WKUP_PINS_PUPD_CFG */

/**
 * @brief Configure & enable a wake-up pin.
 *
 * @param wakeup_pin_cfg wake-up pin runtime configuration.
 *
 */
static void wkup_pin_setup(const struct wkup_pin_cfg_t *wakeup_pin_cfg)
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

#if PWR_STM32_WKUP_PINS_PUPD_CFG
	if (wakeup_pin_cfg->pupd_cfg == STM32_PWR_WKUP_PIN_NOPULL) {
		LL_PWR_DisableGPIOPullUp(wakeup_pin_cfg->ll_gpio_port,
					(1u << wakeup_pin_cfg->ll_gpio_pin));
		LL_PWR_DisableGPIOPullDown(wakeup_pin_cfg->ll_gpio_port,
					(1u << wakeup_pin_cfg->ll_gpio_pin));

	} else if ((wakeup_pin_cfg->pupd_cfg == STM32_PWR_WKUP_PIN_PULLUP) &&
						wakeup_pin_cfg->ll_gpio_port) {
		LL_PWR_EnableGPIOPullUp(wakeup_pin_cfg->ll_gpio_port,
					(1u << wakeup_pin_cfg->ll_gpio_pin));

	} else if ((wakeup_pin_cfg->pupd_cfg == STM32_PWR_WKUP_PIN_PULLDOWN) &&
						wakeup_pin_cfg->ll_gpio_port) {
		LL_PWR_EnableGPIOPullDown(wakeup_pin_cfg->ll_gpio_port,
					  (1u << wakeup_pin_cfg->ll_gpio_pin));
	}
#endif /* PWR_STM32_WKUP_PINS_PUPD_CFG */

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
}

/**
 * @brief Exported function to configure a given GPIO pin as a source for wake-up pins
 *
 * @param gpio Container for GPIO pin information specified in devicetree
 *
 * @return 0 on success, -EINVAL if said GPIO pin is not associated with any wake-up pin.
 */
int stm32_pwr_wkup_pin_cfg_gpio(const struct gpio_dt_spec *gpio)
{
	struct wkup_pin_cfg_t wakeup_pin_cfg;
	struct wkup_pin_dt_cfg_t *wkup_pin_dt_cfg;
	struct gpio_dt_spec *wkup_pin_gpio_cfg;
	bool found_gpio = false;
	int i;
	int j;

	UNUSED(empty_gpio);

	/* Look for a wake-up pin that has this GPIO pin as a source, if any */
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

	if (!found_gpio) {
		LOG_DBG("Couldn't find a wake-up event correspending to GPIO %s pin %d\n",
			gpio->port->name, gpio->pin);
		LOG_DBG("=> It cannot be used as a wake-up source\n");
		return -EINVAL;
	}

	wakeup_pin_cfg.wkup_pin_id = wkup_pin_dt_cfg->wkup_pin_id;

/* Each wake-up pin on STM32U5 is associated with 4 wkup srcs, 3 of them correspond to GPIOs. */
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
	 * get polarity config from GPIO flags specified in device "gpios" property
	 */
	if (gpio->dt_flags & GPIO_ACTIVE_LOW) {
		wakeup_pin_cfg.polarity = STM32_PWR_WKUP_PIN_P_FALLING;
	} else {
		wakeup_pin_cfg.polarity = STM32_PWR_WKUP_PIN_P_RISING;
	}
#endif /* PWR_STM32_WKUP_PINS_POLARITY */

#if PWR_STM32_WKUP_PINS_PUPD_CFG
	wakeup_pin_cfg.ll_gpio_port = 0;
	for (i = 0; i < gpio_ports_cnt; i++) {
		if ((gpio_ports[i] == gpio->port) && (i < pwr_ll_gpio_ports_cnt)) {
			wakeup_pin_cfg.ll_gpio_port = table_ll_pwr_gpio_ports[i];
			break;
		}
	}

	wakeup_pin_cfg.ll_gpio_pin = gpio->pin;

	/*
	 * Get pull-up/down config, if any, from GPIO flags specified in device "gpios" property
	 */
	if (gpio->dt_flags & GPIO_PULL_UP) {
		wakeup_pin_cfg.pupd_cfg = STM32_PWR_WKUP_PIN_PULLUP;
	} else if (gpio->dt_flags & GPIO_PULL_DOWN) {
		wakeup_pin_cfg.pupd_cfg = STM32_PWR_WKUP_PIN_PULLDOWN;
	} else {
		wakeup_pin_cfg.pupd_cfg = STM32_PWR_WKUP_PIN_NOPULL;
	}
#endif /* PWR_STM32_WKUP_PINS_PUPD_CFG */

	wkup_pin_setup(&wakeup_pin_cfg);

	return 0;
}

/**
 * @brief Exported function to activate pull-ups/pull-downs configuration of GPIO ports
 * associated with wake-up pins.
 */
void stm32_pwr_wkup_pin_cfg_pupd(void)
{
#if PWR_STM32_WKUP_PINS_PUPD_CFG
#ifdef CONFIG_SOC_SERIES_STM32U5X
	LL_PWR_EnablePUPDConfig();
#else
	LL_PWR_EnablePUPDCfg();
#endif /* CONFIG_SOC_SERIES_STM32U5X */
#else
	return;
#endif /* PWR_STM32_WKUP_PINS_PUPD_CFG */
}
