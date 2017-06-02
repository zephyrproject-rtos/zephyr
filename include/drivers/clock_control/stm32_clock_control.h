/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2016 RnDity Sp. z o.o.
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _STM32_CLOCK_CONTROL_H_
#define _STM32_CLOCK_CONTROL_H_

#include <clock_control.h>

/* common clock control device name for all STM32 chips */
#define STM32_CLOCK_CONTROL_NAME "stm32-cc"

/* Bus */
enum {
	STM32_CLOCK_BUS_AHB1,
	STM32_CLOCK_BUS_AHB2,
	STM32_CLOCK_BUS_APB1,
#ifdef CONFIG_SOC_SERIES_STM32L4X
	STM32_CLOCK_BUS_APB1_2,
#endif
	STM32_CLOCK_BUS_APB2,
};

struct stm32_pclken {
	u32_t bus;
	u32_t enr;
};

#endif /* _STM32_CLOCK_CONTROL_H_ */
