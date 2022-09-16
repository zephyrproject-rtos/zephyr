/*
 * Copyright (c) 2020 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family devicetree helper macros
 */

#ifndef _ATMEL_SAM_DT_H_
#define _ATMEL_SAM_DT_H_

#include <zephyr/devicetree.h>

/* Devicetree macros related to clock */

#define ATMEL_SAM_DT_CPU_CLK_FREQ_HZ \
		DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#endif /* _ATMEL_SAM_SOC_DT_H_ */
