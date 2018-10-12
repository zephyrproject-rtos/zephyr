/* spi_dw_quark_se_ss.h - Designware SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_DW_QUARK_SE_SS_REGS_H_
#define ZEPHYR_DRIVERS_SPI_SPI_DW_QUARK_SE_SS_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Registers:
 * Some registers have been collapsed into one
 * - SER is part of SSIENR
 * - TXFTLR is part of RXFTLR
 * This requires a little bit different access functions
 * see below: write_ser(), write_rxftlr() and write_txftlr()
 */
#define DW_SPI_REG_CTRLR0		(0x00)
#define DW_SPI_REG_SSIENR		(0x02)
#define DW_SPI_REG_BAUDR		(0x04)
#define DW_SPI_REG_RXFTLR		(0x05)
#define DW_SPI_REG_TXFLR		(0x07)
#define DW_SPI_REG_RXFLR		(0x08)
#define DW_SPI_REG_SR			(0x09)
#define DW_SPI_REG_ISR			(0x0a)
#define DW_SPI_REG_IMR			(0x0b)
#define DW_SPI_REG_ICR			(0x0c)
#define DW_SPI_REG_DR			(0x0d)

#define DW_SPI_CTRLR0_CLK_ENA_BIT	(15)
#define DW_SPI_CTRLR0_CLK_ENA_MASK	BIT(DW_SPI_CTRLR0_CLK_ENA_BIT)
#define DW_SPI_QSS_SSIENR_SER(__slv)	(__slv << 4)
#define DW_SPI_QSS_TXFTLR(__lvl)	(__lvl << 16)

#define DW_SPI_QSS_SER_MASK		(0xf0)
#define DW_SPI_QSS_RXFTLR_MASK		(0x0000ffff)
#define DW_SPI_QSS_TXFTLR_MASK		(0xffff0000)

#define DW_SPI_DR_WD_BIT		(30)
#define DW_SPI_DR_WD_MASK		BIT(DW_SPI_DR_WD_BIT)
#define DW_SPI_DR_STROBE_BIT		(31)
#define DW_SPI_DR_STROBE_MASK		BIT(DW_SPI_DR_STROBE_BIT)

#define DW_SPI_DR_WRITE			(DW_SPI_DR_STROBE_MASK | \
					 DW_SPI_DR_WD_MASK)
#define DW_SPI_DR_READ			(DW_SPI_DR_STROBE_MASK)

/* Register helpers
 * *_b functions only used for creating proper ser one
 */

/* CTRLR0 on Quark SE SS has a CLK_ENA bit we want to keep
 * as it is while writing the configuration.
 * *_b function only used for creating proper ser one
 */
DEFINE_MM_REG_READ(ctrlr0_b, DW_SPI_REG_CTRLR0, 16)
static inline u32_t read_ctrlr0(u32_t addr)
{
	return read_ctrlr0_b(addr);
}

DEFINE_MM_REG_WRITE(ctrlr0_b, DW_SPI_REG_CTRLR0, 16)
static inline void write_ctrlr0(u32_t data, u32_t addr)
{
	write_ctrlr0_b((read_ctrlr0_b(addr) & DW_SPI_CTRLR0_CLK_ENA_MASK) |
								data, addr);
}

DEFINE_MM_REG_READ(ctrlr1_b, DW_SPI_REG_CTRLR0, 32)
DEFINE_MM_REG_WRITE(ctrlr1_b, DW_SPI_REG_CTRLR0, 32)
static inline void write_ctrlr1(u32_t data, u32_t addr)
{
	write_ctrlr1_b((read_ctrlr1_b(addr) & (data << 16)), addr);
}

DEFINE_MM_REG_READ(ssienr_b, DW_SPI_REG_SSIENR, 8)
DEFINE_MM_REG_WRITE(ssienr_b, DW_SPI_REG_SSIENR, 8)
static inline void write_ser(u32_t data, u32_t addr)
{
	write_ssienr_b((read_ssienr_b(addr) & (~DW_SPI_QSS_SER_MASK)) |
					DW_SPI_QSS_SSIENR_SER(data), addr);
}

DEFINE_MM_REG_READ(rxftlr_b, DW_SPI_REG_RXFTLR, 32)
DEFINE_MM_REG_WRITE(rxftlr_b, DW_SPI_REG_RXFTLR, 32)
DEFINE_MM_REG_READ(rxftlr, DW_SPI_REG_RXFTLR, 16)

static inline void write_rxftlr(u32_t data, u32_t addr)
{
	write_rxftlr_b((read_rxftlr_b(addr) & (~DW_SPI_QSS_RXFTLR_MASK)) |
								data, addr);
}

static inline void write_txftlr(u32_t data, u32_t addr)
{
	write_rxftlr_b((read_rxftlr_b(addr) & (~DW_SPI_QSS_TXFTLR_MASK)) |
						DW_SPI_QSS_TXFTLR(data), addr);
}

/* Quark SE SS requires to clear up all interrupts */
DEFINE_MM_REG_WRITE(icr, DW_SPI_REG_ICR, 8)
static inline void clear_interrupts(u32_t addr)
{
	write_icr(0x1f, addr);
}

/* Reading and Writing Data
 * Quark SE SS DW SPI controller requires more logic from the driver which:
 * - needs to tell when it has been pushing in bits for TX FIFO
 * - needs to tell when it will be pulling out bits from RX FIFO
 */
DEFINE_MM_REG_WRITE(dr_b, DW_SPI_REG_DR, 32)
DEFINE_MM_REG_READ(dr_b, DW_SPI_REG_DR, 32)
static inline void write_dr(u32_t data, u32_t addr)
{
	write_dr_b(data | DW_SPI_DR_WRITE, addr);
}

static inline u32_t read_dr(u32_t addr)
{
	write_dr_b(DW_SPI_DR_READ, addr);
	__asm__("nop\n");
	return read_dr_b(addr);
}

/* Internal clock gating */
DEFINE_SET_BIT_OP(clk_ena, DW_SPI_REG_CTRLR0, DW_SPI_CTRLR0_CLK_ENA_BIT)
DEFINE_CLEAR_BIT_OP(clk_ena, DW_SPI_REG_CTRLR0, DW_SPI_CTRLR0_CLK_ENA_BIT)

static inline void _extra_clock_on(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;

	set_bit_clk_ena(info->regs);
}

static inline void _extra_clock_off(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;

	clear_bit_clk_ena(info->regs);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPI_DW_QUARK_SE_SS_REGS_H_ */
