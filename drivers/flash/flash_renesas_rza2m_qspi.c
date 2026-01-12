/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_qspi_spibsc

#include <stddef.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/flash.h>
#include "spi_nor.h"
#include "flash_renesas_rza2m_qspi.h"

LOG_MODULE_REGISTER(renesas_rza2m_qspi_spibsc, CONFIG_FLASH_LOG_LEVEL);

#define ERASE_VALUE          0xff
#define QSPI_MAX_BUFFER_SIZE 256U

/* QSPI COMMANDS */
#define QSPI_CMD_RDSFDP (0x5A) /* Read SFDP */

/* One byte data transfer */
#define DATA_LENGTH_DEFAULT_BYTE (0U)
#define ONE_BYTE                 (1U)
#define TWO_BYTE                 (2U)
#define THREE_BYTE               (3U)
#define FOUR_BYTE                (4U)

enum flash_rza2m_types {
	SERIAL_FLASH,
	HYPER_FLASH,
	OCTA_FLASH,
};

struct flash_rza2m_config {
	DEVICE_MMIO_ROM; /* Must be first */

	enum flash_rza2m_types type;
	const struct pinctrl_dev_config *pcfg;

	uint32_t erase_block_size;
	uint32_t flash_size;
	struct flash_parameters flash_param;

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

struct flash_rza2m_data {
	DEVICE_MMIO_RAM; /* Must be first */
};

K_MUTEX_DEFINE(lock);

static uint8_t write_tmp_buf[QSPI_MAX_BUFFER_SIZE];

struct spibsc_reg {
	uint32_t smenr_cdb;   /* bit-width : command */
	uint32_t smenr_ocdb;  /* bit-width : optional command */
	uint32_t smenr_adb;   /* bit-width : address */
	uint32_t smenr_opdb;  /* bit-width : option data */
	uint32_t smenr_spidb; /* bit-width : data */

	uint32_t smenr_cde;   /* Enable : command */
	uint32_t smenr_ocde;  /* Enable : optional command */
	uint32_t smenr_ade;   /* Enable : address */
	uint32_t smenr_opde;  /* Enable : option data */
	uint32_t smenr_spide; /* Enable : data */

	uint32_t smcr_sslkp; /* SPBSSL level */
	uint32_t smcr_spire; /* Enable data read */
	uint32_t smcr_spiwe; /* Enable data write */

	uint32_t smenr_dme; /* Enable : dummy cycle */

	uint32_t smdrenr_addre;  /* DDR enable : address  */
	uint32_t smdrenr_opdre;  /* DDR enable : option data */
	uint32_t smdrenr_spidre; /* DDR enable : data */

	uint8_t dmdb;         /* bit-width : dummy cycle */
	uint8_t smdmcr_dmcyc; /* number of dummy cycles */

	uint8_t smcmr_cmd;    /* command */
	uint8_t smcmr_ocmd;   /* optional command */
	uint32_t smadr_addr;  /* address */
	uint8_t smopr_opd[4]; /* option data 3/2/1/0 */
	uint32_t smrdr[2];    /* read data */
	uint32_t smwdr[2];    /* write data */
};

static uint32_t spi_reg_read(const struct device *dev, uint32_t off, uint32_t shift, uint32_t mask)
{
	uint32_t val;

	val = sys_read32(DEVICE_MMIO_GET(dev) + off);
	return (val & mask) >> shift;
}

static void spi_reg_write(const struct device *dev, uint32_t off, uint32_t write_value,
			  uint32_t shift, uint32_t mask)
{
	uint32_t val;

	val = sys_read32(DEVICE_MMIO_GET(dev) + off);
	val = (val & (~mask)) | (write_value << shift);
	sys_write32(val, DEVICE_MMIO_GET(dev) + off);
}

static void spi_smrdr(const struct device *dev, uint32_t off, uint32_t spide, uint32_t *smrdr)
{
	switch (spide) {
	case SPI_OUTPUT_SPID_8:
		*smrdr = sys_read8(DEVICE_MMIO_GET(dev) + off);
		break;
	case SPI_OUTPUT_SPID_16:
		*smrdr = sys_read16(DEVICE_MMIO_GET(dev) + off);
		break;
	case SPI_OUTPUT_SPID_32:
		*smrdr = sys_read32(DEVICE_MMIO_GET(dev) + off);
		break;
	default:
		LOG_ERR("%s: Invalid transfer data enable value", __func__);
	}
}

static void spi_smwdr(const struct device *dev, uint32_t off, uint32_t spide, uint32_t smwdr)
{
	switch (spide) {
	case SPI_OUTPUT_SPID_8:
		sys_write8((uint8_t)smwdr, DEVICE_MMIO_GET(dev) + off);
		break;
	case SPI_OUTPUT_SPID_16:
		sys_write16((uint16_t)smwdr, DEVICE_MMIO_GET(dev) + off);
		break;
	case SPI_OUTPUT_SPID_32:
		sys_write32(smwdr, DEVICE_MMIO_GET(dev) + off);
		break;
	default:
		LOG_ERR("%s: Invalid transfer data enable value", __func__);
	}
}

static void write_data_section(const struct device *dev, struct spibsc_reg *regset)
{
	uint32_t bsz = spi_reg_read(dev, CMNCR_OFF, SPI_CMNCR_BSZ_SHIFT, SPI_CMNCR_BSZ);

	bool is_single = (bsz == SPI_CMNCR_BSZ_SINGLE);

	switch (regset->smenr_spide) {
	case SPI_OUTPUT_SPID_8:
		if (is_single) {
			spi_smwdr(dev, SMWDR0_OFF, regset->smenr_spide, regset->smwdr[0]);
		} else {
			spi_smwdr(dev, SMWDR0_OFF, SPI_OUTPUT_SPID_16, regset->smwdr[0]);
		}
		break;
	case SPI_OUTPUT_SPID_16:
		if (is_single) {
			spi_smwdr(dev, SMWDR0_OFF, regset->smenr_spide, regset->smwdr[0]);
		} else {
			spi_smwdr(dev, SMWDR0_OFF, SPI_OUTPUT_SPID_32, regset->smwdr[0]);
		}
		break;
	case SPI_OUTPUT_SPID_32:
		if (is_single) {
			spi_smwdr(dev, SMWDR0_OFF, regset->smenr_spide, regset->smwdr[0]);
		} else {
			spi_smwdr(dev, SMWDR0_OFF, regset->smenr_spide, regset->smwdr[0]);
			spi_smwdr(dev, SMWDR1_OFF, regset->smenr_spide, regset->smwdr[1]);
		}
		break;
	default:
		LOG_ERR("%s: Invalid transfer data enable value", __func__);
	}

	/* Single/Dual/Quad */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_spidb, SPI_SMENR_SPIDB_SHIFT, SPI_SMENR_SPIDB);
}

