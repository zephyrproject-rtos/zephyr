/* spi_dw.h - Designware SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_DW_H_
#define ZEPHYR_DRIVERS_SPI_SPI_DW_H_

#include <spi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*spi_dw_config_t)(void);

/* Private structures */
struct spi_dw_config {
	u32_t regs;
#ifdef CONFIG_CLOCK_CONTROL
	const char *clock_name;
	void *clock_data;
#endif /* CONFIG_CLOCK_CONTROL */
	spi_dw_config_t config_func;
	u8_t op_modes;
};

#include "spi_context.h"

struct spi_dw_data {
#ifdef CONFIG_CLOCK_CONTROL
	struct device *clock;
#endif /* CONFIG_CLOCK_CONTROL */
	struct spi_context ctx;
	u8_t dfs;	/* dfs in bytes: 1,2 or 4 */
	u8_t fifo_diff;	/* cannot be bigger than FIFO depth */
	u16_t _unused;
};

/* Helper macros */

#ifdef SPI_DW_SPI_CLOCK
#define SPI_DW_CLK_DIVIDER(ssi_clk_hz) \
		((SPI_DW_SPI_CLOCK / ssi_clk_hz) & 0xFFFF)
/* provision for soc.h providing a clock that is different than CPU clock */
#else
#define SPI_DW_CLK_DIVIDER(ssi_clk_hz) \
		((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / ssi_clk_hz) & 0xFFFF)
#endif


#ifdef CONFIG_SPI_DW_ARC_AUX_REGS
#define _REG_READ(__sz) sys_in##__sz
#define _REG_WRITE(__sz) sys_out##__sz
#define _REG_SET_BIT sys_io_set_bit
#define _REG_CLEAR_BIT sys_io_clear_bit
#define _REG_TEST_BIT sys_io_test_bit
#else
#define _REG_READ(__sz) sys_read##__sz
#define _REG_WRITE(__sz) sys_write##__sz
#define _REG_SET_BIT sys_set_bit
#define _REG_CLEAR_BIT sys_clear_bit
#define _REG_TEST_BIT sys_test_bit
#endif /* CONFIG_SPI_DW_ARC_AUX_REGS */

#define DEFINE_MM_REG_READ(__reg, __off, __sz)				\
	static inline u32_t read_##__reg(u32_t addr)			\
	{								\
		return _REG_READ(__sz)(addr + __off);			\
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)				\
	static inline void write_##__reg(u32_t data, u32_t addr)	\
	{								\
		_REG_WRITE(__sz)(data, addr + __off);			\
	}

#define DEFINE_SET_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline void set_bit_##__reg_bit(u32_t addr)		\
	{								\
		_REG_SET_BIT(addr + __reg_off, __bit);			\
	}

#define DEFINE_CLEAR_BIT_OP(__reg_bit, __reg_off, __bit)		\
	static inline void clear_bit_##__reg_bit(u32_t addr)		\
	{								\
		_REG_CLEAR_BIT(addr + __reg_off, __bit);		\
	}

#define DEFINE_TEST_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline int test_bit_##__reg_bit(u32_t addr)		\
	{								\
		return _REG_TEST_BIT(addr + __reg_off, __bit);		\
	}

/* Common registers settings, bits etc... */

/* CTRLR0 settings */
#define DW_SPI_CTRLR0_SCPH_BIT		(6)
#define DW_SPI_CTRLR0_SCPOL_BIT		(7)
#define DW_SPI_CTRLR0_SRL_BIT		(11)

#define DW_SPI_CTRLR0_SCPH		BIT(DW_SPI_CTRLR0_SCPH_BIT)
#define DW_SPI_CTRLR0_SCPOL		BIT(DW_SPI_CTRLR0_SCPOL_BIT)
#define DW_SPI_CTRLR0_SRL		BIT(DW_SPI_CTRLR0_SRL_BIT)

#define DW_SPI_CTRLR0_SLV_OE_BIT	(10)
#define DW_SPI_CTRLR0_SLV_OE		BIT(DW_SPI_CTRLR0_SLV_OE_BIT)

#define DW_SPI_CTRLR0_TMOD_SHIFT	(8)
#define DW_SPI_CTRLR0_TMOD_TX_RX	(0)
#define DW_SPI_CTRLR0_TMOD_TX		(1 << DW_SPI_CTRLR0_TMOD_SHIFT)
#define DW_SPI_CTRLR0_TMOD_RX		(2 << DW_SPI_CTRLR0_TMOD_SHIFT)
#define DW_SPI_CTRLR0_TMOD_EEPROM	(3 << DW_SPI_CTRLR0_TMOD_SHIFT)
#define DW_SPI_CTRLR0_TMOD_RESET	(3 << DW_SPI_CTRLR0_TMOD_SHIFT)

#define DW_SPI_CTRLR0_DFS_16(__bpw)	((__bpw) - 1)
#define DW_SPI_CTRLR0_DFS_32(__bpw)	(((__bpw) - 1) << 16)

