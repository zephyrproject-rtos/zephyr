/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SPI_LITESPI__H
#define _SPI_LITESPI__H

#include "spi_context.h"

#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

#define SPI_BASE_ADDR		DT_INST_REG_ADDR(0)
#define SPI_CONTROL_ADDR	DT_INST_REG_ADDR_BY_NAME(0, control)
#define SPI_STATUS_ADDR		DT_INST_REG_ADDR_BY_NAME(0, status)
#define SPI_MOSI_DATA_ADDR	DT_INST_REG_ADDR_BY_NAME(0, mosi)
#define SPI_MISO_DATA_ADDR	DT_INST_REG_ADDR_BY_NAME(0, miso)
#define SPI_CS_ADDR		DT_INST_REG_ADDR_BY_NAME(0, cs)
#define SPI_LOOPBACK_ADDR	DT_INST_REG_ADDR_BY_NAME(0, loopback)

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
