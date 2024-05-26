/*
 * Copyright (c) 2016-2017 Piotr Mienkowski
 * Copyright (c) 2021 ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Cypress PSoC-6 MCU family General Purpose Input Output (GPIO)
 * module HAL driver.
 */

#ifndef _CYPRESS_PSOC6_SOC_GPIO_H_
#define _CYPRESS_PSOC6_SOC_GPIO_H_

#include <zephyr/types.h>
#include <soc.h>

/*
 * Pin flags/attributes
 */

#define SOC_GPIO_DEFAULT                (0)

#define SOC_GPIO_FLAGS_POS              (0)
#define SOC_GPIO_FLAGS_MASK             (0x3F << SOC_GPIO_FLAGS_POS)
#define SOC_GPIO_PULLUP_POS             (0)
#define SOC_GPIO_PULLUP                 (1 << SOC_GPIO_PULLUP_POS)
#define SOC_GPIO_PULLDOWN_POS           (1)
#define SOC_GPIO_PULLDOWN               (1 << SOC_GPIO_PULLDOWN_POS)
#define SOC_GPIO_OPENDRAIN_POS          (2)
#define SOC_GPIO_OPENDRAIN              (1 << SOC_GPIO_OPENDRAIN_POS)
#define SOC_GPIO_OPENSOURCE_POS         (3)
#define SOC_GPIO_OPENSOURCE             (1 << SOC_GPIO_OPENSOURCE_POS)
/* Push-Pull means Strong, see dts/pinctrl/pincfg-node.yaml */
#define SOC_GPIO_PUSHPULL_POS           (4)
#define SOC_GPIO_PUSHPULL               (1 << SOC_GPIO_PUSHPULL_POS)
/* Input-Enable means Input-Buffer, see dts/pinctrl/pincfg-node.yaml */
#define SOC_GPIO_INPUTENABLE_POS        (5)
#define SOC_GPIO_INPUTENABLE            (1 << SOC_GPIO_INPUTENABLE_POS)

/* Bit field: SOC_GPIO_IN_FILTER */
#define SOC_GPIO_IN_FILTER_POS          (6)
#define SOC_GPIO_IN_FILTER_MASK         (3 << SOC_GPIO_IN_FILTER_POS)
#define SOC_GPIO_IN_FILTER_NONE         (0 << SOC_GPIO_IN_FILTER_POS)
#define SOC_GPIO_IN_FILTER_DEBOUNCE     (1 << SOC_GPIO_IN_FILTER_POS)
#define SOC_GPIO_IN_FILTER_DEGLITCH     (2 << SOC_GPIO_IN_FILTER_POS)

#define SOC_GPIO_INT_ENABLE             (1 << 8)

/* Bit field: SOC_GPIO_INT_TRIG */
#define SOC_GPIO_INT_TRIG_POS           (9)
#define SOC_GPIO_INT_TRIG_MASK          (3 << SOC_GPIO_INT_TRIG_POS)
/** Interrupt is triggered by a level detection event. */
#define SOC_GPIO_INT_TRIG_LEVEL         (0 << SOC_GPIO_INT_TRIG_POS)
/** Interrupt is triggered by an edge detection event. */
#define SOC_GPIO_INT_TRIG_EDGE          (1 << SOC_GPIO_INT_TRIG_POS)
/** Interrupt is triggered by any edge detection event. */
#define SOC_GPIO_INT_TRIG_DOUBLE_EDGE   (2 << SOC_GPIO_INT_TRIG_POS)

/** Interrupt is triggered by a high level / rising edge detection event */
#define SOC_GPIO_INT_ACTIVE_HIGH        (1 << 11)

/* Bit field: SOC_GPIO_FUNC */
#define SOC_GPIO_FUNC_POS               (16)
#define SOC_GPIO_FUNC_MASK              (0x1F << SOC_GPIO_FUNC_POS)

struct soc_gpio_pin {
	GPIO_PRT_Type *regs; /** pointer to registers of the GPIO controller */
	uint32_t pinum;      /** pin number */
	uint32_t flags;      /** pin flags/attributes */
};

/**
 * @brief Configure GPIO pin(s).
 *
 * Configure one or several pins belonging to the same GPIO port.
 * Example scenarios:
 * - configure pin(s) as input with debounce filter enabled.
 * - connect pin(s) to a HSIOM function and enable pull-up.
 * - configure pin(s) as open drain output.
 * All pins are configured in the same way.
 *
 * @param pin  pin's configuration data such as pin mask, pin attributes, etc.
 */
void soc_gpio_configure(const struct soc_gpio_pin *pin);

/**
 * @brief Configure a list of GPIO pin(s).
 *
 * Configure an arbitrary amount of pins in an arbitrary way. Each
 * configuration entry is a single item in an array passed as an
 * argument to the function.
 *
 * @param pins an array where each item contains pin's configuration data.
 * @param size size of the pin list.
 */
void soc_gpio_list_configure(const struct soc_gpio_pin pins[], size_t size);

#endif /* _CYPRESS_PSOC6_SOC_GPIO_H_ */
