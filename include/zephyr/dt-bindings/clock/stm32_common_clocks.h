/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_

/** System clock */
#define STM32_SRC_SYSCLK	0x001
/** Fixed clocks  */
#define STM32_SRC_LSE		0x002
#define STM32_SRC_LSI		0x003

/** Dummy: Add a specifier when no selection is possible */
#define NO_SEL			0xFF

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32_COMMON_CLOCKS_H_ */
