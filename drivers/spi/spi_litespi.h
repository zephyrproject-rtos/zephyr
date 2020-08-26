/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SPI_LITESPI__H
#define _SPI_LITESPI__H

#include "spi_context.h"

#include <sys/sys_io.h>
#include <device.h>
#include <drivers/spi.h>

#define SPI_BASE_ADDR DT_INST_REG_ADDR(0)
#define SPI_CONTROL_REG SPI_BASE_ADDR
#define SPI_STATUS_REG (SPI_BASE_ADDR + 0x08)
#define SPI_MOSI_DATA_REG (SPI_BASE_ADDR + 0x0c)
#define SPI_MISO_DATA_REG (SPI_BASE_ADDR + 0x10)
#define SPI_CS_REG (SPI_BASE_ADDR + 0x14)
#define SPI_LOOPBACK_REG (SPI_BASE_ADDR + 0x18)

#define POSITION_WORD_SIZE 8
#define SPI_MAX_CS_SIZE 0x100
#define SPI_WORD_SIZE 8

#define SPI_ENABLE 0x1

#define SPI_DATA(dev) ((struct spi_litespi_data *) ((dev)->data))

/* Structure Declarations */

struct spi_litespi_data {
	struct spi_context ctx;
};

struct spi_litespi_cfg {
	uint32_t base;
	uint32_t f_sys;
};

#endif /* _SPI_LITESPI__H */