static int configure_data_section(const struct device *dev, struct spibsc_reg *regset)
{
	/* Data Section */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_spide, SPI_SMENR_SPIDE_SHIFT, SPI_SMENR_SPIDE);
	if (regset->smenr_spide != SPI_OUTPUT_DISABLE) {
		write_data_section(dev, regset);
	}

	spi_reg_write(dev, SMCR_OFF, regset->smcr_sslkp, SPI_SMCR_SSLKP_SHIFT, SPI_SMCR_SSLKP);

	if ((regset->smenr_spidb != SPI_1BIT) && (regset->smenr_spide != SPI_OUTPUT_DISABLE) &&
	    (regset->smcr_spire == SPI_SPIDATA_ENABLE) &&
	    (regset->smcr_spiwe == SPI_OUTPUT_ENABLE)) {
		LOG_ERR("Read/Write mode is not supported for data width != 1 bit");
		return -EINVAL;
	}

	return 0;
}

static void read_data_section(const struct device *dev, struct spibsc_reg *regset)
{
	uint32_t bsz = spi_reg_read(dev, CMNCR_OFF, SPI_CMNCR_BSZ_SHIFT, SPI_CMNCR_BSZ);

	bool is_single = (bsz == SPI_CMNCR_BSZ_SINGLE);

	switch (regset->smenr_spide) {
	case SPI_OUTPUT_SPID_8:
		if (is_single) {
			spi_smrdr(dev, SMRDR0_OFF, regset->smenr_spide, &regset->smrdr[0]);
		} else {
			spi_smrdr(dev, SMRDR0_OFF, SPI_OUTPUT_SPID_16, &regset->smrdr[0]);
		}
		break;
	case SPI_OUTPUT_SPID_16:
		if (is_single) {
			spi_smrdr(dev, SMRDR0_OFF, regset->smenr_spide, &regset->smrdr[0]);
		} else {
			spi_smrdr(dev, SMRDR0_OFF, SPI_OUTPUT_SPID_32, &regset->smrdr[0]);
		}
		break;
	case SPI_OUTPUT_SPID_32:
		if (is_single) {
			spi_smrdr(dev, SMRDR0_OFF, regset->smenr_spide, &regset->smrdr[0]);
		} else {
			spi_smrdr(dev, SMRDR0_OFF, regset->smenr_spide, &regset->smrdr[0]);
			spi_smrdr(dev, SMRDR1_OFF, regset->smenr_spide, &regset->smrdr[1]);
		}
		break;
	default:
		LOG_ERR("%s: Invalid transfer data enable value", __func__);
	}
}

