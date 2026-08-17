/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_RTS5817_H_
#define ZEPHYR_DRIVERS_SPI_SPI_RTS5817_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>

#include "spi_context.h"
#include <stdint.h>

#define R_MST_SPI_SSI_START_CTRL        0x00
#define R_MST_SPI_SSI_STOP_CTRL         0x04
#define R_MST_SPI_SSI_RD_ADDR           0x08
#define R_MST_SPI_SSI_WR_ADDR           0x0C
#define R_MST_SPI_SSI_DATA_LEN          0x10
#define R_MST_SPI_SSI_CONTROL           0x14
#define R_MST_SPI_SSI_IRQ_ENABLE        0x18
#define R_MST_SPI_SSI_IRQ_STATUS        0x1C
#define R_MST_SPI_SSI_MS_SELECT         0x20
#define R_MST_SPI_SSI_DMA_STATE         0x24
#define R_MST_SPI_SSI_RD_FIFO_THERSHOLD 0x28

/* Bits of R_MST_SPI_SSI_START_CTRL (0x00) */
#define MST_SSI_START_MASK BIT(0)

/* Bits of R_MST_SPI_SSI_RD_ADDR (0x08) */
#define MST_SSI_SRAM_RD_ADDR_MASK GENMASK(31, 0)

/* Bits of R_MST_SPI_SSI_WR_ADDR (0x0C) */
#define MST_SSI_SRAM_WR_ADDR_MASK GENMASK(31, 0)

/* Bits of R_MST_SPI_SSI_CONTROL (0x14) */
#define MST_SCK_INTERVAL_EN_OFFSET 0
#define MST_SCK_INTERVAL_EN_MASK   GENMASK(1, 0)

/* Bits of R_MST_SPI_SSI_IRQ_ENABLE (0x18) and R_MST_SPI_SSI_IRQ_STATUS (0x1C) */
#define MST_DONE_INT_MASK              BIT(0)
#define MST_SSI_WR_SRAM_OVERFLOW_MASK  BIT(1)
#define MST_SSI_RD_SRAM_UNDERFLOW_MASK BIT(2)

#define SSI_MST_INT_MASK                                                                           \
	(MST_DONE_INT_MASK | MST_SSI_WR_SRAM_OVERFLOW_MASK | MST_SSI_RD_SRAM_UNDERFLOW_MASK)

typedef void (*spi_rts5817_irq_config_t)(void);

struct spi_rts5817_config {
	uint32_t ctrl_regs;              /* m_ctrl regs base */
	const struct device *dw_spi_dev; /* underlying DW SSI device */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	spi_rts5817_irq_config_t irq_config_func;
};

struct spi_rts5817_data {
	struct spi_context ctx;
	uint32_t clock_frequency;
	uint8_t dfs;                /* dfs in bytes: 1,2 or 4 */
	size_t xfer_len;            /* in-flight chunk length */
	uint8_t *aligned_buffer;    /* DMA bounce buffer */
	size_t aligned_buffer_size; /* allocated size of aligned_buffer */
};

#define RTS5817_SPI_RX_BUFF_ALIGN_SIZE 32
#define RTS5817_SPI_TIME_OUT_MS        1000

#define RTS5817_SPI_TXFTLR_VALUE 1
#define RTS5817_SPI_RXFTLR_VALUE 1

#endif