#ifdef CONFIG_ARC
#define DW_SPI_CTRLR0_DFS		DW_SPI_CTRLR0_DFS_16
#else
#define DW_SPI_CTRLR0_DFS		DW_SPI_CTRLR0_DFS_32
#endif

/* 0x38 represents the bits 8, 16 and 32. Knowing that 24 is bits 8 and 16
 * These are the bits were when you divide by 8, you keep the result as it is.
 * For all the other ones, 4 to 7, 9 to 15, etc... you need a +1,
 * since on such division it takes only the result above 0
 */
#define SPI_WS_TO_DFS(__bpw)		(((__bpw) & ~0x38) ?		\
					 (((__bpw) / 8) + 1) :		\
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
#define DW_SPI_IMR_TXEIM		BIT(DW_SPI_IMR_TXEIM_BIT)
#define DW_SPI_IMR_TXOIM		BIT(DW_SPI_IMR_TXOIM_BIT)
#define DW_SPI_IMR_RXUIM		BIT(DW_SPI_IMR_RXUIM_BIT)
#define DW_SPI_IMR_RXOIM		BIT(DW_SPI_IMR_RXOIM_BIT)
#define DW_SPI_IMR_RXFIM		BIT(DW_SPI_IMR_RXFIM_BIT)
#define DW_SPI_IMR_MSTIM		BIT(DW_SPI_IMR_MSTIM_BIT)

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
#define DW_SPI_FIFO_DEPTH		CONFIG_SPI_DW_FIFO_DEPTH
#define DW_SPI_TXFTLR_DFLT		((DW_SPI_FIFO_DEPTH * 1) / 2) /* 50% */
#define DW_SPI_RXFTLR_DFLT		((DW_SPI_FIFO_DEPTH * 5) / 8)

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

/*
 * Including the right register definition file
 * SoC SPECIFIC!
 */
#ifdef CONFIG_SOC_QUARK_SE_C1000_SS
#include "spi_dw_quark_se_ss_regs.h"
#else
#include "spi_dw_regs.h"

#define _extra_clock_on(...)
#define _extra_clock_off(...)

#endif

/* Interrupt mask
 * SoC SPECIFIC!
 */
#if defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
#ifdef CONFIG_ARC
#define _INT_UNMASK     INT_ENABLE_ARC
#else
#define _INT_UNMASK	INT_UNMASK_IA
#endif

#define _spi_int_unmask(__mask)						\
	sys_write32(sys_read32(__mask) & _INT_UNMASK, __mask)
#else
#define _spi_int_unmask(...)
#endif /* CONFIG_SOC_QUARK_SE_C1000 || CONFIG_SOC_QUARK_SE_C1000_SS */

/* Based on those macros above, here are common helpers for some registers */
DEFINE_MM_REG_WRITE(baudr, DW_SPI_REG_BAUDR, 16)
DEFINE_MM_REG_READ(txflr, DW_SPI_REG_TXFLR, 32)
DEFINE_MM_REG_READ(rxflr, DW_SPI_REG_RXFLR, 32)
DEFINE_MM_REG_WRITE(imr, DW_SPI_REG_IMR, 8)
DEFINE_MM_REG_READ(imr, DW_SPI_REG_IMR, 8)
DEFINE_MM_REG_READ(isr, DW_SPI_REG_ISR, 8)

DEFINE_SET_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_CLEAR_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_TEST_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_TEST_BIT_OP(sr_busy, DW_SPI_REG_SR, DW_SPI_SR_BUSY_BIT)

#ifdef CONFIG_CLOCK_CONTROL

#include <string.h>

static inline int _clock_config(struct device *dev)
{
	const struct spi_dw_config *info = dev->config->config_info;
	struct spi_dw_data *spi = dev->driver_data;

	if (!info->clock_name || strlen(info->clock_name) == 0) {
		spi->clock = NULL;
		return 0;
	}

	spi->clock = device_get_binding(info->clock_name);
	if (!spi->clock) {
		return -ENODEV;
	}

	return 0;
}

static inline void _clock_on(struct device *dev)
{
	struct spi_dw_data *spi = dev->driver_data;

	if (spi->clock) {
		const struct spi_dw_config *info = dev->config->config_info;

		clock_control_on(spi->clock, info->clock_data);
	}

	_extra_clock_on(dev);
}

static inline void _clock_off(struct device *dev)
{
	struct spi_dw_data *spi = dev->driver_data;

	if (spi->clock) {
		const struct spi_dw_config *info = dev->config->config_info;

		clock_control_off(spi->clock, info->clock_data);
	}

	_extra_clock_off(dev);
}
#else
#define _clock_config(...)
#define _clock_on(...)
#define _clock_off(...)
#endif /* CONFIG_CLOCK_CONTROL */

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_SPI_SPI_DW_H_ */