static int spi_xfer(const struct device *dev, struct spibsc_reg *regset)
{
	int ret;

	if (spi_reg_read(dev, CMNCR_OFF, SPI_CMNCR_MD_SHIFT, SPI_CMNCR_MD) != SPI_CMNCR_MD_SPI) {
		if (spi_reg_read(dev, CMNSR_OFF, SPI_CMNSR_SSLF_SHIFT, SPI_CMNSR_SSLF) !=
		    SPI_SSL_NEGATE) {
			LOG_ERR("%s: SSL is in the high state", __func__);
			return -EBUSY;
		}
		/* SPI Mode */
		spi_reg_write(dev, CMNCR_OFF, SPI_CMNCR_MD_SPI, SPI_CMNCR_MD_SHIFT, SPI_CMNCR_MD);
	}

	if (spi_reg_read(dev, CMNSR_OFF, SPI_CMNSR_TEND_SHIFT, SPI_CMNSR_TEND) != SPI_TRANS_END) {
		LOG_ERR("%s: transaction is still in progress", __func__);
		return -EBUSY;
	}

	/* Command Section */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_cde, SPI_SMENR_CDE_SHIFT, SPI_SMENR_CDE);
	if (regset->smenr_cde != SPI_OUTPUT_DISABLE) {
		/* Command */
		spi_reg_write(dev, SMCMR_OFF, regset->smcmr_cmd, SPI_SMCMR_CMD_SHIFT,
			      SPI_SMCMR_CMD);
		/* Single/Dual/Quad */
		spi_reg_write(dev, SMENR_OFF, regset->smenr_cdb, SPI_SMENR_CDB_SHIFT,
			      SPI_SMENR_CDB);
	}

	/* Option Command Section */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_ocde, SPI_SMENR_OCDE_SHIFT, SPI_SMENR_OCDE);
	if (regset->smenr_ocde != SPI_OUTPUT_DISABLE) {
		/* Option Command */
		spi_reg_write(dev, SMCMR_OFF, regset->smcmr_ocmd, SPI_SMCMR_OCMD_SHIFT,
			      SPI_SMCMR_OCMD);
		/* Single/Dual/Quad */
		spi_reg_write(dev, SMENR_OFF, regset->smenr_ocdb, SPI_SMENR_OCDB_SHIFT,
			      SPI_SMENR_OCDB);
	}

	/* Address Section */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_ade, SPI_SMENR_ADE_SHIFT, SPI_SMENR_ADE);
	if (regset->smenr_ade != SPI_OUTPUT_DISABLE) {
		/* Address */
		spi_reg_write(dev, SMADR_OFF, regset->smadr_addr, SPI_SMADR_ADR_SHIFT,
			      SPI_SMADR_ADR);
		/* Single/Dual/Quad */
		spi_reg_write(dev, SMENR_OFF, regset->smenr_adb, SPI_SMENR_ADB_SHIFT,
			      SPI_SMENR_ADB);
	}

	/* Option Data Section */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_opde, SPI_SMENR_OPDE_SHIFT, SPI_SMENR_OPDE);
	if (regset->smenr_opde != SPI_OUTPUT_DISABLE) {
		/* Option Data */
		spi_reg_write(dev, SMOPR_OFF, regset->smopr_opd[0], SPI_SMOPR_OPD3_SHIFT,
			      SPI_SMOPR_OPD3);
		spi_reg_write(dev, SMOPR_OFF, regset->smopr_opd[1], SPI_SMOPR_OPD2_SHIFT,
			      SPI_SMOPR_OPD2);
		spi_reg_write(dev, SMOPR_OFF, regset->smopr_opd[2], SPI_SMOPR_OPD1_SHIFT,
			      SPI_SMOPR_OPD1);
		spi_reg_write(dev, SMOPR_OFF, regset->smopr_opd[3], SPI_SMOPR_OPD0_SHIFT,
			      SPI_SMOPR_OPD0);
		/* Single/Dual/Quad */
		spi_reg_write(dev, SMENR_OFF, regset->smenr_opdb, SPI_SMENR_OPDB_SHIFT,
			      SPI_SMENR_OPDB);
	}

	/* Dummy Cycles */
	spi_reg_write(dev, SMENR_OFF, regset->smenr_dme, SPI_SMENR_DME_SHIFT, SPI_SMENR_DME);
	if (regset->smenr_dme != SPI_DUMMY_CYC_DISABLE) {
		spi_reg_write(dev, SMDMCR_OFF, regset->smdmcr_dmcyc, SPI_SMDMCR_DMCYC_SHIFT,
			      SPI_SMDMCR_DMCYC);
	}

	/* Data Section */
	ret = configure_data_section(dev, regset);
	if (ret < 0) {
		return ret;
	}

	spi_reg_write(dev, SMCR_OFF, regset->smcr_spire, SPI_SMCR_SPIRE_SHIFT, SPI_SMCR_SPIRE);
	spi_reg_write(dev, SMCR_OFF, regset->smcr_spiwe, SPI_SMCR_SPIWE_SHIFT, SPI_SMCR_SPIWE);

	/* SDR/DDR Transmission Setting */
	spi_reg_write(dev, SMDRENR_OFF, regset->smdrenr_addre, SPI_SMDRENR_ADDRE_SHIFT,
		      SPI_SMDRENR_ADDRE);
	spi_reg_write(dev, SMDRENR_OFF, regset->smdrenr_opdre, SPI_SMDRENR_OPDRE_SHIFT,
		      SPI_SMDRENR_OPDRE);
	spi_reg_write(dev, SMDRENR_OFF, regset->smdrenr_spidre, SPI_SMDRENR_SPIDRE_SHIFT,
		      SPI_SMDRENR_SPIDRE);

	/* Execute after setting SPNDL bit */
	spi_reg_write(dev, SMCR_OFF, SPI_SPI_ENABLE, SPI_SMCR_SPIE_SHIFT, SPI_SMCR_SPIE);

	/* Wait for transfer-start */
	while (spi_reg_read(dev, CMNSR_OFF, SPI_CMNSR_TEND_SHIFT, SPI_CMNSR_TEND) !=
	       SPI_TRANS_END) {
		/* Wait for transfer-end */
	}

	/* Read data if needed */
	if (regset->smenr_spide != SPI_OUTPUT_DISABLE && regset->smcr_spire == SPI_SPIDATA_ENABLE) {
		read_data_section(dev, regset);
	}

	return ret;
}

static void spi_stop(const struct device *dev)
{
	uint32_t value = sys_read32(DEVICE_MMIO_GET(dev) + DRCR_OFF);

	if (((value & SPI_DRCR_RBE) != 0) && ((value & SPI_DRCR_SSLE) != 0)) {
		spi_reg_write(dev, DRCR_OFF, SPI_DRCR_SSLN_ASSERT, SPI_DRCR_SSLN_SHIFT,
			      SPI_DRCR_SSLN);
	}

	while (spi_reg_read(dev, CMNSR_OFF, SPI_CMNSR_SSLF_SHIFT, SPI_CMNSR_SSLF) !=
	       SPI_SSL_NEGATE) {
		/* Wait for SSL to be negated */
	}

	while (spi_reg_read(dev, CMNSR_OFF, SPI_CMNSR_TEND_SHIFT, SPI_CMNSR_TEND) !=
	       SPI_TRANS_END) {
		/* Wait for transfer to end */
	}
}

