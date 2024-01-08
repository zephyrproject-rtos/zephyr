/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSO_H_

/* Accel power-modes */
#define LSM6DSO_DT_XL_HP_MODE			0
#define LSM6DSO_DT_XL_LP_NORMAL_MODE		1
#define LSM6DSO_DT_XL_ULP_MODE			2

/* Gyro power-modes */
#define LSM6DSO_DT_GY_HP_MODE			0
#define LSM6DSO_DT_GY_NORMAL_MODE		1

/* Accel range */
#define LSM6DSO_DT_FS_2G			0
#define LSM6DSO_DT_FS_16G			1
#define LSM6DSO_DT_FS_4G			2
#define LSM6DSO_DT_FS_8G			3

/* Gyro range */
#define LSM6DSO_DT_FS_250DPS			0
#define LSM6DSO_DT_FS_125DPS			1
#define LSM6DSO_DT_FS_500DPS			2
#define LSM6DSO_DT_FS_1000DPS			4
#define LSM6DSO_DT_FS_2000DPS			6

/* Accel and Gyro Data rates */
#define LSM6DSO_DT_ODR_OFF			0x0
#define LSM6DSO_DT_ODR_12Hz5			0x1
#define LSM6DSO_DT_ODR_26H			0x2
#define LSM6DSO_DT_ODR_52Hz			0x3
#define LSM6DSO_DT_ODR_104Hz			0x4
#define LSM6DSO_DT_ODR_208Hz			0x5
#define LSM6DSO_DT_ODR_417Hz			0x6
#define LSM6DSO_DT_ODR_833Hz			0x7
#define LSM6DSO_DT_ODR_1667Hz			0x8
#define LSM6DSO_DT_ODR_3333Hz			0x9
#define LSM6DSO_DT_ODR_6667Hz			0xa
#define LSM6DSO_DT_ODR_1Hz6			0xb

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSO_H_ */
