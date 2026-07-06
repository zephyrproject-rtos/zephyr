/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_CLOCK_H_

/* clock bus references */
/** @brief AHB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB1    0
/** @brief AHB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB2    1
/** @brief APB1 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1    2
/** @brief APB2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB2    3
/** @brief APB1_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB1_2  4
/** @brief IOP bus clock enable register offset */
#define STM32_CLOCK_BUS_IOP     5
/** @brief AHB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB3    6
/** @brief AHB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB4    7
/** @brief AHB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB5    8
/** @brief AHB6 bus clock enable register offset */
#define STM32_CLOCK_BUS_AHB6    9
/** @brief APB3 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB3    10
/** @brief APB4 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB4    11
/** @brief APB5 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB5    12
/** @brief AXI bus clock enable register offset */
#define STM32_CLOCK_BUS_AXI     13
/** @brief MLAHB bus clock enable register offset */
#define STM32_CLOCK_BUS_MLAHB   14
/** @brief APB4_2 bus clock enable register offset */
#define STM32_CLOCK_BUS_APB4_2  15

/** @brief Bit shift position for the STM32 clock divider field */
#define STM32_CLOCK_DIV_SHIFT	12

/* Clock divider */
/** @brief Compute the STM32 clock divider configuration field value from a divisor */
#define STM32_CLOCK_DIV(div)	(((div) - 1) << STM32_CLOCK_DIV_SHIFT)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_CLOCK_H_ */