static void clear_spi_reg(struct spibsc_reg *regset)
{
	/* Clear settings of command */
	regset->smenr_cde = SPI_OUTPUT_DISABLE;
	regset->smenr_cdb = SPI_1BIT;
	regset->smcmr_cmd = 0x00;

	/* Clear settings of optional command */
	regset->smenr_ocde = SPI_OUTPUT_DISABLE;
	regset->smenr_ocdb = SPI_1BIT;
	regset->smcmr_ocmd = 0x00;

	/* Clear settings of address */
	regset->smenr_ade = SPI_OUTPUT_DISABLE;
	regset->smdrenr_addre = SPI_SDR_TRANS;
	regset->smenr_adb = SPI_1BIT;
	regset->smadr_addr = 0x00000000;

	/* Clear settings of option data */
	regset->smenr_opde = SPI_OUTPUT_DISABLE;
	regset->smdrenr_opdre = SPI_SDR_TRANS;
	regset->smenr_opdb = SPI_1BIT;
	regset->smopr_opd[0] = 0x00; /* OPD3 */
	regset->smopr_opd[1] = 0x00; /* OPD2 */
	regset->smopr_opd[2] = 0x00; /* OPD1 */
	regset->smopr_opd[3] = 0x00; /* OPD0 */

	/* Clear settings of dummy cycles */
	regset->smenr_dme = SPI_DUMMY_CYC_DISABLE;
	regset->dmdb = SPI_1BIT;
	regset->smdmcr_dmcyc = SPI_DUMMY_2CYC;

	/* Clear settings of Transfer Data */
	regset->smenr_spide = SPI_OUTPUT_DISABLE;
	regset->smdrenr_spidre = SPI_SDR_TRANS;
	regset->smenr_spidb = SPI_1BIT;

	regset->smcr_sslkp = SPI_SPISSL_NEGATE;   /* SPBSSL level */
	regset->smcr_spire = SPI_SPIDATA_DISABLE; /* Read disable */
	regset->smcr_spiwe = SPI_SPIDATA_DISABLE; /* Write disable */
}

static int flash_read_register(const struct device *dev, uint8_t cmd, uint8_t *status)
{
	int ret;
	struct spibsc_reg spimd_reg;

	clear_spi_reg(&spimd_reg);

	spimd_reg.smenr_cde = SPI_OUTPUT_ENABLE;
	spimd_reg.smenr_cdb = SPI_1BIT;
	spimd_reg.smcmr_cmd = cmd;

	spimd_reg.smcr_sslkp = SPI_SPISSL_NEGATE;  /* SPBSSL level */
	spimd_reg.smcr_spire = SPI_SPIDATA_ENABLE; /* Read enable */
	spimd_reg.smcr_spiwe = SPI_SPIDATA_ENABLE; /* Write enable */

	spimd_reg.smenr_spide = SPI_OUTPUT_SPID_8; /* Enable 8bit */
	spimd_reg.smdrenr_spidre = SPI_SDR_TRANS;
	spimd_reg.smenr_spidb = SPI_1BIT;

	spimd_reg.smwdr[0] = 0x00; /* Output 0 in read status */
	spimd_reg.smwdr[1] = 0x00; /* Output 0 in read status */

	ret = spi_xfer(dev, &spimd_reg);
	if (ret == 0) {
		*status = (uint8_t)(spimd_reg.smrdr[0]); /* Data[7:0]  */
	}

	return ret;
}

static int wait_status_from_flash(const struct device *dev)
{
	int ret;
	uint8_t status_reg = true; /* Wip = true */

	while (true) {
		ret = flash_read_register(dev, SPI_NOR_CMD_RDSR, &status_reg);
		if (ret < 0) {
			LOG_ERR("Failed to read status register");
			break;
		}
		if ((status_reg & SPI_NOR_WIP_BIT) == 0) {
			break;
		}
	}

	return ret;
}

static void spi_timing_adjustment(const struct device *dev)
{
	/*
	 * The setting values follow User's manual
	 * SPI Multi I/O Bus Controller for RZ/A2M Group
	 * Section: Timing Adjustment
	 */
	sys_write32(0xa5390000U, DEVICE_MMIO_GET(dev) + PHYADJ2_OFF);
	sys_write32(0x80000000U, DEVICE_MMIO_GET(dev) + PHYADJ1_OFF);
	sys_write32(0x00008080U, DEVICE_MMIO_GET(dev) + PHYADJ2_OFF);
	sys_write32(0x80000022U, DEVICE_MMIO_GET(dev) + PHYADJ1_OFF);
	sys_write32(0x00008080U, DEVICE_MMIO_GET(dev) + PHYADJ2_OFF);
	sys_write32(0x80000024U, DEVICE_MMIO_GET(dev) + PHYADJ1_OFF);
	sys_write32(0x00000000U, DEVICE_MMIO_GET(dev) + PHYADJ2_OFF);
	sys_write32(0x80000032U, DEVICE_MMIO_GET(dev) + PHYADJ1_OFF);
}

