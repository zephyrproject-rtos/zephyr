/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22HB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22HB_H_

/* Data rate */
#define LPS22HB_DT_ODR_POWER_DOWN		0
#define LPS22HB_DT_ODR_1HZ			1
#define LPS22HB_DT_ODR_10HZ			2
#define LPS22HB_DT_ODR_25HZ			3
#define LPS22HB_DT_ODR_50HZ			4
#define LPS22HB_DT_ODR_75HZ			5

/* LPF Mode */
#define LPS22HB_DT_LPF_DIV_2  0
#define LPS22HB_DT_LPF_DIV_9  2
#define LPS22HB_DT_LPF_DIV_20 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LPS22HB_H_ */
