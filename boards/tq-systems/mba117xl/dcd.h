/*!
 * Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>
 * SPDX-License-Identifier: Apache 2.0
 * Author: Isaac L. L. Yuki
 */

#ifndef __DCD__
#define __DCD__

#include <stdint.h>

/*! @name Driver version */
/*@{*/
/*! @brief XIP_BOARD driver version 2.0.0. */
#define FSL_XIP_BOARD_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*************************************
 *  DCD Data
 *************************************/
#define DCD_TAG_HEADER       (0xD2)
#define DCD_VERSION          (0x41)
#define DCD_TAG_HEADER_SHIFT (24)
#define DCD_ARRAY_SIZE       1

#endif /* __DCD__ */