static void mmap_mode(const struct device *dev)
{
	uint32_t cmncr;
	uint32_t value;

	if (spi_reg_read(dev, CMNCR_OFF, SPI_CMNCR_MD_SHIFT, SPI_CMNCR_MD) == SPI_CMNCR_MD_EXTRD) {
		return;
	}

	spi_stop(dev);

	/* Flush SPIBSC's read cache */
	spi_reg_write(dev, DRCR_OFF, SPI_DRCR_RCF_EXE, SPI_DRCR_RCF_SHIFT, SPI_DRCR_RCF);
	/* Dummy read */
	sys_read32(DEVICE_MMIO_GET(dev) + DRCR_OFF);

	cmncr = 0;
	/* External address space mode */
	cmncr = (cmncr & (~SPI_CMNCR_MD)) | (SPI_CMNCR_MD_EXTRD << SPI_CMNCR_MD_SHIFT);

	/* Only one connected serial flash is supported */
	cmncr = (cmncr & (~SPI_CMNCR_BSZ)) | (SPI_CMNCR_BSZ_SINGLE << SPI_CMNCR_BSZ_SHIFT);
	cmncr = (cmncr & (~SPI_CMNCR_IO0FV)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_IO0FV_SHIFT);
	cmncr = (cmncr & (~SPI_CMNCR_IO2FV)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_IO2FV_SHIFT);
	cmncr = (cmncr & (~SPI_CMNCR_IO3FV)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_IO3FV_SHIFT);
	/* MOIIO3 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO3)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_MOIIO3_SHIFT);
	/* MOIIO2 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO2)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_MOIIO2_SHIFT);
	/* MOIIO1 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO1)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_MOIIO1_SHIFT);
	/* MOIIO0 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO0)) | (SPI_CMNCR_IO_KEEP << SPI_CMNCR_MOIIO0_SHIFT);
	/* Set reserved bit to 1 */
	cmncr |= (1 << SPI_CMNCR_RESERV_SHIFT);
	/* Dummy read */
	sys_read32(DEVICE_MMIO_GET(dev) + CMNCR_OFF);

	sys_write32(cmncr, DEVICE_MMIO_GET(dev) + CMNCR_OFF);

	/* Dummy read */
	sys_read32(DEVICE_MMIO_GET(dev) + CMNCR_OFF);

	/* Set DRCR */
	value = 0;

	/* Enable Read Burst */
	value = (value & (~SPI_DRCR_RBE)) | (SPI_BURST_ENABLE << SPI_DRCR_RBE_SHIFT);

	/* Set 3 continuous data units in read burst */
	value = (value & (~SPI_DRCR_RBURST)) | (SPI_BURST_3 << SPI_DRCR_RBURST_SHIFT);
	sys_write32(value, DEVICE_MMIO_GET(dev) + DRCR_OFF); /* Write to register */

	/* Set DRCMR */
	spi_reg_write(dev, DRCMR_OFF, SPI_NOR_CMD_READ, SPI_DRCMR_CMD_SHIFT, SPI_DRCMR_CMD);

	/* Clean DREAR */
	sys_write32(0, DEVICE_MMIO_GET(dev) + DREAR_OFF);
	/* Clear DROPR */
	sys_write32(0, DEVICE_MMIO_GET(dev) + DROPR_OFF);

	/* Set DRENR */
	value = 0;
	value = (value & (~SPI_DRENR_ADE)) |
		(0b0111 << SPI_DRENR_ADE_SHIFT); /* 0b0111 = SPI_OUTPUT_ADDR_24*/
	value = (value & (~SPI_DRENR_DME)) | (SPI_DUMMY_CYC_DISABLE << SPI_DRENR_DME_SHIFT);
	value = (value & (~SPI_DRENR_CDE)) | (SPI_COMMAND_ENABLE << SPI_DRENR_CDE_SHIFT);
	sys_write32(value, DEVICE_MMIO_GET(dev) + DRENR_OFF);

	/* Clear DRDMCR */
	sys_write32(0, DEVICE_MMIO_GET(dev) + DRDMCR_OFF);

	/* Clear DRDRENR */
	sys_write32(0, DEVICE_MMIO_GET(dev) + DRDRENR_OFF);

	/* Clean PHYCNT. Bits 5, 6 and 9 should be 1 according to HW manual */
	spi_reg_write(dev, PHYCNT_OFF, 0x00000260U, 0U, 0xffffffffU);

	/* Set register initial value */
	value = SPI_PHYOFFSET1_INITIAL_VALUE;
	/* Set DDRTMG to PHYOFFSET1 */
	value |= (SPI_PHYOFFSET1_SDR << SPI_PHYOFFSET1_DDRTMG_SHIFT);
	sys_write32(value, DEVICE_MMIO_GET(dev) + PHYOFFSET1_OFF);

	value = SPI_PHYOFFSET2_INITIAL_VALUE;
	value |= (4 << SPI_PHYOFFSET2_OCTTMG_SHIFT);
	sys_write32(value, DEVICE_MMIO_GET(dev) + PHYOFFSET2_OFF);

	value = SPI_PHYINT_INITIAL_VALUE;
	value |= SPI_PHYINT_WPVAL | SPI_PHYINT_INTEN | SPI_PHYINT_WPEN | SPI_PHYINT_RSTEN;
	sys_write32(value, DEVICE_MMIO_GET(dev) + PHYINT_OFF);

	/* Set timing Adjustment */
	spi_timing_adjustment(dev);
}

static int flash_rza2m_write_enable(const struct device *dev)
{
	int ret;
	struct spibsc_reg spimd_reg;

	clear_spi_reg(&spimd_reg);

	spimd_reg.smenr_cde = SPI_OUTPUT_ENABLE;
	spimd_reg.smenr_cdb = SPI_1BIT;
	spimd_reg.smcmr_cmd = SPI_NOR_CMD_WREN;

	ret = spi_xfer(dev, &spimd_reg);
	if (ret < 0) {
		LOG_ERR("Failed to send Write Enable Command");
	}

	return ret;
}

