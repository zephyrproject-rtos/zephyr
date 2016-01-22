/* spi_dw.h - Designware SPI driver private definitions */

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

#ifndef __SPI_DW_H__
#define __SPI_DW_H__

#include <spi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*spi_dw_config_t)(void);

/* Private structures */
struct spi_dw_config {
	uint32_t regs;
	uint32_t irq;
	uint32_t int_mask;
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	void *clock_data;
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
#ifdef CONFIG_SPI_DW_CS_GPIO
	char *cs_gpio_name;
	uint32_t cs_gpio_pin;
#endif /* CONFIG_SPI_DW_CS_GPIO */
	spi_dw_config_t config_func;
};

struct spi_dw_data {
	device_sync_call_t sync;
	uint32_t error:1;
	uint32_t dfs:3; /* dfs in bytes: 1,2 or 4 */
	uint32_t slave:17; /* up 16 slaves */
	uint32_t fifo_diff:9; /* cannot be bigger than FIFO depth */
	uint32_t _unused:2;
#ifdef CONFIG_SPI_DW_CLOCK_GATE
	struct device *clock;
#endif /* CONFIG_SPI_DW_CLOCK_GATE */
#ifdef CONFIG_SPI_DW_CS_GPIO
	struct device *cs_gpio_port;
#endif /* CONFIG_SPI_DW_CS_GPIO */
	uint8_t *tx_buf;
	uint32_t tx_buf_len;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
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
#define DW_SPI_CTRLR0_DFS_16(__bpw)	((__bpw) - 1)
#define DW_SPI_CTRLR0_DFS_32(__bpw)	(((__bpw) - 1) << 16)
#ifdef CONFIG_ARC
#define DW_SPI_CTRLR0_DFS		DW_SPI_CTRLR0_DFS_16
#else
#define DW_SPI_CTRLR0_DFS		DW_SPI_CTRLR0_DFS_32
#endif

/* 0x38 represents the bits 8,16 and 32. Knowing that 24 is bits 8 and 16
 * These are the bits were when you divide by 8, you keep the result as it is.
 * For all the other ones, 4 to 7, 9 to 15, etc... you need a +1,
 * since on such division it takes only the result above 0
 */
#define SPI_DFS_TO_BYTES(__bpw)		(((__bpw) & ~0x38) ?		\
						(((__bpw) / 8) + 1) :	\
						((__bpw) / 8))

/* SSIENR bits */
#define DW_SPI_SSIENR_SSIEN_BIT		(0)

/* SR bits and values */
#define DW_SPI_SR_BUSY_BIT		(0)
#define DW_SPI_SR_TFNF_BIT		(1)
#define DW_SPI_SR_RFNE_BIT		(3)

/* IMR bits (ISR valid as well) */
#define DW_SPI_IMR_TXEIM_BIT		(0)
#define DW_SPI_IMR_TXOIM_BIT		(1)
#define DW_SPI_IMR_RXUIM_BIT		(2)
#define DW_SPI_IMR_RXOIM_BIT		(3)
#define DW_SPI_IMR_RXFIM_BIT		(4)
#define DW_SPI_IMR_MSTIM_BIT		(5)

/* IMR values */
#define DW_SPI_IMR_TXEIM		(0x1 << DW_SPI_IMR_TXEIM_BIT)
#define DW_SPI_IMR_TXOIM		(0x1 << DW_SPI_IMR_TXOIM_BIT)
#define DW_SPI_IMR_RXUIM		(0x1 << DW_SPI_IMR_RXUIM_BIT)
#define DW_SPI_IMR_RXOIM		(0x1 << DW_SPI_IMR_RXOIM_BIT)
#define DW_SPI_IMR_RXFIM		(0x1 << DW_SPI_IMR_RXFIM_BIT)
#define DW_SPI_IMR_MSTIM		(0x1 << DW_SPI_IMR_MSTIM_BIT)

/* ISR values (same as IMR) */
#define DW_SPI_ISR_TXEIS		DW_SPI_IMR_TXEIM
#define DW_SPI_ISR_TXOIS		DW_SPI_IMR_TXOIM
#define DW_SPI_ISR_RXUIS		DW_SPI_IMR_RXUIM
#define DW_SPI_ISR_RXOIS		DW_SPI_IMR_RXOIM
#define DW_SPI_ISR_RXFIS		DW_SPI_IMR_RXFIM
#define DW_SPI_ISR_MSTIS		DW_SPI_IMR_MSTIM

/* Error interrupt */
#define DW_SPI_ISR_ERRORS_MASK		(DW_SPI_ISR_TXOIS | \
					 DW_SPI_ISR_RXUIS | \
					 DW_SPI_ISR_RXOIS | \
					 DW_SPI_ISR_MSTIS)
/* ICR Bit */
#define DW_SPI_SR_ICR_BIT		(0)

/* Threshold defaults */
#define DW_SPI_TXFTLR_DFLT		(0x5)
#define DW_SPI_RXFTLR_DFLT		(0x5)
#define DW_SPI_FIFO_DEPTH		(8)

/* Interrupt mask (IMR) */
#define DW_SPI_IMR_MASK			(0x0)
#define DW_SPI_IMR_UNMASK		(DW_SPI_IMR_TXEIM | \
					 DW_SPI_IMR_TXOIM | \
					 DW_SPI_IMR_RXUIM | \
					 DW_SPI_IMR_RXOIM | \
					 DW_SPI_IMR_RXFIM)
#define DW_SPI_IMR_MASK_TX		(~(DW_SPI_IMR_TXEIM | \
					   DW_SPI_IMR_TXOIM))
#define DW_SPI_IMR_MASK_RX		(~(DW_SPI_IMR_RXUIM | \
					   DW_SPI_IMR_RXOIM | \
					   DW_SPI_IMR_RXFIM))

#ifdef __cplusplus
}
#endif
#endif /* __SPI_DW_H__ */

