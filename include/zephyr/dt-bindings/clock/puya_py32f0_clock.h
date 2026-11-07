/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_PUYA_PY32F0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_PUYA_PY32F0_CLOCK_H_

#include <zephyr/dt-bindings/clock/stm32_common_clocks.h>

/* RCC enable-register offsets used by the shared STM32 clock driver. */
#define STM32_CLOCK_BUS_IOP  0x034
#define STM32_CLOCK_BUS_AHB1 0x038
#define STM32_CLOCK_BUS_APB1 0x03c
#define STM32_CLOCK_BUS_APB2 0x040

#define STM32_PERIPH_BUS_MIN STM32_CLOCK_BUS_IOP
#define STM32_PERIPH_BUS_MAX STM32_CLOCK_BUS_APB2

#define STM32_SRC_HSI (STM32_SRC_LSI + 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_PUYA_PY32F0_CLOCK_H_ */
