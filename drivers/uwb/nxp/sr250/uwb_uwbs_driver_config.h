/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __UWB_UWBS_DRIVER_CONFIG_H__
#define __UWB_UWBS_DRIVER_CONFIG_H__

#ifdef UWBIOT_USE_FTR_FILE
#include "uwb_iot_ftr.h"
#else
#include "uwb_iot_ftr_default.h"
#endif

#if UWBIOT_UWBD_SR2XXT

#define UWB_UWBS_SPI_BAUDRATE (16 * 1000 * 1000)
#define UWB_UWBS_SPI_CPOL     0 /* Active high */
#define UWB_UWBS_SPI_CPHA     0 /* First edge */

#define DIRECTIONAL_BYTE_WRITE 0x00
#define DIRECTIONAL_BYTE_READ  0xFF

#define UWB_UWBS_SPI_ASYNC        1
#define UWB_UWBS_SPI_BLOCKING     0
#define UWB_UWBS_SPI_DMA_TRANSFER 1
#define UWB_UWBS_SPI_METHOD       UWB_UWBS_SPI_ASYNC

#if UWBIOT_TML_LIBUWBD
#define UCI_CMD_INDEX         0
#define ACTUAL_PACKET_START   0
#define DIRECTION_BYTE_OFFSET 0
#else
#define UCI_CMD_INDEX         1
#define ACTUAL_PACKET_START   2
#define DIRECTION_BYTE_OFFSET 0
#endif

#endif /* UWBIOT_UWBD_SR2XX */

#endif /* __UWB_UWBS_DRIVER_CONFIG_H__ */
