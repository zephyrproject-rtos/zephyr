/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SPI_SIFIVE__H
#define _SPI_SIFIVE__H

#include "spi_context.h"

#include <sys/sys_io.h>
#include <device.h>
#include <drivers/spi.h>

#define SPI_CFG(dev) ((struct spi_sifive_cfg *) ((dev)->config))
#define SPI_DATA(dev) ((struct spi_sifive_data *) ((dev)->data))

#define SPI_REG(dev, offset) ((mem_addr_t) (SPI_CFG(dev)->base + (offset)))

/* Register Offsets */
#define REG_SCKDIV  0x000
#define REG_SCKMODE 0x004
#define REG_CSID    0x010
#define REG_CSDEF   0x014
#define REG_CSMODE  0x018
#define REG_DELAY0  0x028
#define REG_DELAY1  0x02C
#define REG_FMT     0x040
#define REG_TXDATA  0x048
#define REG_RXDATA  0x04C
#define REG_TXMARK  0x050
#define REG_RXMARK  0x054
#define REG_FCTRL   0x060
#define REG_FFMT    0x064
#define REG_IE      0x070
#define REG_IP      0x074

/* Masks */
#define SF_SCKDIV_DIV_MASK  (0xFFF << 0)
#define SF_FMT_PROTO_MASK   (0x3 << 0)
#define SF_FMT_LEN_MASK     (0xF << 16)

/* Offsets */
#define SF_SCKMODE_POL  1
#define SF_SCKMODE_PHA  0

#define SF_FMT_LEN      16
#define SF_FMT_ENDIAN   2

#define SF_FCTRL_EN 0

/* Values */
#define SF_CSMODE_AUTO  0
#define SF_CSMODE_HOLD  2
#define SF_CSMODE_OFF   3

#define SF_FMT_PROTO_SINGLE	0

#define SF_TXDATA_FULL  (1 << 31)
#define SF_RXDATA_EMPTY (1 << 31)

/* Structure Declarations */

struct spi_sifive_data {
	struct spi_context ctx;
};

struct spi_sifive_cfg {
	uint32_t base;
	uint32_t f_sys;
};

#endif /* _SPI_SIFIVE__H */
