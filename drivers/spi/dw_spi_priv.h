/* dw_spi_priv.h - Designware SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DW_SPI_PRIV_H__
#define __DW_SPI_PRIV_H__

#include <spi.h>

typedef void (*spi_dw_config_t)(struct device *dev);

/* Private structures */
struct spi_dw_config {
	uint32_t regs;
	uint32_t irq;
	uint32_t int_mask;
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	void *clock_data;
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
	spi_dw_config_t config_func;
};

struct spi_dw_data {
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	struct device *clock;
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
	uint32_t slave;
	spi_callback callback;
	void *user_data;
	uint8_t *tx_buf;
	uint32_t tx_buf_len;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	uint32_t t_len;
};

/* Registers */
#define DW_SPI_REG_CTRLR0		(0x00)
#define DW_SPI_REG_CTRLR1		(0x04)
#define DW_SPI_REG_SSIENR		(0x08)
#define DW_SPI_REG_MWCR			(0x0c)
#define DW_SPI_REG_SER			(0x10)
#define DW_SPI_REG_BAUDR		(0x14)
#define DW_SPI_REG_TXFTLR		(0x18)
#define DW_SPI_REG_RXFTLR		(0x1c)
#define DW_SPI_REG_TXFLR		(0x20)
#define DW_SPI_REG_RXFLR		(0x24)
#define DW_SPI_REG_SR			(0x28)
#define DW_SPI_REG_IMR			(0x2c)
#define DW_SPI_REG_ISR			(0x30)
#define DW_SPI_REG_RISR			(0x34)
#define DW_SPI_REG_TXOICR		(0x38)
#define DW_SPI_REG_RXOICR		(0x3c)
#define DW_SPI_REG_RXUICR		(0x40)
#define DW_SPI_REG_MSTICR		(0x44)
#define DW_SPI_REG_ICR			(0x48)
#define DW_SPI_REG_DMACR		(0x4c)
#define DW_SPI_REG_DMATDLR		(0x50)
#define DW_SPI_REG_DMARDLR		(0x54)
#define DW_SPI_REG_IDR			(0x58)
#define DW_SPI_REG_SSI_COMP_VERSION	(0x5c)
#define DW_SPI_REG_DR			(0x60)
#define DW_SPI_REG_RX_SAMPLE_DLY	(0xf0)

#define DW_SSI_COMP_VERSION		(0x3332332a)

/* CTRLR0 settings */
#define DW_SPI_CTRLR0_SCPH		(0x1 << 6)
#define DW_SPI_CTRLR0_SCPOL		(0x1 << 7)
#define DW_SPI_CTRLR0_SRL		(0x1 << 11)
#define DW_SPI_CTRLR0_DFS(__bpw)	(((__bpw) - 1) << 16)

/* SSIENR bits */
#define DW_SPI_SSIENR_SSIEN_BIT		(0)

/* SR bits and values */
#define DW_SPI_SR_BUSY_BIT		(0)
#define DW_SPI_SR_TFNF_BIT		(1)
#define DW_SPI_SR_RFNE_BIT		(3)

/* IMR values */
#define DW_SPI_IMR_TXEIM_BIT		(0)
#define DW_SPI_IMR_TXOIM_BIT		(1)
#define DW_SPI_IMR_RXUIM_BIT		(2)
#define DW_SPI_IMR_RXOIM_BIT		(3)
#define DW_SPI_IMR_RXFIM_BIT		(4)
#define DW_SPI_IMR_MSTIM_BIT		(5)

/* ISR values */
#define DW_SPI_ISR_TXEIS		(0x1 << DW_SPI_IMR_TXEIM_BIT)
#define DW_SPI_ISR_TXOIF		(0x1 << DW_SPI_IMR_TXOIM_BIT)
#define DW_SPI_ISR_RXUIS		(0x1 << DW_SPI_IMR_RXUIM_BIT)
#define DW_SPI_ISR_RXOIS		(0x1 << DW_SPI_IMR_RXOIM_BIT)
#define DW_SPI_ISR_RXFIS		(0x1 << DW_SPI_IMR_RXFIM_BIT)
#define DW_SPI_ISR_MSTIS		(0x1 << DW_SPI_IMR_MSTIM_BIT)

/* Error interrupt */
#define DW_SPI_ISR_ERRORS_MASK		(DW_SPI_ISR_TXOIF | \
					 DW_SPI_ISR_RXUIS | \
					 DW_SPI_ISR_RXOIS | \
					 DW_SPI_ISR_MSTIS)
/* ICR Bit */
#define DW_SPI_SR_ICR_BIT		(0)

/* Threshold defaults */
#define DW_SPI_TXFTLR_DFLT		(8)
#define DW_SPI_RXFTLR_DFLT		(8)

/* Interrupt mask (IMR) */
#define DW_SPI_IMR_MASK			(0x0)
#define DW_SPI_IMR_UNMASK		(0x1f)
#define DW_SPI_IMR_MASK_TX		(~(0x3))
#define DW_SPI_IMR_MASK_RX		(~(0x28))

#endif /* __DW_SPI_PRIV_H__ */
