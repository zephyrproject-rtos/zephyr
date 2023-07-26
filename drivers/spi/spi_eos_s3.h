/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_EOS_S3_H_
#define ZEPHYR_DRIVERS_SPI_SPI_EOS_S3_H_

#define SPI_WORD_SIZE	8
#define NO_RX_DATA	-1
#define NO_TX_DATA	-2
#define SPI_CLK		MHZ(10)

typedef enum {
	TXRX_MODE,
	TX_MODE,
	RX_MODE,
	EEPROM_READ_MODE
} SPI_Mode;

#endif	/* ZEPHYR_DRIVERS_SPI_SPI_EOS_S3_H_ */
