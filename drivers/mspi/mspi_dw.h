/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is a part of mspi_dw.c extracted only for clarity.
 * It is not supposed to be included by any file other than mspi_dw.c.
 */

/* CTRLR0 - Control Register 0 */
#define CTRLR0_SPI_FRF_MASK	GENMASK(23, 22)
#define CTRLR0_SPI_FRF_STANDARD	0UL
#define CTRLR0_SPI_FRF_DUAL	1UL
#define CTRLR0_SPI_FRF_QUAD	2UL
#define CTRLR0_SPI_FRF_OCTAL	3UL
#define CTRLR0_TMOD_MASK	GENMASK(11, 10)
#define CTRLR0_TMOD_TX_RX	0UL
#define CTRLR0_TMOD_TX		1UL
#define CTRLR0_TMOD_RX		2UL
#define CTRLR0_TMOD_EEPROM	3UL
#define CTRLR0_SCPOL_BIT	BIT(9)
#define CTRLR0_SCPH_BIT		BIT(8)
#define CTRLR0_FRF_MASK		GENMASK(7, 6)
#define CTRLR0_FRF_SPI		0UL
#define CTRLR0_FRF_SSP		1UL
#define CTRLR0_FRF_MICROWIRE	2UL
#define CTRLR0_DFS_MASK		GENMASK(4, 0)

/* CTRLR1- Control Register 1 */
#define CTRLR1_NDF_MASK		GENMASK(15, 0)

/* SSIENR - SSI Enable Register */
#define SSIENR_SSIC_EN_BIT	BIT(0)

/* TXFTLR - Transmit FIFO Threshold Level */
#define TXFTLR_TXFTHR_MASK	GENMASK(23, 16)
#define TXFTLR_TFT_MASK		GENMASK(7, 0)

/* RXFTLR - Receive FIFO Threshold Level */
#define RXFTLR_RFT_MASK		GENMASK(7, 0)

/* TXFLR - Transmit FIFO Level Register */
#define TXFLR_TXTFL_MASK	GENMASK(7, 0)

/* RXFLR - Receive FIFO Level Register */
#define RXFLR_RXTFL_MASK	GENMASK(7, 0)

/* SR - Status Register */
#define SR_BUSY_BIT	        BIT(0)

/* IMR - Interrupt Mask Register */
#define IMR_TXEIM_BIT		BIT(0)
#define IMR_TXOIM_BIT		BIT(1)
#define IMR_RXUIM_BIT		BIT(2)
#define IMR_RXOIM_BIT		BIT(3)
#define IMR_RXFIM_BIT		BIT(4)
#define IMR_MSTIM_BIT		BIT(5)

/* ISR - Interrupt Status Register */
#define ISR_TXEIS_BIT		BIT(0)
#define ISR_TXOIS_BIT		BIT(1)
#define ISR_RXUIS_BIT		BIT(2)
#define ISR_RXOIS_BIT		BIT(3)
#define ISR_RXFIS_BIT		BIT(4)
#define ISR_MSTIS_BIT		BIT(5)

/* SPI_CTRLR0 - SPI Control Register */
#define SPI_CTRLR0_CLK_STRETCH_EN_BIT		BIT(30)
#define SPI_CTRLR0_XIP_PREFETCH_EN_BIT		BIT(29)
#define SPI_CTRLR0_XIP_MBL_BIT			BIT(26)
#define SPI_CTRLR0_SPI_RXDS_SIG_EN_BIT		BIT(25)
#define SPI_CTRLR0_SPI_DM_EN_BIT		BIT(24)
#define SPI_CTRLR0_RXDS_VL_EN_BIT		BIT(23)
#define SPI_CTRLR0_SSIC_XIP_CONT_XFER_EN_BIT	BIT(21)
#define SPI_CTRLR0_XIP_INST_EN_BIT		BIT(20)
#define SPI_CTRLR0_XIP_DFS_HC_BIT		BIT(19)
#define SPI_CTRLR0_SPI_RXDS_EN_BIT		BIT(18)
#define SPI_CTRLR0_INST_DDR_EN_BIT		BIT(17)
#define SPI_CTRLR0_SPI_DDR_EN_BIT		BIT(16)
#define SPI_CTRLR0_WAIT_CYCLES_MASK		GENMASK(15, 11)
#define SPI_CTRLR0_WAIT_CYCLES_MAX		BIT_MASK(5)
#define SPI_CTRLR0_INST_L_MASK			GENMASK(9, 8)
#define SPI_CTRLR0_INST_L0			0UL
#define SPI_CTRLR0_INST_L4			1UL
#define SPI_CTRLR0_INST_L8			2UL
#define SPI_CTRLR0_INST_L16			3UL
#define SPI_CTRLR0_XIP_MD_BIT_EN_BIT		BIT(7)
#define SPI_CTRLR0_ADDR_L_MASK			GENMASK(5, 2)
#define SPI_CTRLR0_TRANS_TYPE_MASK		GENMASK(1, 0)
#define SPI_CTRLR0_TRANS_TYPE_TT0		0UL
#define SPI_CTRLR0_TRANS_TYPE_TT1		1UL
#define SPI_CTRLR0_TRANS_TYPE_TT2		2UL
#define SPI_CTRLR0_TRANS_TYPE_TT3		3UL

