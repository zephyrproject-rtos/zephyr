/*
 * Copyright (c) 2019 Western Digital Corporation or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_context.h"

#define SPI_OC_SIMPLE_DATA(dev) \
	((struct spi_oc_simple_data *) ((dev)->driver_data))

#define SPI_OC_SIMPLE_REG(info, offset) \
	((mem_addr_t) (info->base + \
		       (offset * CONFIG_SPI_OC_SIMPLE_BUS_WIDTH / 8)))

#define SPI_OC_SIMPLE_SPCR(dev) SPI_OC_SIMPLE_REG(dev, 0x0)
#define SPI_OC_SIMPLE_SPSR(dev) SPI_OC_SIMPLE_REG(dev, 0x1)
#define SPI_OC_SIMPLE_SPDR(dev) SPI_OC_SIMPLE_REG(dev, 0x2)
#define SPI_OC_SIMPLE_SPER(dev) SPI_OC_SIMPLE_REG(dev, 0x3)
#define SPI_OC_SIMPLE_SPSS(dev) SPI_OC_SIMPLE_REG(dev, 0x4)

#define SPI_OC_SIMPLE_SPCR_SPE  BIT(6)
#define SPI_OC_SIMPLE_SPCR_CPOL BIT(3)
#define SPI_OC_SIMPLE_SPCR_CPHA BIT(2)

struct spi_oc_simple_cfg {
	u32_t base;
	u32_t f_sys;
};

struct spi_oc_simple_data {
	struct spi_context ctx;
};

