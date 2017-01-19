/* spi_dw_regs.h - Designware SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_DW_REGS_H__
#define __SPI_DW_REGS_H__

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

/* Register helpers */
DEFINE_MM_REG_WRITE(ctrlr0, DW_SPI_REG_CTRLR0, 32)
DEFINE_MM_REG_WRITE(ser, DW_SPI_REG_SER, 8)
DEFINE_MM_REG_WRITE(txftlr, DW_SPI_REG_TXFTLR, 32)
DEFINE_MM_REG_WRITE(rxftlr, DW_SPI_REG_RXFTLR, 32)
DEFINE_MM_REG_READ(rxftlr, DW_SPI_REG_RXFTLR, 32)
DEFINE_MM_REG_READ(txftlr, DW_SPI_REG_TXFTLR, 32)
DEFINE_MM_REG_WRITE(dr, DW_SPI_REG_DR, 32)
DEFINE_MM_REG_READ(dr, DW_SPI_REG_DR, 32)
DEFINE_MM_REG_READ(ssi_comp_version, DW_SPI_REG_SSI_COMP_VERSION, 32)

/* ICR is on a unique bit */
DEFINE_TEST_BIT_OP(icr, DW_SPI_REG_ICR, DW_SPI_SR_ICR_BIT)
#define clear_interrupts(addr) test_bit_icr(addr)

#ifdef CONFIG_SPI_DW_CLOCK_GATE
static inline void _clock_config(struct device *dev)
{
	struct device *clk;
	char *drv = CONFIG_SPI_DW_CLOCK_GATE_DRV_NAME;

	clk = device_get_binding(drv);
	if (clk) {
		struct spi_dw_data *spi = dev->driver_data;

		spi->clock = clk;
	}
}

static inline void _clock_on(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	clock_control_on(spi->clock, info->clock_data);
}

static inline void _clock_off(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	clock_control_off(spi->clock, info->clock_data);
}
#else
#define _clock_config(...)
#define _clock_on(...)
#define _clock_off(...)
#endif /* CONFIG_SPI_DW_CLOCK_GATE */

#endif /* __SPI_DW_REGS_H__ */
