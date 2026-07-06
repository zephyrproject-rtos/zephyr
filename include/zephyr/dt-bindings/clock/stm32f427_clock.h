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
/* DCKCFGR devices */
/** @brief Kernel clock source selection for CKDFSDM2A */
#define CKDFSDM2A_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 14, DCKCFGR_REG)
/** @brief Kernel clock source selection for CKDFSDM1A */
#define CKDFSDM1A_SEL(val)	STM32_DT_CLOCK_SELECT((val), 15, 15, DCKCFGR_REG)
/** @brief Kernel clock source selection for SAI1A */
#define SAI1A_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, DCKCFGR_REG)
/** @brief Kernel clock source selection for SAI1B */
#define SAI1B_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, DCKCFGR_REG)
/** @brief Kernel clock source selection for CK48M */
#define CK48M_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 27, DCKCFGR_REG)
/** @brief Kernel clock source selection for SDMMC */
#define SDMMC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 28, 28, DCKCFGR_REG)
/** @brief Kernel clock source selection for DSI */
#define DSI_SEL(val)		STM32_DT_CLOCK_SELECT((val), 29, 29, DCKCFGR_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F427_CLOCK_H_ */
