/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STHS34PF80_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STHS34PF80_H_

/*
 * Output data rate (CTRL1 ODR[3:0]).
 * Values match sths34pf80_odr_t in sths34pf80_reg.h (hal_st).
 */
#define STHS34PF80_DT_ODR_POWER_DOWN 0
#define STHS34PF80_DT_ODR_0HZ25      1
#define STHS34PF80_DT_ODR_0HZ50      2
#define STHS34PF80_DT_ODR_1HZ        3
#define STHS34PF80_DT_ODR_2HZ        4
#define STHS34PF80_DT_ODR_4HZ        5
#define STHS34PF80_DT_ODR_8HZ        6
#define STHS34PF80_DT_ODR_15HZ       7
#define STHS34PF80_DT_ODR_30HZ       8

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STHS34PF80_H_ */
