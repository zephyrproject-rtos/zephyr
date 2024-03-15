/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2ICLX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2ICLX_H_

/* Accel range */
#define IIS2ICLX_DT_FS_500mG			0
#define IIS2ICLX_DT_FS_3G			1
#define IIS2ICLX_DT_FS_1G			2
#define IIS2ICLX_DT_FS_2G			3

/* Accel Data rates */
#define IIS2ICLX_DT_ODR_OFF			0x0
#define IIS2ICLX_DT_ODR_12Hz5			0x1
#define IIS2ICLX_DT_ODR_26H			0x2
#define IIS2ICLX_DT_ODR_52Hz			0x3
#define IIS2ICLX_DT_ODR_104Hz			0x4
#define IIS2ICLX_DT_ODR_208Hz			0x5
#define IIS2ICLX_DT_ODR_416Hz			0x6
#define IIS2ICLX_DT_ODR_833Hz			0x7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_IIS2ICLX_H_ */
