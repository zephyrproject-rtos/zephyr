/*
 * Copyright (c) 2023 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F410_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F410_CLOCK_H_

#include "stm32f4_clock.h"

/** @brief RCC_DCKCFGR register offset */
#define DCKCFGR_REG		0x8C
#define DCKCFGR2_REG		0x94

/** @brief Device domain clocks selection helpers */
/** DCKCFGR devices */
#define CKDFSDM2A_SEL(val)	STM32_DT_CLOCK_SELECT((val), 14, 14, DCKCFGR_REG)
#define CKDFSDM1A_SEL(val)	STM32_DT_CLOCK_SELECT((val), 15, 15, DCKCFGR_REG)
#define SAI1A_SEL(val)		STM32_DT_CLOCK_SELECT((val), 21, 20, DCKCFGR_REG)
#define SAI1B_SEL(val)		STM32_DT_CLOCK_SELECT((val), 23, 22, DCKCFGR_REG)
#define I2S1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 26, 25, DCKCFGR_REG)
#define I2S2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 28, 27, DCKCFGR_REG)
#define CKDFSDM_SEL(val)	STM32_DT_CLOCK_SELECT((val), 31, 31, DCKCFGR_REG)

/** DCKCFGR2 devices */
#define I2CFMP1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 23, 22, DCKCFGR2_REG)
#define CK48M_SEL(val)		STM32_DT_CLOCK_SELECT((val), 27, 27, DCKCFGR2_REG)
#define SDIO_SEL(val)		STM32_DT_CLOCK_SELECT((val), 28, 28, DCKCFGR2_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 31, 30, DCKCFGR2_REG)

/* F4 generic I2S_SEL is not compatible with F410 devices */
#ifdef I2S_SEL
#undef I2S_SEL
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F410_CLOCK_H_ */
