/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_AM26X_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_AM26X_PINCTRL_H_

/* Pin mode - it is at 0th bit. No shift required */
#define PIN_MODE(mode) ((mode) & 0xf)

/*  Override IP default to enable input */
#define PIN_FORCE_INPUT_ENABLE   ((0x1U) << 4U)
/*  Override IP default to disable input */
#define PIN_FORCE_INPUT_DISABLE  ((0x3U) << 4U)
/*  Override IP default to enable output */
#define PIN_FORCE_OUTPUT_ENABLE  ((0x1U) << 6U)
/*  Override IP default to disable output */
#define PIN_FORCE_OUTPUT_DISABLE ((0x3U) << 6U)

/*  Resistor enable */
#define PIN_PULL_DISABLE ((0x1U) << 8U)
/*  Resistor disable */
#define PIN_PULL_ENABLE  ((0x0U) << 8U)
/*  Pull Up */
#define PIN_PULL_UP      ((0x1U) << 9U)
/*  Pull Down */
#define PIN_PULL_DOWN    ((0x0U) << 9U)

/*  Slew Rate High */
#define PIN_SLEW_RATE_HIGH ((0x0U) << 10U)
/*  Slew Rate Low */
#define PIN_SLEW_RATE_LOW  ((0x1U) << 10U)

/*  GPIO Pin CPU ownership - R5SS0_0 */
#define PIN_GPIO_R5SS0_0 ((0x0U) << 16U)
/*  GPIO Pin CPU ownership - R5SS0_1 */
#define PIN_GPIO_R5SS0_1 ((0x1U) << 16U)

/*  Pin Qualifier - SYNC */
#define PIN_QUAL_SYNC    ((0x0U) << 18U)
/*  Pin Qualifier - 3 SAMPLE */
#define PIN_QUAL_3SAMPLE ((0x1U) << 18U)
/*  Pin Qualifier - 6 SAMPLE */
#define PIN_QUAL_6SAMPLE ((0x2U) << 18U)
/*  Pin Qualifier - ASYNC */
#define PIN_QUAL_ASYNC   ((0x3U) << 18U)

/*  Pin Invert */
#define PIN_INVERT     ((0x1U) << 20U)
/*  Pin Non Invert */
#define PIN_NON_INVERT ((0x0U) << 20U)

#define PIN_GPIO(num) ((num) * 4)

#define TI_AM26X_PINMUX(gpio_num, value) (gpio_num)(value)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_AM26X_PINCTRL_H_ */
