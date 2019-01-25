/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __EVKMIMXRT1060_DCD_SDRAM_INIT__
#define __EVKMIMXRT1060_DCD_SDRAM_INIT__

#include <stdint.h>

/*! @name Driver version */
/*@{*/
/*! @brief XIP_BOARD driver version 2.0.0. */
#define FSL_XIP_BOARD_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*************************************
 *  DCD Data
 *************************************/
#define DCD_TAG_HEADER (0xD2)
#define DCD_TAG_HEADER_SHIFT (24)
#define DCD_VERSION (0x40)
#define DCD_ARRAY_SIZE 1

#endif /* __EVKMIMXRT1060_DCD_SDRAM_INIT__ */
