/*
 * Copyright (c) 2026 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * EM32F967 SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_ST_EM32_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_ST_EM32_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

/* Use EM32F967 pinctrl definitions */
#include <zephyr/dt-bindings/pinctrl/em32f967-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Type for EM32 pin. */
typedef struct pinctrl_soc_pin {
	/** Pinmux settings (port, pin and function). */
	uint32_t pinmux;
	/** Pin configuration (bias, drive and slew rate). */
	uint32_t pincfg;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize pinmux field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_EM32_PINMUX_INIT(node_id) DT_PROP(node_id, pinmux)

/**
 * @brief Definitions used to initialize fields in #pinctrl_pin_t
 *
 * These definitions are compatible with the EM32F967 pinctrl system
 */
#define EM32_NO_PULL     0x0
#define EM32_PULL_UP     0x1
#define EM32_PULL_DOWN   0x2
#define EM32_PUSH_PULL   0x0
#define EM32_OPEN_DRAIN  0x1
#define EM32_OUTPUT_LOW  0x0
#define EM32_OUTPUT_HIGH 0x1
#define EM32_GPIO_OUTPUT 0x1

/* Bit field definitions for EM32F967 */
#define EM32_MODER_SHIFT   4
#define EM32_OTYPER_SHIFT  6
#define EM32_OSPEEDR_SHIFT 7
#define EM32_PUPDR_SHIFT   9
#define EM32_ODR_SHIFT     11
#define EM32_DRIVE_SHIFT   13

/* EM32F967 IP_Share register definitions (0x4003023C) */
#define EM32_IP_SHARE_REG 0x4003023C

/* IP_Share bit field definitions based on EM32F967 specification */
#define EM32_IP_SHARE_SPI1_SHIFT   0  /* [1:0] SPI1 pin assignment */
#define EM32_IP_SHARE_SSP2_SHIFT   2  /* [3:2] SSP2 pin assignment */
#define EM32_IP_SHARE_ISO7816_1    4  /* [4] ISO7816-1 pin assignment */
#define EM32_IP_SHARE_ISO7816_2    5  /* [5] ISO7816-2 pin assignment */
#define EM32_IP_SHARE_UART1        6  /* [6] UART1 pin assignment */
#define EM32_IP_SHARE_UART2        7  /* [7] UART2 pin assignment */
#define EM32_IP_SHARE_I2C1         8  /* [8] I2C1 pin assignment */
#define EM32_IP_SHARE_I2C2         9  /* [9] I2C2 pin assignment */
#define EM32_IP_SHARE_USART        10 /* [10] USART pin assignment */
#define EM32_IP_SHARE_DMA_CS_SHIFT 11 /* [12:11] DMA_CS pin assignment */
#define EM32_IP_SHARE_PWM_SW1      15 /* [15] PWM Switch 1: 0=PWMD, 1=PWMA1 (PA1/PB11 N output) */
#define EM32_IP_SHARE_PWM_SW2      16 /* [16] PWM Switch 2: 0=PWME, 1=PWMB1 (PA3/PB13 N output) */
#define EM32_IP_SHARE_PWM_SW3      17 /* [17] PWM Switch 3: 0=PWMF, 1=PWMC1 (PA5/PB15 N output) */
#define EM32_IP_SHARE_PWM          18 /* [18] PWM_S pin select: 0=PA0-5, 1=PB10-15 */

/**
 * @brief Utility macro to initialize pincfg field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */

#define Z_PINCTRL_EM32_PINCFG_INIT(node_id)                                                        \
	(((EM32_NO_PULL * DT_PROP(node_id, bias_disable)) << EM32_PUPDR_SHIFT) |                   \
	 ((EM32_PULL_UP * DT_PROP(node_id, bias_pull_up)) << EM32_PUPDR_SHIFT) |                   \
	 ((EM32_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) << EM32_PUPDR_SHIFT) |               \
	 ((EM32_PUSH_PULL * DT_PROP(node_id, drive_push_pull)) << EM32_OTYPER_SHIFT) |             \
	 ((EM32_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) << EM32_OTYPER_SHIFT) |           \
	 ((EM32_OUTPUT_LOW * DT_PROP(node_id, output_low)) << EM32_ODR_SHIFT) |                    \
	 ((EM32_OUTPUT_HIGH * DT_PROP(node_id, output_high)) << EM32_ODR_SHIFT) |                  \
	 ((EM32_GPIO_OUTPUT * DT_PROP(node_id, output_low)) << EM32_MODER_SHIFT) |                 \
	 ((EM32_GPIO_OUTPUT * DT_PROP(node_id, output_high)) << EM32_MODER_SHIFT) |                \
	 (DT_ENUM_IDX(node_id, slew_rate) << EM32_OSPEEDR_SHIFT) |                                 \
	 ((DT_PROP_OR(node_id, drive_strength, 0) > 0 ? 1 : 0) << EM32_DRIVE_SHIFT))

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)                                         \
	{.pinmux = Z_PINCTRL_EM32_PINMUX_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx)),           \
	 .pincfg = Z_PINCTRL_EM32_PINCFG_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_ST_EM32_COMMON_PINCTRL_SOC_H_ */