static void flash_rza2m_manual_mode(const struct device *dev)
{
	uint32_t cmncr;

	/* Select SDR mode */
	spi_reg_write(dev, PHYCNT_OFF, SPI_PHYMEM_SDR, SPI_PHYCNT_PHYMEM_SHIFT, SPI_PHYCNT_PHYMEM);
	/* Set phyoffset SDR mode */
	spi_reg_write(dev, PHYOFFSET1_OFF, SPI_PHYOFFSET1_SDR, SPI_PHYOFFSET1_DDRTMG_SHIFT,
		      SPI_PHYOFFSET1_DDRTMG);

	spi_reg_write(dev, SMDMCR_OFF, 0, SPI_SMDMCR_DMCYC_SHIFT, SPI_SMDMCR_DMCYC);

	/* Set CMNCR */
	cmncr = sys_read32(DEVICE_MMIO_GET(dev) + CMNCR_OFF);
	cmncr = (cmncr & (~SPI_CMNCR_MD)) | (SPI_CMNCR_MD_SPI << SPI_CMNCR_MD_SHIFT);
	/* MOIIO3 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO3)) | (SPI_CMNCR_IO_HIGH << SPI_CMNCR_MOIIO3_SHIFT);
	/* MOIIO2 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO2)) | (SPI_CMNCR_IO_HIGH << SPI_CMNCR_MOIIO2_SHIFT);
	/* MOIIO1 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO1)) | (SPI_CMNCR_IO_HIGH << SPI_CMNCR_MOIIO1_SHIFT);
	/* MOIIO0 */
	cmncr = (cmncr & (~SPI_CMNCR_MOIIO0)) | (SPI_CMNCR_IO_HIGH << SPI_CMNCR_MOIIO0_SHIFT);
	/* SET BSZ to single */
	cmncr = (cmncr & (~SPI_CMNCR_BSZ)) | (SPI_CMNCR_BSZ_SINGLE << SPI_CMNCR_BSZ_SHIFT);

	/* Dummy read */
	sys_read32(DEVICE_MMIO_GET(dev) + CMNCR_OFF);

	/* SPI Mode */
	sys_write32(cmncr, DEVICE_MMIO_GET(dev) + CMNCR_OFF);

	sys_write32(SPI_SSLDR_DEFAULT, DEVICE_MMIO_GET(dev) + SSLDR_OFF);

	sys_read32(DEVICE_MMIO_GET(dev) + CMNCR_OFF);
}

static void spi_mode(const struct device *dev)
{
	if (spi_reg_read(dev, CMNCR_OFF, SPI_CMNCR_MD_SHIFT, SPI_CMNCR_MD) == SPI_CMNCR_MD_SPI) {
		return;
	}

	spi_stop(dev);

	/* Dummy read */
	sys_read32(DEVICE_MMIO_GET(dev) + CMNCR_OFF);

	/* Set Manual mode */
	flash_rza2m_manual_mode(dev);

	/* Set timing Adjustment */
	spi_timing_adjustment(dev);
}

static int data_send(const struct device *dev, uint32_t bit_width, uint32_t spbssl_level,
		     const uint8_t *buf, int size)
{
	int ret = 0;
	int32_t unit;
	const uint8_t *buf_b;
	const uint16_t *buf_s;
	const uint32_t *buf_l;
	struct spibsc_reg spimd_reg;

	clear_spi_reg(&spimd_reg);

	spimd_reg.smcr_sslkp = SPI_SPISSL_KEEP;    /* SPBSSL level */
	spimd_reg.smcr_spiwe = SPI_SPIDATA_ENABLE; /* Write enable */

	spimd_reg.smenr_spidb = bit_width;
	spimd_reg.smdrenr_spidre = SPI_SDR_TRANS;

	if (((uint32_t)size & 0x3) == 0) {
		spimd_reg.smenr_spide = SPI_OUTPUT_SPID_32; /* Enable 32bit */
		unit = 4;
	} else if (((uint32_t)size & 0x1) == 0) {
		spimd_reg.smenr_spide = SPI_OUTPUT_SPID_16; /* Enable 16bit */
		unit = 2;
	} else {
		spimd_reg.smenr_spide = SPI_OUTPUT_SPID_8; /* Enable 8bit */
		unit = 1;
	}

	while (size > 0) {
		if (unit == 1) {
			buf_b = (const uint8_t *)buf;
			spimd_reg.smwdr[0] = *buf_b;
		} else if (unit == 2) {
			buf_s = (const uint16_t *)buf;
			spimd_reg.smwdr[0] = *buf_s;
		} else {
			buf_l = (const uint32_t *)buf;
			spimd_reg.smwdr[0] = *buf_l;
		}

		buf += unit;
		size -= unit;

		if (size <= 0) {
			spimd_reg.smcr_sslkp = spbssl_level;
		}

		ret = spi_xfer(dev, &spimd_reg); /* Data */
		if (ret < 0) {
			LOG_ERR("Failed to send data to flash");
			return ret;
		}
	}

	return ret;
}

int flash_rza2m_page_program(const struct device *dev, uint32_t offset, const uint8_t *buf,
			     uint32_t size)
{
	int ret = 0;
	int32_t program_size;
	int32_t remainder;
	int32_t idx = 0;
	struct spibsc_reg spimd_reg;

	while (size > 0) {
		program_size = MIN(size, QSPI_MAX_BUFFER_SIZE);
		remainder = QSPI_MAX_BUFFER_SIZE - (offset % QSPI_MAX_BUFFER_SIZE);

		if ((remainder != 0) && (program_size > remainder)) {
			program_size = remainder;
		}

		memcpy(write_tmp_buf, &buf[idx], program_size);

		sys_cache_data_flush_range((void *)write_tmp_buf, program_size);

		ret = flash_rza2m_write_enable(dev);
		if (ret < 0) {
			return ret;
		}

		clear_spi_reg(&spimd_reg);

		spimd_reg.smenr_cde = SPI_OUTPUT_ENABLE;
		spimd_reg.smenr_cdb = SPI_1BIT;
		spimd_reg.smcmr_cmd = SPI_NOR_CMD_PP_4B;

		spimd_reg.smenr_ade = SPI_OUTPUT_ADDR_32;
		spimd_reg.smdrenr_addre = SPI_SDR_TRANS;
		spimd_reg.smenr_adb = SPI_1BIT;
		spimd_reg.smadr_addr = offset;

		spimd_reg.smcr_sslkp = SPI_SPISSL_KEEP;

		ret = spi_xfer(dev, &spimd_reg);
		if (ret < 0) {
			LOG_ERR("Failed to send Program Page Command");
			return ret;
		}

		ret = data_send(dev, SPI_1BIT, SPI_SPISSL_NEGATE, write_tmp_buf, program_size);
		if (ret < 0) {
			LOG_ERR("Failed to send data to flash");
			return ret;
		}

		ret = wait_status_from_flash(dev);
		if (ret < 0) {
			return ret;
		}

		offset += program_size;
		idx += program_size;
		size -= program_size;
	}

	return ret;
}