/* XIP_CTRL - XIP Control Register */
#define XIP_CTRL_XIP_PREFETCH_EN_BIT	BIT(28)
#define XIP_CTRL_XIP_MBL_MASK		GENMASK(27, 26)
#define XIP_CTRL_XIP_MBL_2		0UL
#define XIP_CTRL_XIP_MBL_4		1UL
#define XIP_CTRL_XIP_MBL_8		2UL
#define XIP_CTRL_XIP_MBL_16		3UL
#define XIP_CTRL_RXDS_SIG_EN_BIT	BIT(25)
#define XIP_CTRL_XIP_HYBERBUS_EN_BIT	BIT(24)
#define XIP_CTRL_CONT_XFER_EN_BIT	BIT(23)
#define XIP_CTRL_INST_EN_BIT		BIT(22)
#define XIP_CTRL_RXDS_EN_BIT		BIT(21)
#define XIP_CTRL_INST_DDR_EN_BIT	BIT(20)
#define XIP_CTRL_DDR_EN_BIT		BIT(19)
#define XIP_CTRL_DFS_HC_BIT		BIT(18)
#define XIP_CTRL_WAIT_CYCLES_MASK	GENMASK(17, 13)
#define XIP_CTRL_WAIT_CYCLES_MAX	BIT_MASK(5)
#define XIP_CTRL_MD_BITS_EN_BIT		BIT(12)
#define XIP_CTRL_INST_L_MASK		GENMASK(10, 9)
#define XIP_CTRL_INST_L0		0UL
#define XIP_CTRL_INST_L4		1UL
#define XIP_CTRL_INST_L8		2UL
#define XIP_CTRL_INST_L16		3UL
#define XIP_CTRL_ADDR_L_MASK		GENMASK(7, 4)
#define XIP_CTRL_TRANS_TYPE_MASK	GENMASK(3, 2)
#define XIP_CTRL_TRANS_TYPE_TT0		0UL
#define XIP_CTRL_TRANS_TYPE_TT1		1UL
#define XIP_CTRL_TRANS_TYPE_TT2		2UL
#define XIP_CTRL_FRF_MASK		GENMASK(1, 0)
#define XIP_CTRL_FRF_DUAL		1UL
#define XIP_CTRL_FRF_QUAD		2UL
#define XIP_CTRL_FRF_OCTAL		3UL

/* XIP_CTRL - XIP Control Register */
#define XIP_CTRL_XIP_PREFETCH_EN_BIT	BIT(28)
#define XIP_CTRL_XIP_MBL_MASK		GENMASK(27, 26)
#define XIP_CTRL_XIP_MBL_2		0UL
#define XIP_CTRL_XIP_MBL_4		1UL
#define XIP_CTRL_XIP_MBL_8		2UL
#define XIP_CTRL_XIP_MBL_16		3UL
#define XIP_CTRL_XIP_HYBERBUS_EN_BIT	BIT(24)
#define XIP_CTRL_CONT_XFER_EN_BIT	BIT(23)
#define XIP_CTRL_INST_EN_BIT		BIT(22)
#define XIP_CTRL_RXDS_EN_BIT		BIT(21)
#define XIP_CTRL_INST_DDR_EN_BIT	BIT(20)
#define XIP_CTRL_DDR_EN_BIT		BIT(19)
#define XIP_CTRL_DFS_HC_BIT		BIT(18)

