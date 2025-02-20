/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L1_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L1_CLOCK_H_

#include "stm32_common_clocks.h"

/** Bus gatting clocks */
#define STM32_CLOCK_BUS_AHB1    0x01c
#define STM32_CLOCK_BUS_APB2    0x020
#define STM32_CLOCK_BUS_APB1    0x024

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB1

/** Domain clocks */
/* RM0038.pdf, §6.3.14 Control/status register (RCC_CSR) */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI		(STM32_SRC_HSE + 1)

/** @brief RCC_CSR register offset */
#define CSR_REG		0x34

#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 16, CSR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L1_CLOCK_H_ */
