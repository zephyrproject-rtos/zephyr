/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DCD__
#define __DCD__

#include <stdint.h>

/*! @name Driver version */
/*@{*/
/*! @brief XIP_BOARD driver version 2.0.1. */
#define FSL_XIP_BOARD_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

#define DCD_TAG_HEADER       (0xD2)
#define DCD_VERSION          (0x41)
#define DCD_TAG_HEADER_SHIFT (24)
#define DCD_ARRAY_SIZE       1

#endif /* __DCD__ */
