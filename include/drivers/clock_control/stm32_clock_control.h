/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_

#include <drivers/clock_control.h>
#include <dt-bindings/clock/stm32_clock.h>

/* common clock control device node for all STM32 chips */
#define STM32_CLOCK_CONTROL_NODE DT_NODELABEL(rcc)

struct stm32_pclken {
	uint32_t bus;
	uint32_t enr;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_ */
