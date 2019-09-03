/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_STM32_PINCTRLF1_H_
#define ZEPHYR_STM32_PINCTRLF1_H_

#include <dt-bindings/pinctrl/stm32-pinctrl-common.h>

/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *    GPIO I/O Mode       [ 0 ]
 *    GPIO Input config   [ 1 : 2 ]
 *    GPIO Output speed   [ 3 : 4 ]
 *    GPIO Output PP/OD   [ 5 ]
 *    GPIO Output AF/GP   [ 6 ]
 *    GPIO PUPD Config    [ 7 : 8 ]
 *
 * Applicable to STM32F1 series
 */

/* Alternate functions */
/* STM32F1 Pinmux doesn't use explicit alternate functions */
/* These are kept for compatibility with other STM32 pinmux */
#define STM32_AFR_MASK			0
#define STM32_AFR_SHIFT			0

/* Port Mode */
#define STM32_MODE_INPUT		(0x0<<STM32_MODE_INOUT_SHIFT)
#define STM32_MODE_OUTPUT		(0x1<<STM32_MODE_INOUT_SHIFT)
#define STM32_MODE_INOUT_MASK		0x1
#define STM32_MODE_INOUT_SHIFT		0

/* Input Port configuration */
#define STM32_CNF_IN_ANALOG		(0x0<<STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_FLOAT		(0x1<<STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_PUPD		(0x2<<STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_MASK		0x3
#define STM32_CNF_IN_SHIFT		1

/* Output Port configuration */
#define STM32_MODE_OUTPUT_MAX_10	(0x0<<STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OUTPUT_MAX_2		(0x1<<STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OUTPUT_MAX_50	(0x2<<STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OSPEED_MASK		0x3
#define STM32_MODE_OSPEED_SHIFT		3

#define STM32_CNF_PUSH_PULL		(0x0<<STM32_CNF_OUT_0_SHIFT)
#define STM32_CNF_OPEN_DRAIN		(0x1<<STM32_CNF_OUT_0_SHIFT)
#define STM32_CNF_OUT_0_MASK		0x1
#define STM32_CNF_OUT_0_SHIFT		5

#define STM32_CNF_GP_OUTPUT		(0x0<<STM32_CNF_OUT_1_SHIFT)
#define STM32_CNF_ALT_FUNC		(0x1<<STM32_CNF_OUT_1_SHIFT)
#define STM32_CNF_OUT_1_MASK		0x1
#define STM32_CNF_OUT_1_SHIFT		6

/* GPIO High impedance/Pull-up/Pull-down */
#define STM32_PUPD_NO_PULL		(0x0<<STM32_PUPD_SHIFT)
#define STM32_PUPD_PULL_UP		(0x1<<STM32_PUPD_SHIFT)
#define STM32_PUPD_PULL_DOWN		(0x2<<STM32_PUPD_SHIFT)
#define STM32_PUPD_MASK			0x3
#define STM32_PUPD_SHIFT		7

/* Alternate defines */
/* IO pin functions are mostly common across STM32 devices. Notable
 * exception is STM32F1 as these MCUs do not have registers for
 * configuration of pin's alternate function. The configuration is
 * done implicitly by setting specific mode and config in MODE and CNF
 * registers for particular pin.
 */

#define STM32_PIN_USART_TX		(STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC | STM32_CNF_PUSH_PULL | STM32_PUPD_PULL_UP)
#define STM32_PIN_USART_RX		(STM32_MODE_INPUT | STM32_CNF_IN_FLOAT)
#define STM32_PIN_I2C			(STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC | STM32_CNF_OPEN_DRAIN)
#define STM32_PIN_PWM			(STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC | STM32_CNF_PUSH_PULL)
#define STM32_PIN_SPI_MASTER_SCK	(STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC | STM32_CNF_PUSH_PULL)
#define STM32_PIN_SPI_SLAVE_SCK		(STM32_MODE_INPUT | STM32_CNF_IN_FLOAT)
#define STM32_PIN_SPI_MASTER_MOSI	(STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC | STM32_CNF_PUSH_PULL)
#define STM32_PIN_SPI_SLAVE_MOSI	(STM32_MODE_INPUT | STM32_CNF_IN_FLOAT)
#define STM32_PIN_SPI_MASTER_MISO	(STM32_MODE_INPUT | STM32_CNF_IN_FLOAT)
#define STM32_PIN_SPI_SLAVE_MISO	(STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC | STM32_CNF_PUSH_PULL)

/*
 * Reference manual (RM0008)
 * Section 25.3.1: Slave select (NSS) pin management
 *
 * Hardware NSS management:
 * - NSS output disabled: allows multimaster capability for devices operating
 *   in master mode.
 * - NSS output enabled: used only when the device operates in master mode.
 *
 * Software NSS management:
 * - External NSS pin remains free for other application uses.
 *
 */

/* Hardware master NSS output disabled */
#define STM32_PIN_SPI_MASTER_NSS	(STM32_MODE_INPUT | STM32_CNF_IN_FLOAT)
/* Hardware master NSS output enabled */
#define STM32_PIN_SPI_MASTER_NSS_OE	(STM32_MODE_OUTPUT | \
						STM32_CNF_ALT_FUNC | \
						STM32_CNF_PUSH_PULL | \
						STM32_PUPD_PULL_UP)
#define STM32_PIN_SPI_SLAVE_NSS		(STM32_MODE_INPUT | STM32_CNF_IN_FLOAT)
#define STM32_PIN_USB			(STM32_MODE_INPUT | STM32_CNF_IN_PUPD)

#endif	/* ZEPHYR_STM32_PINCTRLF1_H_ */
