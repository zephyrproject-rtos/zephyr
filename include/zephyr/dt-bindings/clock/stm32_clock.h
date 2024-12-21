/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_CLOCK_H_

/* clock bus references */
#define STM32_CLOCK_BUS_AHB1    0
#define STM32_CLOCK_BUS_AHB2    1
#define STM32_CLOCK_BUS_APB1    2
#define STM32_CLOCK_BUS_APB2    3
#define STM32_CLOCK_BUS_APB1_2  4
#define STM32_CLOCK_BUS_IOP     5
#define STM32_CLOCK_BUS_AHB3    6
#define STM32_CLOCK_BUS_AHB4    7
#define STM32_CLOCK_BUS_AHB5    8
#define STM32_CLOCK_BUS_AHB6    9
#define STM32_CLOCK_BUS_APB3    10
#define STM32_CLOCK_BUS_APB4    11
#define STM32_CLOCK_BUS_APB5    12
#define STM32_CLOCK_BUS_AXI     13
#define STM32_CLOCK_BUS_MLAHB   14

#define STM32_CLOCK_DIV_SHIFT	12

/** Clock divider */
#define STM32_CLOCK_DIV(div)	(((div) - 1) << STM32_CLOCK_DIV_SHIFT)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_CLOCK_H_ */