/* XIP_WRITE_CTRL - XIP Write Control Register */
#define XIP_WRITE_CTRL_WAIT_CYCLES_MASK	GENMASK(20, 16)
#define XIP_WRITE_CTRL_WAIT_CYCLES_MAX	BIT_MASK(5)
#define XIP_WRITE_CTRL_RXDS_SIG_EN_BIT	BIT(13)
#define XIP_WRITE_CTRL_HYBERBUS_EN_BIT	BIT(12)
#define XIP_WRITE_CTRL_INST_DDR_EN_BIT	BIT(11)
#define XIP_WRITE_CTRL_SPI_DDR_EN_BIT	BIT(10)
#define XIP_WRITE_CTRL_INST_L_MASK	GENMASK(9, 8)
#define XIP_WRITE_CTRL_INST_L0		0UL
#define XIP_WRITE_CTRL_INST_L4		1UL
#define XIP_WRITE_CTRL_INST_L8		2UL
#define XIP_WRITE_CTRL_INST_L16		3UL
#define XIP_WRITE_CTRL_ADDR_L_MASK	GENMASK(7, 4)
#define XIP_WRITE_CTRL_TRANS_TYPE_MASK	GENMASK(3, 2)
#define XIP_WRITE_CTRL_TRANS_TYPE_TT0	0UL
#define XIP_WRITE_CTRL_TRANS_TYPE_TT1	1UL
#define XIP_WRITE_CTRL_TRANS_TYPE_TT2	2UL
#define XIP_WRITE_CTRL_FRF_MASK		GENMASK(1, 0)
#define XIP_WRITE_CTRL_FRF_DUAL		1UL
#define XIP_WRITE_CTRL_FRF_QUAD		2UL
#define XIP_WRITE_CTRL_FRF_OCTAL	3UL

/* Register access helpers. */
#define USES_AUX_REG(inst) + DT_INST_PROP(inst, aux_reg_enable)
#define AUX_REG_INSTANCES (0 DT_INST_FOREACH_STATUS_OKAY(USES_AUX_REG))
#define BASE_ADDR(dev) (mm_reg_t)DEVICE_MMIO_GET(dev)

#if AUX_REG_INSTANCES != 0
static uint32_t aux_reg_read(const struct device *dev, uint32_t off)
{
	return sys_in32(BASE_ADDR(dev) + off/4);
}
static void aux_reg_write(uint32_t data, const struct device *dev, uint32_t off)
{
	sys_out32(data, BASE_ADDR(dev) + off/4);
}
#endif

#if AUX_REG_INSTANCES != DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
static uint32_t reg_read(const struct device *dev, uint32_t off)
{
	return sys_read32(BASE_ADDR(dev) + off);
}
static void reg_write(uint32_t data, const struct device *dev, uint32_t off)
{
	sys_write32(data, BASE_ADDR(dev) + off);
}
#endif

#if AUX_REG_INSTANCES == 0
/* If no instance uses aux-reg access. */
#define DECLARE_REG_ACCESS()
#define DEFINE_REG_ACCESS(inst)
#define DEFINE_MM_REG_RD(reg, off) \
	static inline uint32_t read_##reg(const struct device *dev) \
	{ return reg_read(dev, off); }
#define DEFINE_MM_REG_WR(reg, off) \
	static inline void write_##reg(const struct device *dev, uint32_t data) \
	{ reg_write(data, dev, off); }

#elif AUX_REG_INSTANCES == DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
/* If all instances use aux-reg access. */
#define DECLARE_REG_ACCESS()
#define DEFINE_REG_ACCESS(inst)
#define DEFINE_MM_REG_RD(reg, off) \
	static inline uint32_t read_##reg(const struct device *dev) \
	{ return aux_reg_read(dev, off); }
#define DEFINE_MM_REG_WR(reg, off) \
	static inline void write_##reg(const struct device *dev, uint32_t data) \
	{ aux_reg_write(data, dev, off); }

#else
/* If register access varies by instance. */
#define DECLARE_REG_ACCESS() \
	uint32_t (*read)(const struct device *dev, uint32_t off); \
	void (*write)(uint32_t data, const struct device *dev, uint32_t off)
#define DEFINE_REG_ACCESS(inst) \
	COND_CODE_1(DT_INST_PROP(inst, aux_reg_enable), \
		(.read = aux_reg_read, \
		.write = aux_reg_write,), \
		(.read = reg_read, \
		.write = reg_write,))
#define DEFINE_MM_REG_RD(reg, off) \
	static inline uint32_t read_##reg(const struct device *dev) \
	{ \
		const struct mspi_dw_config *dev_config = dev->config; \
		return dev_config->read(dev, off); \
	}
#define DEFINE_MM_REG_WR(reg, off) \
	static inline void write_##reg(const struct device *dev, uint32_t data) \
	{ \
		const struct mspi_dw_config *dev_config = dev->config; \
		dev_config->write(data, dev, off); \
	}
#endif
