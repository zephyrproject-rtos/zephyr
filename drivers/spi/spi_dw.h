/* spi_dw.h - Designware SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2023 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_DW_H_
#define ZEPHYR_DRIVERS_SPI_SPI_DW_H_

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

#include "spi_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*spi_dw_config_t)(void);
typedef uint32_t (*spi_dw_read_t)(uint8_t size, mm_reg_t addr, uint32_t off);
typedef void (*spi_dw_write_t)(uint8_t size, uint32_t data, mm_reg_t addr, uint32_t off);
typedef void (*spi_dw_set_bit_t)(uint8_t bit, mm_reg_t addr, uint32_t off);
typedef void (*spi_dw_clear_bit_t)(uint8_t bit, mm_reg_t addr, uint32_t off);
typedef int (*spi_dw_test_bit_t)(uint8_t bit, mm_reg_t addr, uint32_t off);

/* Private structures */
struct spi_dw_config {
	DEVICE_MMIO_ROM;
	uint32_t clock_frequency;
	spi_dw_config_t config_func;
	bool serial_target;
	uint8_t fifo_depth;
	uint8_t max_xfer_size;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
	spi_dw_read_t read_func;
	spi_dw_write_t write_func;
	spi_dw_set_bit_t set_bit_func;
	spi_dw_clear_bit_t clear_bit_func;
	spi_dw_test_bit_t test_bit_func;
};

struct spi_dw_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	uint32_t version;	/* ssi comp version */
	uint8_t dfs;	/* dfs in bytes: 1,2 or 4 */
	uint8_t fifo_diff;	/* cannot be bigger than FIFO depth */
};

/* Register operation functions */
#define DT_INST_NODE_PROP_NOT_OR(inst, prop) \
	!DT_INST_PROP(inst, prop) ||
#define DT_ANY_INST_NOT_PROP_STATUS_OKAY(prop) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_INST_NODE_PROP_NOT_OR, prop) 0)

#define DT_INST_NODE_PROP_AND_OR(inst, prop) \
	DT_INST_PROP(inst, prop) ||
#define DT_ANY_INST_PROP_STATUS_OKAY(prop) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_INST_NODE_PROP_AND_OR, prop) 0)

#if DT_ANY_INST_PROP_STATUS_OKAY(aux_reg)
static uint32_t aux_reg_read(uint8_t size, mm_reg_t addr, uint32_t off)
{
	ARG_UNUSED(size);
	return sys_in32(addr + off/4);
}

static void aux_reg_write(uint8_t size, uint32_t data, mm_reg_t addr, uint32_t off)
{
	ARG_UNUSED(size);
	sys_out32(data, addr + off/4);
}

static void aux_reg_set_bit(uint8_t bit, mm_reg_t addr, uint32_t off)
{
	sys_io_set_bit(addr + off/4, bit);
}

static void aux_reg_clear_bit(uint8_t bit, mm_reg_t addr, uint32_t off)
{
	sys_io_clear_bit(addr + off/4, bit);
}

static int aux_reg_test_bit(uint8_t bit, mm_reg_t addr, uint32_t off)
{
	return sys_io_test_bit(addr + off/4, bit);
}
#endif

#if DT_ANY_INST_NOT_PROP_STATUS_OKAY(aux_reg)
static uint32_t reg_read(uint8_t size, mm_reg_t addr, uint32_t off)
{
	switch (size) {
	case 8:
		return sys_read8(addr + off);
	case 16:
		return sys_read16(addr + off);
	case 32:
		return sys_read32(addr + off);
	default:
		return -EINVAL;
	}
}

static void reg_write(uint8_t size, uint32_t data, mm_reg_t addr, uint32_t off)
{
	switch (size) {
	case 8:
		sys_write8(data, addr + off); break;
	case 16:
		sys_write16(data, addr + off); break;
	case 32:
		sys_write32(data, addr + off); break;
	default:
		break;
	}
}

static void reg_set_bit(uint8_t bit, mm_reg_t addr, uint32_t off)
{
	sys_set_bit(addr + off, bit);
}

static void reg_clear_bit(uint8_t bit, mm_reg_t addr, uint32_t off)
{
	sys_clear_bit(addr + off, bit);
}

static int reg_test_bit(uint8_t bit, mm_reg_t addr, uint32_t off)
{
	return sys_test_bit(addr + off, bit);
}
#endif

/* Helper macros */

#define SPI_DW_CLK_DIVIDER(clock_freq, ssi_clk_hz) \
		((clock_freq / ssi_clk_hz) & 0xFFFF)

#define DEFINE_MM_REG_READ(__reg, __off, __sz)				\
	static inline uint32_t read_##__reg(const struct device *dev)	\
	{								\
		const struct spi_dw_config *info = dev->config;         \
		return info->read_func(__sz, (mm_reg_t)DEVICE_MMIO_GET(dev), __off);		\
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)				\
	static inline void write_##__reg(const struct device *dev, uint32_t data)\
	{								\
		const struct spi_dw_config *info = dev->config;         \
		info->write_func(__sz, data, (mm_reg_t)DEVICE_MMIO_GET(dev), __off);		\
	}

