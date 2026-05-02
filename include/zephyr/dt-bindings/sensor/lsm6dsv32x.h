/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSV32X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSV32X_H_

#include "lsm6dsvxxx.h"

/* Accel range */
#define LSM6DSV32X_DT_FS_4G                     4
#define LSM6DSV32X_DT_FS_8G                     8
#define LSM6DSV32X_DT_FS_16G                    16
#define LSM6DSV32X_DT_FS_32G                    32

/* Gyro range */
#define LSM6DSV32X_DT_FS_125DPS			0x0
#define LSM6DSV32X_DT_FS_250DPS			0x1
#define LSM6DSV32X_DT_FS_500DPS			0x2
#define LSM6DSV32X_DT_FS_1000DPS		0x3
#define LSM6DSV32X_DT_FS_2000DPS		0x4
#define LSM6DSV32X_DT_FS_4000DPS		0xc

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSV32X_H_ */
