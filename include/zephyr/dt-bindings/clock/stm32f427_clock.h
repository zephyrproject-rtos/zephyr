/*
 * Copyright (c) 2023 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F427_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F427_CLOCK_H_

#include "stm32f4_clock.h"

/** @brief RCC_DCKCFGR register offset */
#define DCKCFGR_REG		0x8C

/** @brief Device domain clocks selection helpers */
/** DCKCFGR devices */
#define CKDFSDM2A_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 14, DCKCFGR_REG)
#define CKDFSDM1A_SEL(val)	STM32_DT_CLOCK_SELECT((val), 1, 15, DCKCFGR_REG)
#define SAI1A_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, DCKCFGR_REG)
#define SAI1B_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 22, DCKCFGR_REG)
#define CLK48M_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 27, DCKCFGR_REG)
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 28, DCKCFGR_REG)
#define DSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 29, DCKCFGR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F427_CLOCK_H_ */