#define DEFINE_SET_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline void set_bit_##__reg_bit(const struct device *dev)	\
	{								\
		const struct spi_dw_config *info = dev->config;         \
		info->set_bit_func(__bit, (mm_reg_t)DEVICE_MMIO_GET(dev), __reg_off);		\
	}

#define DEFINE_CLEAR_BIT_OP(__reg_bit, __reg_off, __bit)		\
	static inline void clear_bit_##__reg_bit(const struct device *dev)\
	{								\
		const struct spi_dw_config *info = dev->config;         \
		info->clear_bit_func(__bit, (mm_reg_t)DEVICE_MMIO_GET(dev), __reg_off);		\
	}

#define DEFINE_TEST_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline int test_bit_##__reg_bit(const struct device *dev)\
	{								\
		const struct spi_dw_config *info = dev->config;         \
		return info->test_bit_func(__bit, (mm_reg_t)DEVICE_MMIO_GET(dev), __reg_off);	\
	}

/* Common registers settings, bits etc... */

/* CTRLR0 settings */
#if !defined(CONFIG_SPI_DW_HSSI)
#define DW_SPI_CTRLR0_SCPH_BIT		(6)
#define DW_SPI_CTRLR0_SCPOL_BIT		(7)
#define DW_SPI_CTRLR0_TMOD_SHIFT	(8)
#define DW_SPI_CTRLR0_SLV_OE_BIT	(10)
#define DW_SPI_CTRLR0_SRL_BIT		(11)
#else
/* The register layout is different in the HSSI variant */
#define DW_SPI_CTRLR0_SCPH_BIT		(8)
#define DW_SPI_CTRLR0_SCPOL_BIT		(9)
#define DW_SPI_CTRLR0_TMOD_SHIFT	(10)
#define DW_SPI_CTRLR0_SLV_OE_BIT	(12)
#define DW_SPI_CTRLR0_SRL_BIT		(13)
#endif

#if defined(CONFIG_SPI_DW_HSSI) && defined(CONFIG_SPI_EXTENDED_MODES)
/* TXFTLR setting. Only valid for Controller operation mode. */
#define DW_SPI_TXFTLR_TXFTLR_SHIFT	(16)
#endif

#define DW_SPI_CTRLR0_SCPH		BIT(DW_SPI_CTRLR0_SCPH_BIT)
#define DW_SPI_CTRLR0_SCPOL		BIT(DW_SPI_CTRLR0_SCPOL_BIT)
#define DW_SPI_CTRLR0_SRL		BIT(DW_SPI_CTRLR0_SRL_BIT)
#define DW_SPI_CTRLR0_SLV_OE		BIT(DW_SPI_CTRLR0_SLV_OE_BIT)

#define DW_SPI_CTRLR0_TMOD_TX_RX	(0)
#define DW_SPI_CTRLR0_TMOD_TX		(1 << DW_SPI_CTRLR0_TMOD_SHIFT)
#define DW_SPI_CTRLR0_TMOD_RX		(2 << DW_SPI_CTRLR0_TMOD_SHIFT)
#define DW_SPI_CTRLR0_TMOD_EEPROM	(3 << DW_SPI_CTRLR0_TMOD_SHIFT)
#define DW_SPI_CTRLR0_TMOD_RESET	(3 << DW_SPI_CTRLR0_TMOD_SHIFT)

#define DW_SPI_CTRLR0_DFS_16(__bpw)	((__bpw) - 1)
#define DW_SPI_CTRLR0_DFS_32(__bpw)	(((__bpw) - 1) << 16)

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

/* CLK_ENA bits */
#define DW_SPI_CLK_ENA_BIT		(0)

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
 *
 * The file included next uses the DEFINE_MM_REG macros above to
 * declare functions.  In this situation we'll leave the containing
 * extern "C" active in C++ compilations.
 */
#include "spi_dw_regs.h"

#define z_extra_clock_on(...)
#define z_extra_clock_off(...)

/* Based on those macros above, here are common helpers for some registers */

DEFINE_MM_REG_READ(txflr, DW_SPI_REG_TXFLR, 32)
DEFINE_MM_REG_READ(rxflr, DW_SPI_REG_RXFLR, 32)

#ifdef CONFIG_SPI_DW_ACCESS_WORD_ONLY
DEFINE_MM_REG_WRITE(baudr, DW_SPI_REG_BAUDR, 32)
DEFINE_MM_REG_WRITE(imr, DW_SPI_REG_IMR, 32)
DEFINE_MM_REG_READ(imr, DW_SPI_REG_IMR, 32)
DEFINE_MM_REG_READ(isr, DW_SPI_REG_ISR, 32)
#else
DEFINE_MM_REG_WRITE(baudr, DW_SPI_REG_BAUDR, 16)
DEFINE_MM_REG_WRITE(imr, DW_SPI_REG_IMR, 8)
DEFINE_MM_REG_READ(imr, DW_SPI_REG_IMR, 8)
DEFINE_MM_REG_READ(isr, DW_SPI_REG_ISR, 8)
#endif

DEFINE_SET_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_CLEAR_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_TEST_BIT_OP(ssienr, DW_SPI_REG_SSIENR, DW_SPI_SSIENR_SSIEN_BIT)
DEFINE_TEST_BIT_OP(sr_busy, DW_SPI_REG_SR, DW_SPI_SR_BUSY_BIT)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPI_DW_H_ */
