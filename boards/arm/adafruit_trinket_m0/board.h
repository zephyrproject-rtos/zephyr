/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

#define CONFIG_SPI_SAM0_SERCOM0_PADS \
	(SERCOM_SPI_CTRLA_DIPO(0) | SERCOM_SPI_CTRLA_DOPO(1))

#define CONFIG_SPI_SAM0_SERCOM1_PADS \
	(SERCOM_SPI_CTRLA_DIPO(2) | SERCOM_SPI_CTRLA_DOPO(0))

#endif /* __INC_BOARD_H */