static bool is_valid_range(const struct device *dev, off_t offset, uint32_t size)
{
	const struct flash_rza2m_config *config = dev->config;

	return (offset >= 0) && ((offset + size) <= config->flash_size);
}

static int flash_rza2m_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	const struct flash_rza2m_config *config = dev->config;
	uint8_t *virt;
	size_t aligned_size = MAX(size, SPI_NOR_PAGE_SIZE);

	if (config->type != SERIAL_FLASH) {
		LOG_ERR("%s: Only Serial Flash is supported", __func__);
		return -ENOTSUP;
	}

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(dev, offset, size)) {
		LOG_ERR("Range exceeds the flash boundaries. Offset=%#lx, Size=%u", offset, size);
		return -EINVAL;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	mmap_mode(dev);

	k_mem_map_phys_bare(&virt, (CONFIG_FLASH_BASE_ADDRESS + offset), aligned_size,
			    K_MEM_CACHE_NONE);

	memcpy(data, virt, size);

	k_mem_unmap_phys_bare(virt, aligned_size);

	spi_mode(dev);

	k_mutex_unlock(&lock);

	return 0;
}

static int flash_rza2m_write(const struct device *dev, off_t offset, const void *data, size_t size)
{
	int ret;
	uint32_t key;
	const struct flash_rza2m_config *config = dev->config;

	if (config->type != SERIAL_FLASH) {
		LOG_ERR("%s: Only Serial Flash is supported", __func__);
		return -ENOTSUP;
	}

	if (size == 0) {
		LOG_DBG("%s: Skip writing 0 length buffer", __func__);
		return 0;
	}

	if (!data) {
		return -EINVAL;
	}

	if (!is_valid_range(dev, offset, size)) {
		LOG_ERR("Range exceeds the flash boundaries. Offset=%#lx, Size=%u", offset, size);
		return -EINVAL;
	}

	key = irq_lock();

	ret = flash_rza2m_page_program(dev, offset, data, size);

	irq_unlock(key);

	return ret;
}

static int sector_erase_serial(const struct device *dev, uint32_t offset)
{
	int ret;
	struct spibsc_reg spimd_reg;

	ret = flash_rza2m_write_enable(dev); /* WREN Command */
	if (ret < 0) {
		return ret;
	}

	clear_spi_reg(&spimd_reg);

	spimd_reg.smenr_cde = SPI_OUTPUT_ENABLE;
	spimd_reg.smenr_cdb = SPI_1BIT;
	spimd_reg.smcmr_cmd = SPI_NOR_CMD_SE_4B;

	spimd_reg.smenr_ade = SPI_OUTPUT_ADDR_32;
	spimd_reg.smdrenr_addre = SPI_SDR_TRANS;
	spimd_reg.smenr_adb = SPI_1BIT;
	spimd_reg.smadr_addr = offset;

	ret = spi_xfer(dev, &spimd_reg);
	if (ret < 0) {
		LOG_ERR("Failed to send Sector Erase Command");
		return ret;
	}

	return wait_status_from_flash(dev);
}

static int range_erase_serial(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_rza2m_config *config = dev->config;
	int ret;
	uint32_t offt = offset;

	/* This function expects valid offset and size values */
	do {
		ret = sector_erase_serial(dev, offt);
		if (ret) {
			LOG_ERR("%s: Unable to clear sector on addr: %x", __func__, offt);
			return ret;
		}

		offt += config->erase_block_size;
	} while (offt < offset + size);

	return 0;
}

static int flash_rza2m_erase(const struct device *dev, off_t offset, size_t size)
{
	int ret;
	uint32_t key;
	const struct flash_rza2m_config *config = dev->config;

	if (config->type != SERIAL_FLASH) {
		LOG_ERR("%s: Only Serial Flash is supported", __func__);
		return -ENOTSUP;
	}

	if (size % config->erase_block_size) {
		LOG_ERR("%s: erase size isn't aligned to the sector size", __func__);
		return -EINVAL;
	}

	if (offset % config->erase_block_size) {
		LOG_ERR("%s: offset isn't aligned to the sector size", __func__);
		return -EINVAL;
	}

	if (size == 0) {
		LOG_DBG("%s: Skip writing 0 length buffer", __func__);
		return 0;
	}

	if (!is_valid_range(dev, offset, size)) {
		LOG_ERR("Erase range exceeds the flash boundaries. Offset=%#lx, Size=%u", offset,
			size);
		return -EINVAL;
	}

	key = irq_lock();

	ret = range_erase_serial(dev, offset, size);

	irq_unlock(key);

	return ret;
}

