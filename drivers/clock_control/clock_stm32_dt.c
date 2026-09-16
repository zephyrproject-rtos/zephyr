/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Clock objects of every consumer of the STM32 RCC. Each object carries a
 * struct stm32_pclken decoded from the consumer's clock specifier cells, so
 * consumers need neither this header nor the cell layout.
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

CLOCK_DT_DEFINE_CONSUMERS_DATA(STM32_CLOCK_CONTROL_NODE, struct stm32_pclken,
			       STM32_CLOCK_DT_INIT)
