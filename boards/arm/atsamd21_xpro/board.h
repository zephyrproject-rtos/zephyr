/*
 * Copyright (c) 2018 Bryan O'Donoghue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* SPI SERCOM5 on MISO=PB16/pad 0, MOSI=PB22/pad 2, SCK=PB23/pad 3 */
#define CONFIG_SPI_SAM0_SERCOM5_PADS \
	(SERCOM_SPI_CTRLA_DIPO(0) | SERCOM_SPI_CTRLA_DOPO(2))

#endif /* __INC_BOARD_H */
