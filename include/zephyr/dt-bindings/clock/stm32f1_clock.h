/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F1_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F1_CLOCK_H_

#include "stm32_common_clocks.h"

/** Domain clocks */

/** Bus clocks */
#define STM32_CLOCK_BUS_AHB1    0x014
#define STM32_CLOCK_BUS_APB2    0x018
#define STM32_CLOCK_BUS_APB1    0x01c

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1

/** System clock */
/* defined in stm32_common_clocks.h */

/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSI           (STM32_SRC_LSI + 1)
#define STM32_SRC_HSE           (STM32_SRC_HSI + 1)
#define STM32_SRC_EXT_HSE       (STM32_SRC_HSE + 1)
#define STM32_SRC_PLLCLK        (STM32_SRC_EXT_HSE + 1)
#define STM32_SRC_TIMPCLK1	(STM32_SRC_PLLCLK + 1)
#define STM32_SRC_TIMPCLK2	(STM32_SRC_TIMPCLK1 + 1)

/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x04
#define CFGR2_REG		0x2C

/** @brief RCC_BDCR register offset */
#define BDCR_REG		0x20

/** @brief Device domain clocks selection helpers */
/** CFGR2 devices */
#define I2S2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 17, CFGR2_REG)
#define I2S3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 18, CFGR2_REG)
/** BDCR devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, BDCR_REG)

/** CFGR1 devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 24, CFGR1_REG)
/* No MCO prescaler support on STM32F1 series. */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F1_CLOCK_H_ */