#if defined(CONFIG_FLASH_JESD216_API)
static int flash_rza2m_read_jedec_id(const struct device *dev, uint8_t *id)
{
	int ret;
	struct spibsc_reg spimd_reg;

	if (id == NULL) {
		return -EINVAL;
	}

	ret = flash_rza2m_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	clear_spi_reg(&spimd_reg);

	spimd_reg.smenr_cde = SPI_OUTPUT_ENABLE;
	spimd_reg.smenr_cdb = SPI_1BIT;
	spimd_reg.smcmr_cmd = SPI_NOR_CMD_RDID;

	spimd_reg.smcr_sslkp = SPI_SPISSL_NEGATE;  /* SPBSSL level */
	spimd_reg.smcr_spire = SPI_SPIDATA_ENABLE; /* Read enable */

	spimd_reg.smenr_spide = SPI_OUTPUT_SPID_32;
	spimd_reg.smdrenr_spidre = SPI_SDR_TRANS;
	spimd_reg.smenr_spidb = SPI_1BIT;

	spimd_reg.smwdr[0] = 0x00; /* Output 0 in read status */

	ret = spi_xfer(dev, &spimd_reg);
	if (ret == 0) {
		/* Get flash device ID */
		memcpy(id, &spimd_reg.smrdr[0], SPI_NOR_MAX_ID_LEN);
	}

	return ret;
}

static int flash_rza2m_sfdp_read(const struct device *dev, off_t addr, void *data, size_t len)
{
	int ret = 0;
	size_t size;
	struct spibsc_reg spimd_reg;

	while (len > 0) {

		ret = flash_rza2m_write_enable(dev);
		if (ret < 0) {
			return ret;
		}

		size = MIN(len, FOUR_BYTE);
		clear_spi_reg(&spimd_reg);

		spimd_reg.smenr_cde = SPI_OUTPUT_ENABLE;
		spimd_reg.smenr_cdb = SPI_1BIT;
		spimd_reg.smcmr_cmd = QSPI_CMD_RDSFDP;

		spimd_reg.smcr_sslkp = SPI_SPISSL_NEGATE;  /* SPBSSL level */
		spimd_reg.smcr_spire = SPI_SPIDATA_ENABLE; /* Read enable */

		spimd_reg.smenr_ade = SPI_OUTPUT_ADDR_24; /* 3-bytes address */

		spimd_reg.smenr_spide = SPI_OUTPUT_SPID_32;
		spimd_reg.smdrenr_spidre = SPI_SDR_TRANS;
		spimd_reg.smenr_spidb = SPI_1BIT;

		spimd_reg.smadr_addr = addr;

		spimd_reg.smenr_dme = SPI_DUMMY_CYC_ENABLE;
		spimd_reg.smdmcr_dmcyc = SPI_DUMMY_8CYC;

		spimd_reg.smwdr[0] = 0x00; /* Output 0 in read status */
		ret = spi_xfer(dev, &spimd_reg);

		if (ret < 0) {
			LOG_INF("Failed to transfer command");
			return ret;
		}

		memcpy((uint8_t *)data, &spimd_reg.smrdr[0], size);

		len -= size;
		addr += size;
		data = (uint8_t *)data + size;
	}

	return ret;
}
#endif

static const struct flash_parameters *flash_rza2m_get_parameters(const struct device *dev)
{
	const struct flash_rza2m_config *config = dev->config;

	return &config->flash_param;
}

static int flash_rza2m_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_rza2m_config *config = dev->config;

	*size = (uint64_t)config->flash_size;

	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_rza2m_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	const struct flash_rza2m_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_renesas_rz_qspi_driver_api) = {
	.read = flash_rza2m_read,
	.write = flash_rza2m_write,
	.erase = flash_rza2m_erase,
	.get_parameters = flash_rza2m_get_parameters,
	.get_size = flash_rza2m_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rza2m_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_rza2m_sfdp_read,
	.read_jedec_id = flash_rza2m_read_jedec_id,
#endif
};

static int flash_rza2m_init(const struct device *dev)
{
	const struct flash_rza2m_config *config = dev->config;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: unable to apply pinctrl configuration with code: %d", __func__, ret);
		return -EINVAL;
	}

	spi_mode(dev);
	return 0;
}

#define FLASH_RENESAS_RZA2M_QSPI_SPIBSC_DEFINE(n)                                                  \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(n));                                                      \
	uint32_t clock_subsys_spibsc##n = DT_CLOCKS_CELL(DT_INST_PARENT(n), clk_id);               \
	static const struct flash_rza2m_config flash_renesas_rz_config_##n = {                     \
		DEVICE_MMIO_ROM_INIT(DT_INST_PARENT(n)),                                           \
		.type = SERIAL_FLASH,                                                              \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(n)),                              \
		.flash_size = DT_INST_REG_SIZE(n),                                                 \
		.erase_block_size = DT_INST_PROP_OR(n, erase_block_size, 4096),                    \
		.flash_param =                                                                     \
			{                                                                          \
				.write_block_size = DT_INST_PROP(n, write_block_size),             \
				.erase_value = ERASE_VALUE,                                        \
			},                                                                         \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,	\
		(.layout = {                                                                       \
			.pages_count =                                                             \
				DT_INST_REG_SIZE(n) / DT_INST_PROP_OR(n, erase_block_size, 4096),  \
			.pages_size = DT_INST_PROP_OR(n, erase_block_size, 4096),                  \
		},))};               \
	static struct flash_rza2m_data flash_renesas_rz_data_##n;                                  \
	DEVICE_DT_INST_DEFINE(n, flash_rza2m_init, NULL, &flash_renesas_rz_data_##n,               \
			      &flash_renesas_rz_config_##n, POST_KERNEL,                           \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_renesas_rz_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_RENESAS_RZA2M_QSPI_SPIBSC_DEFINE)
