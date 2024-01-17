/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include "sdhc_cdns_ll.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdhc_cdns_ll, CONFIG_SDHC_LOG_LEVEL);

/* card busy and present */
#define CARD_BUSY			1
#define CARD_NOT_BUSY			0
#define CARD_PRESENT			1

/* SRS12 error mask */
#define CDNS_SRS12_ERR_MASK			0xFFFF8000U
#define CDNS_CSD_BYTE_MASK			0x000000FFU

/* General define */
#define SDHC_REG_MASK				0xFFFFFFFFU
#define SD_HOST_BLOCK_SIZE			0x200

#define SDMMC_DMA_MAX_BUFFER_SIZE		(64 * 1024)
#define CDNSMMC_ADDRESS_MASK			(CONFIG_SDHC_BUFFER_ALIGNMENT - 1)

#define SRS10_VAL_READ	(ADMA2_32 | HS_EN | DT_WIDTH)
#define SRS10_VAL_SW	(ADMA2_32 | DT_WIDTH)
#define SRS11_VAL_GEN	(READ_CLK | CDNS_SRS11_ICE | CDNS_SRS11_ICS | CDNS_SRS11_SDCE)
#define SRS11_VAL_CID	(CDNS_SRS11_ICE | CDNS_SRS11_ICS | CDNS_SRS11_SDCE)
#define SRS15_VAL_GEN	(CDNS_SRS15_BIT_AD_64 | CDNS_SRS15_HV4E | CDNS_SRS15_V18SE)
#define SRS15_VAL_RD_WR	(SRS15_VAL_GEN | CDNS_SRS15_SDR104 | CDNS_SRS15_PVE)
#define SRS15_VAL_CID	(CDNS_SRS15_HV4E | CDNS_SRS15_V18SE)

#define CARD_REG_TIME_DELAY_US			100000
#define WAIT_ICS_TIME_DELAY_US			5000
#define RESET_SRS14				0x00000000

static struct sdhc_cdns_params cdns_params;
static struct sdhc_cdns_combo_phy sdhc_cdns_combo_phy_reg_info;
static struct sdhc_cdns_sdmmc sdhc_cdns_sdmmc_reg_info;

/* Function to write general phy registers */
static int sdhc_cdns_write_phy_reg(uint32_t phy_reg_addr, uint32_t phy_reg_addr_value,
			uint32_t phy_reg_data, uint32_t phy_reg_data_value)
{
	uint32_t data = 0;

	/* Set PHY register address, write HRS04*/
	sys_write32(phy_reg_addr_value, phy_reg_addr);

	/* Set PHY register data, write HRS05 */
	sys_write32(phy_reg_data_value, phy_reg_data);
	data = sys_read32(phy_reg_data);

	if (data != phy_reg_data_value) {
		LOG_ERR("PHY_REG_DATA is not set properly");
		return -ENXIO;
	}

	return 0;
}

int sdhc_cdns_wait_ics(uint16_t timeout, uint32_t cdn_srs_res)
{
	/* Wait status command response ready */
	if (!WAIT_FOR(((sys_read32(cdn_srs_res) & CDNS_SRS11_ICS)
		== CDNS_SRS11_ICS), timeout, k_msleep(1))) {
		LOG_ERR("Timed out waiting for ICS response");
		return -ETIMEDOUT;
	}

	return 0;
}

static int sdhc_cdns_busy(void)
{
	unsigned int data;

	data = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS09);
	return (data & CDNS_SRS09_STAT_DAT_BUSY) ? CARD_BUSY : CARD_NOT_BUSY;
}

static int sdhc_cdns_card_present(void)
{
	uint32_t timeout = CARD_REG_TIME_DELAY_US;

	if (!WAIT_FOR((((sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS09))
			& CDNS_SRS09_CI) == CDNS_SRS09_CI),
			timeout, k_msleep(1))) {
		LOG_ERR("Card detection timeout");
		return -ETIMEDOUT;
	}

	return CARD_PRESENT;
}

static int sdhc_cdns_vol_reset(void)
{
	/* Reset embedded card, turn off supply voltage */
	sys_write32(BUS_VOLTAGE_3_3_V,
		(cdns_params.reg_base + SDHC_CDNS_SRS10));

	/*
	 * Turn on supply voltage
	 * CDNS_SRS10_BVS = 7, CDNS_SRS10_BP = 1, BP2 only in UHS2 mode
	 */
	sys_write32(BUS_VOLTAGE_3_3_V | CDNS_SRS10_BP,
		(cdns_params.reg_base + SDHC_CDNS_SRS10));

	return 0;
}

/*
 * Values are taken from IP documents and calc_setting.py script
 * with input value- mode sd_ds,
 * sdmclk 5000,
 * sdclk 10000,
 * iocell_input_delay 2500,
 * iocell_output_delay 2500 and
 * delay_element 24
 */
void cdns_sdhc_set_sdmmc_params(struct sdhc_cdns_combo_phy *sdhc_cdns_combo_phy_reg,
	struct sdhc_cdns_sdmmc *sdhc_cdns_sdmmc_reg)
{
	/* Values are taken by the reference of cadence IP documents */
	sdhc_cdns_combo_phy_reg->cp_clk_wr_delay = 0;
	sdhc_cdns_combo_phy_reg->cp_clk_wrdqs_delay = 0;
	sdhc_cdns_combo_phy_reg->cp_data_select_oe_end = 1;
	sdhc_cdns_combo_phy_reg->cp_dll_bypass_mode = 1;
	sdhc_cdns_combo_phy_reg->cp_dll_locked_mode = 3;
	sdhc_cdns_combo_phy_reg->cp_dll_start_point = 4;
	sdhc_cdns_combo_phy_reg->cp_gate_cfg_always_on = 1;
	sdhc_cdns_combo_phy_reg->cp_io_mask_always_on = 0;
	sdhc_cdns_combo_phy_reg->cp_io_mask_end = 2;
	sdhc_cdns_combo_phy_reg->cp_io_mask_start = 0;
	sdhc_cdns_combo_phy_reg->cp_rd_del_sel = 52;
	sdhc_cdns_combo_phy_reg->cp_read_dqs_cmd_delay = 0;
	sdhc_cdns_combo_phy_reg->cp_read_dqs_delay = 0;
	sdhc_cdns_combo_phy_reg->cp_sw_half_cycle_shift = 0;
	sdhc_cdns_combo_phy_reg->cp_sync_method = 1;
	sdhc_cdns_combo_phy_reg->cp_underrun_suppress = 1;
	sdhc_cdns_combo_phy_reg->cp_use_ext_lpbk_dqs = 1;
	sdhc_cdns_combo_phy_reg->cp_use_lpbk_dqs = 1;
	sdhc_cdns_combo_phy_reg->cp_use_phony_dqs = 1;
	sdhc_cdns_combo_phy_reg->cp_use_phony_dqs_cmd = 1;

	sdhc_cdns_sdmmc_reg->sdhc_extended_rd_mode = 1;
	sdhc_cdns_sdmmc_reg->sdhc_extended_wr_mode = 1;
	sdhc_cdns_sdmmc_reg->sdhc_hcsdclkadj = 6;
	sdhc_cdns_sdmmc_reg->sdhc_idelay_val = 1;
	sdhc_cdns_sdmmc_reg->sdhc_rdcmd_en = 1;
	sdhc_cdns_sdmmc_reg->sdhc_rddata_en = 1;
	sdhc_cdns_sdmmc_reg->sdhc_rw_compensate = 10;
	sdhc_cdns_sdmmc_reg->sdhc_sdcfsh = 0;
	sdhc_cdns_sdmmc_reg->sdhc_sdcfsl = 1;
	sdhc_cdns_sdmmc_reg->sdhc_wrcmd0_dly = 1;
	sdhc_cdns_sdmmc_reg->sdhc_wrcmd0_sdclk_dly = 0;
	sdhc_cdns_sdmmc_reg->sdhc_wrcmd1_dly = 0;
	sdhc_cdns_sdmmc_reg->sdhc_wrcmd1_sdclk_dly = 0;
	sdhc_cdns_sdmmc_reg->sdhc_wrdata0_dly = 1;
	sdhc_cdns_sdmmc_reg->sdhc_wrdata0_sdclk_dly = 0;
	sdhc_cdns_sdmmc_reg->sdhc_wrdata1_dly = 0;
	sdhc_cdns_sdmmc_reg->sdhc_wrdata1_sdclk_dly = 0;
}

/* Phy register programing for phy init */
static int sdhc_cdns_program_phy_reg(struct sdhc_cdns_combo_phy *sdhc_cdns_combo_phy_reg,
	struct sdhc_cdns_sdmmc *sdhc_cdns_sdmmc_reg)
{
	uint32_t value = 0;
	int ret = 0;

	/*
	 * program PHY_DQS_TIMING_REG
	 * This register controls the DQS related timing
	 */
	value = (CP_USE_EXT_LPBK_DQS(sdhc_cdns_combo_phy_reg->cp_use_ext_lpbk_dqs))
		| (CP_USE_LPBK_DQS(sdhc_cdns_combo_phy_reg->cp_use_lpbk_dqs))
		| (CP_USE_PHONY_DQS(sdhc_cdns_combo_phy_reg->cp_use_phony_dqs))
		| (CP_USE_PHONY_DQS_CMD(sdhc_cdns_combo_phy_reg->cp_use_phony_dqs_cmd));
	ret = sdhc_cdns_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DQS_TIMING_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		LOG_ERR("Error in PHY_DQS_TIMING_REG programming");
		return ret;
	}

	/*
	 * program PHY_GATE_LPBK_CTRL_REG
	 * This register controls the gate and loopback control related timing.
	 */
	value = (CP_SYNC_METHOD(sdhc_cdns_combo_phy_reg->cp_sync_method))
		| (CP_SW_HALF_CYCLE_SHIFT(sdhc_cdns_combo_phy_reg->cp_sw_half_cycle_shift))
		| (CP_RD_DEL_SEL(sdhc_cdns_combo_phy_reg->cp_rd_del_sel))
		| (CP_UNDERRUN_SUPPRESS(sdhc_cdns_combo_phy_reg->cp_underrun_suppress))
		| (CP_GATE_CFG_ALWAYS_ON(sdhc_cdns_combo_phy_reg->cp_gate_cfg_always_on));
	ret = sdhc_cdns_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_GATE_LPBK_CTRL_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		LOG_ERR("Error in PHY_GATE_LPBK_CTRL_REG programming");
		return -ret;
	}

	/*
	 * program PHY_DLL_MASTER_CTRL_REG
	 * This register holds the control for the Master DLL logic.
	 */
	value = (CP_DLL_BYPASS_MODE(sdhc_cdns_combo_phy_reg->cp_dll_bypass_mode))
			| (CP_DLL_START_POINT(sdhc_cdns_combo_phy_reg->cp_dll_start_point));
	ret = sdhc_cdns_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DLL_MASTER_CTRL_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		LOG_ERR("Error in PHY_DLL_MASTER_CTRL_REG programming");
		return ret;
	}

	/*
	 * program PHY_DLL_SLAVE_CTRL_REG
	 * This register holds the control for the slave DLL logic.
	 */
	value = (CP_READ_DQS_CMD_DELAY(sdhc_cdns_combo_phy_reg->cp_read_dqs_cmd_delay))
		| (CP_CLK_WRDQS_DELAY(sdhc_cdns_combo_phy_reg->cp_clk_wrdqs_delay))
		| (CP_CLK_WR_DELAY(sdhc_cdns_combo_phy_reg->cp_clk_wr_delay))
		| (CP_READ_DQS_DELAY(sdhc_cdns_combo_phy_reg->cp_read_dqs_delay));
	ret = sdhc_cdns_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DLL_SLAVE_CTRL_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		LOG_ERR("Error in PHY_DLL_SLAVE_CTRL_REG programming");
		return ret;
	}

	/*
	 * program PHY_CTRL_REG
	 * This register handles the global control settings for the PHY.
	 */
	sys_write32(cdns_params.combophy + PHY_CTRL_REG, cdns_params.reg_base
		+ SDHC_CDNS_HRS04);
	value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS05);

	/* phony_dqs_timing=0 */
	value &= ~(CP_PHONY_DQS_TIMING_MASK << CP_PHONY_DQS_TIMING_SHIFT);
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS05);

	/* switch off DLL_RESET */
	do {
		value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
		value |= CDNS_HRS09_PHY_SW_RESET;
		sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS09);
		value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
	/* polling PHY_INIT_COMPLETE */
	} while ((value & CDNS_HRS09_PHY_INIT_COMP) != CDNS_HRS09_PHY_INIT_COMP);

	/*
	 * program PHY_DQ_TIMING_REG
	 * This register controls the DQ related timing.
	 */
	sdhc_cdns_combo_phy_reg->cp_io_mask_end = 0U;
	value = (CP_IO_MASK_ALWAYS_ON(sdhc_cdns_combo_phy_reg->cp_io_mask_always_on))
		| (CP_IO_MASK_END(sdhc_cdns_combo_phy_reg->cp_io_mask_end))
		| (CP_IO_MASK_START(sdhc_cdns_combo_phy_reg->cp_io_mask_start))
		| (CP_DATA_SELECT_OE_END(sdhc_cdns_combo_phy_reg->cp_data_select_oe_end));

	ret = sdhc_cdns_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DQ_TIMING_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		LOG_ERR("Error in PHY_DQ_TIMING_REG programming");
		return ret;
	}
	return 0;
}

static int sdhc_cdns_cache_invd(int lba, uintptr_t buf, size_t size)
{
	int ret = 0;

	ret = arch_dcache_invd_range((void *)buf, size);
	if (ret != 0) {
		LOG_ERR("%s: error in invalidate dcache with ret %d", __func__, ret);
		return ret;
	}

	return 0;
}

/* DMA preparation for the read and write operation */
static int sdhc_cdns_prepare(uint32_t dma_start_addr, uintptr_t dma_buff,
	struct sdhc_data *data)
{
	struct sdhc_cdns_desc *desc;
	uint32_t desc_cnt, i;
	uintptr_t base;
	uint64_t desc_base;
	uint32_t size = data->blocks * data->block_size;

	__ASSERT_NO_MSG(((dma_buff & CDNSMMC_ADDRESS_MASK) == 0) &&
			(cdns_params.desc_size > 0) &&
			((cdns_params.desc_size & MMC_BLOCK_MASK) == 0));

	arch_dcache_flush_range((void *)dma_buff, size);

	desc_cnt = (size + (SDMMC_DMA_MAX_BUFFER_SIZE) - 1) /
		(SDMMC_DMA_MAX_BUFFER_SIZE);
	__ASSERT_NO_MSG(desc_cnt * sizeof(struct sdhc_cdns_desc) <
		cdns_params.desc_size);

	if (desc_cnt > CONFIG_CDNS_DESC_COUNT) {
		LOG_ERR("Requested data transfer length %u greater than configured length %u",
			size, (CONFIG_CDNS_DESC_COUNT * SDMMC_DMA_MAX_BUFFER_SIZE));
		return -EINVAL;
	}

	/*
	 * Creating descriptor as per the desc_count and linked list of
	 * descriptor for contiguous desc alignment
	 */
	base = cdns_params.reg_base;
	desc = (struct sdhc_cdns_desc *)cdns_params.desc_base;
	desc_base = (uint64_t)desc;
	i = 0;

	while ((i+1) < desc_cnt) {
		desc->attr = ADMA_DESC_ATTR_VALID | ADMA_DESC_TRANSFER_DATA;
		desc->reserved = 0;
		desc->len = MAX_64KB_PAGE;
		desc->addr_lo = (dma_buff & 0xffffffff) + (SDMMC_DMA_MAX_BUFFER_SIZE * i);
		desc->addr_hi = (dma_buff >> 32) & 0xffffffff;
		size -= SDMMC_DMA_MAX_BUFFER_SIZE;
		desc++;
		i++;
	}

	desc->attr = ADMA_DESC_ATTR_VALID | ADMA_DESC_TRANSFER_DATA |
			ADMA_DESC_ATTR_END;
	desc->reserved = 0;
	desc->len = size;
	desc->addr_lo = (dma_buff & 0xffffffff) + (SDMMC_DMA_MAX_BUFFER_SIZE * i);
	desc->addr_hi = (dma_buff >> 32) & 0xffffffff;

	sys_write32((uint32_t)desc_base, cdns_params.reg_base + SDHC_CDNS_SRS22);
	sys_write32((uint32_t)(desc_base >> 32), cdns_params.reg_base + SDHC_CDNS_SRS23);
	arch_dcache_flush_range((void *)cdns_params.desc_base,
				desc_cnt * sizeof(struct sdhc_cdns_desc));

	sys_write32((data->block_size << CDNS_SRS01_BLK_SIZE |
				data->blocks << CDNS_SRS01_BLK_COUNT_CT |
				BUFFER_BOUNDARY_512K << CDNS_SRS01_SDMA_BUF),
				cdns_params.reg_base + SDHC_CDNS_SRS01);

	return 0;
}

static int sdhc_cdns_host_set_clk(int clk)
{
	uint32_t sdclkfsval = 0;
	uint32_t dtcvval = 0xe;
	int ret = 0;

	sdclkfsval = (cdns_params.clk_rate / 2000) / clk;
	sys_write32(0, cdns_params.reg_base + SDHC_CDNS_SRS11);
	sys_write32(((dtcvval << CDNS_SRS11_DTCV) | (sdclkfsval << CDNS_SRS11_SDCLKFS) |
		CDNS_SRS11_ICE), cdns_params.reg_base + SDHC_CDNS_SRS11);

	ret = sdhc_cdns_wait_ics(WAIT_ICS_TIME_DELAY_US, cdns_params.reg_base + SDHC_CDNS_SRS11);
	if (ret != 0) {
		return ret;
	}

	/* Enable DLL reset */
	sys_clear_bit(cdns_params.reg_base + SDHC_CDNS_HRS09, 0);
	/* Set extended_wr_mode */
	sys_write32(((sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09)
		& 0xFFFFFFF7) | CDNS_HRS09_EXT_WR_MODE), (cdns_params.reg_base
		+ SDHC_CDNS_HRS09));
	/* Release DLL reset */
	sys_set_bits(cdns_params.reg_base + SDHC_CDNS_HRS09, CDNS_HRS09_RDCMD_EN_BIT |
					CDNS_HRS09_RDDATA_EN_BIT);

	sys_write32(((dtcvval << CDNS_SRS11_DTCV) | (sdclkfsval << CDNS_SRS11_SDCLKFS)
			| CDNS_SRS11_ICE | CDNS_SRS11_SDCE),
			cdns_params.reg_base + SDHC_CDNS_SRS11);

	sys_write32(0xFFFFFFFF, cdns_params.reg_base + SDHC_CDNS_SRS13);

	return 0;
}

static int sdhc_cdns_set_ios(unsigned int clk, unsigned int width)
{
	int ret = 0;

	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		sys_clear_bit(cdns_params.reg_base + SDHC_CDNS_SRS10, WIDTH_BIT1);
		break;
	case SDHC_BUS_WIDTH4BIT:
		sys_set_bit(cdns_params.reg_base + SDHC_CDNS_SRS10, WIDTH_BIT4);
		break;
	case SDHC_BUS_WIDTH8BIT:
		sys_set_bit(cdns_params.reg_base + SDHC_CDNS_SRS10, WIDTH_BIT8);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}

	/* Perform clock configuration when SD clock is not gated */
	if (clk != 0) {
		ret = sdhc_cdns_host_set_clk(clk);
		if (ret != 0) {
			LOG_ERR("%s: Clock configuration failed", __func__);
			return ret;
		}
	}

	return 0;
}

/* Programming HRS register for initialisation */
static int sdhc_cdns_init_hrs_io(struct sdhc_cdns_combo_phy *sdhc_cdns_combo_phy_reg,
	struct sdhc_cdns_sdmmc *sdhc_cdns_sdmmc_reg)
{
	uint32_t value = 0;
	int ret = 0;

	/*
	 * program HRS09, register 42
	 * PHY Control and Status Register
	 */
	value = (CDNS_HRS09_RDDATA_EN(sdhc_cdns_sdmmc_reg->sdhc_rddata_en))
		| (CDNS_HRS09_RDCMD_EN(sdhc_cdns_sdmmc_reg->sdhc_rdcmd_en))
		| (CDNS_HRS09_EXTENDED_WR(sdhc_cdns_sdmmc_reg->sdhc_extended_wr_mode))
		| (CDNS_HRS09_EXT_RD_MODE(sdhc_cdns_sdmmc_reg->sdhc_extended_rd_mode));
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS09);

	/*
	 * program HRS10, register 43
	 * Host Controller SDCLK start point adjustment
	 */
	value = (SDHC_HRS10_HCSDCLKADJ(sdhc_cdns_sdmmc_reg->sdhc_hcsdclkadj));
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS10);

	/*
	 * program HRS16, register 48
	 * CMD/DAT output delay
	 */
	value = (CDNS_HRS16_WRDATA1_SDCLK_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrdata1_sdclk_dly))
		| (CDNS_HRS16_WRDATA0_SDCLK_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrdata0_sdclk_dly))
		| (CDNS_HRS16_WRCMD1_SDCLK_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrcmd1_sdclk_dly))
		| (CDNS_HRS16_WRCMD0_SDCLK_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrcmd0_sdclk_dly))
		| (CDNS_HRS16_WRDATA1_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrdata1_dly))
		| (CDNS_HRS16_WRDATA0_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrdata0_dly))
		| (CDNS_HRS16_WRCMD1_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrcmd1_dly))
		| (CDNS_HRS16_WRCMD0_DLY(sdhc_cdns_sdmmc_reg->sdhc_wrcmd0_dly));
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS16);

	/*
	 * program HRS07, register 40
	 * IO Delay Information Register
	 */
	value = (CDNS_HRS07_RW_COMPENSATE(sdhc_cdns_sdmmc_reg->sdhc_rw_compensate))
		| (CDNS_HRS07_IDELAY_VAL(sdhc_cdns_sdmmc_reg->sdhc_idelay_val));
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS07);

	return ret;
}

static int sdhc_cdns_set_clk(struct sdhc_cdns_params *cdn_sdmmc_dev_type_params)
{
	uint32_t dtcvval, sdclkfsval;
	int ret = 0;

	dtcvval = DTC_VAL;
	sdclkfsval = 0;

	/* Condition for Default speed mode and SDR12 and SDR_BC */
	if ((cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_DS) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_UHS_SDR12) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_SDR_BC)) {
		sdclkfsval = 4;
	} else if ((cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_HS) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_UHS_SDR25) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_UHS_DDR50) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_SDR)) {
		sdclkfsval = 2;
	} else if ((cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_UHS_SDR50) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_DDR) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_HS400) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_HS400ES)) {
		sdclkfsval = 1;
	} else if ((cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_UHS_SDR104) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_HS200)) {
		sdclkfsval = 0;
	}

	/* Disabling SD clock enable */
	sys_write32(0, cdns_params.reg_base + SDHC_CDNS_SRS11);
	sys_write32((dtcvval << CDNS_SRS11_DTCV) |
		(sdclkfsval << CDNS_SRS11_SDCLKFS) | CDNS_SRS11_ICE,
			cdns_params.reg_base + SDHC_CDNS_SRS11);
	ret = sdhc_cdns_wait_ics(WAIT_ICS_TIME_DELAY_US, cdns_params.reg_base + SDHC_CDNS_SRS11);
	if (ret != 0) {
		return ret;
	}

	/* Enable DLL reset */
	sys_clear_bit(cdns_params.reg_base + SDHC_CDNS_HRS09, 0);
	/* Set extended_wr_mode */
	sys_write32(((sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) &
			0xFFFFFFF7) | CDNS_HRS09_EXT_WR_MODE), (cdns_params.reg_base
			+ SDHC_CDNS_HRS09));
	/* Release DLL reset */
	sys_set_bits(cdns_params.reg_base + SDHC_CDNS_HRS09, CDNS_HRS09_RDCMD_EN_BIT |
					CDNS_HRS09_RDDATA_EN_BIT);

	sys_write32((dtcvval << CDNS_SRS11_DTCV) | (sdclkfsval << CDNS_SRS11_SDCLKFS) |
		CDNS_SRS11_ICE | CDNS_SRS11_SDCE, cdns_params.reg_base
		+ SDHC_CDNS_SRS11);

	sys_write32(0xFFFFFFFF, cdns_params.reg_base + SDHC_CDNS_SRS13);
	return 0;
}

static int sdhc_cdns_reset(void)
{
	int32_t timeout;

	sys_clear_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, 0xFFFF);

	/* Software reset */
	sys_set_bit(cdns_params.reg_base + SDHC_CDNS_HRS00, CDNS_HRS00_SWR);

	/* Wait status command response ready */
	timeout = CARD_REG_TIME_DELAY_US;
	if (!WAIT_FOR(((sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS00) &
		CDNS_HRS00_SWR) == 0), timeout, k_msleep(1))) {
		LOG_ERR("Software reset is not completed...timedout");
		return -ETIMEDOUT;
	}

	/* Step 1, switch on DLL_RESET */
	sys_clear_bit(cdns_params.reg_base + SDHC_CDNS_HRS09, CDNS_HRS09_PHY_SW_RESET);

	return 0;
}

static int sdhc_cdns_init(void)
{
	int ret = 0;

	ret = sdhc_cdns_program_phy_reg(&sdhc_cdns_combo_phy_reg_info, &sdhc_cdns_sdmmc_reg_info);
	if (ret != 0U) {
		LOG_ERR("SoftPhy register configuration failed");
		return ret;
	}

	ret = sdhc_cdns_init_hrs_io(&sdhc_cdns_combo_phy_reg_info, &sdhc_cdns_sdmmc_reg_info);
	if (ret != 0U) {
		LOG_ERR("Configuration for HRS IO reg failed");
		return ret;
	}

	ret = sdhc_cdns_card_present();
	if (ret != CARD_PRESENT) {
		LOG_ERR("SD card does not detect");
		return -ETIMEDOUT;
	}

	ret = sdhc_cdns_vol_reset();
	if (ret != 0U) {
		LOG_ERR("SD/MMC card reset failed");
		return ret;
	}

	ret = sdhc_cdns_set_clk(&cdns_params);
	if (ret != 0U) {
		LOG_ERR("Host controller set clk failed");
		return ret;
	}

	return 0;
}

static int sdhc_cdns_send_cmd(struct sdmmc_cmd *cmd, struct sdhc_data *data)
{
	uint32_t op = 0;
	uint32_t value;
	uintptr_t base;
	int32_t timeout;
	uint32_t cmd_indx;
	uint32_t status_check = 0;

	__ASSERT(cmd, "Assert %s function call", __func__);
	base = cdns_params.reg_base;
	cmd_indx = (cmd->cmd_idx) << CDNS_SRS03_COM_IDX;

	if (data) {
		switch (cmd->cmd_idx) {
		case SD_SWITCH:
			op = CDNS_SRS03_DATA_PRSNT;
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS10, SRS10_VAL_SW);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_GEN);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, SRS15_VAL_GEN);
			break;

		case SD_WRITE_SINGLE_BLOCK:
		case SD_READ_SINGLE_BLOCK:
			op = CDNS_SRS03_DATA_PRSNT;
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS10, SRS10_VAL_READ);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_GEN);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, SRS15_VAL_RD_WR);
			sys_write32(CDNS_SRS00_SAAR, cdns_params.reg_base + SDHC_CDNS_SRS00);
			break;

		case SD_WRITE_MULTIPLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
			op = CDNS_SRS03_DATA_PRSNT | AUTO_CMD23 | CDNS_SRS03_MULTI_BLK_READ;
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS10, SRS10_VAL_READ);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_GEN);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, SRS15_VAL_RD_WR);
			sys_write32(CDNS_SRS00_SAAR, cdns_params.reg_base + SDHC_CDNS_SRS00);
			break;

		case SD_APP_SEND_SCR:
			op = CDNS_SRS03_DATA_PRSNT;
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS10, ADMA2_32);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_GEN);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, SRS15_VAL_GEN);
			break;

		default:
			op = 0;
			break;
		}
	} else {
		switch (cmd->cmd_idx) {
		case SD_GO_IDLE_STATE:
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_CID);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, CDNS_SRS15_HV4E);
			break;

		case SD_ALL_SEND_CID:
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_CID);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, SRS15_VAL_CID);
			break;

		case SD_SEND_IF_COND:
			op = CDNS_SRS03_CMD_IDX_CHK_EN;
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS11, SRS11_VAL_GEN);
			sys_set_bits(cdns_params.reg_base + SDHC_CDNS_SRS15, CDNS_SRS15_HV4E);
			break;

		case SD_STOP_TRANSMISSION:
			op = CMD_STOP_ABORT_CMD;
			break;

		case SD_SEND_STATUS:
			break;

		case SD_SELECT_CARD:
			op = CDNS_SRS03_MULTI_BLK_READ;
			break;

		default:
			op = 0;
			break;
		}
	}

	switch (cmd->resp_type) {
	case SD_RSP_TYPE_NONE:
		op |= (CDNS_SRS03_CMD_READ | CDNS_SRS03_MULTI_BLK_READ |
			CDNS_SRS03_DMA_EN | CDNS_SRS03_BLK_CNT_EN);
		break;

	case SD_RSP_TYPE_R2:
		op |= (CDNS_SRS03_CMD_READ | CDNS_SRS03_MULTI_BLK_READ |
			CDNS_SRS03_DMA_EN | CDNS_SRS03_BLK_CNT_EN |
			RES_TYPE_SEL_136 | CDNS_SRS03_RESP_CRCCE);
		break;

	case SD_RSP_TYPE_R3:
		op |= (CDNS_SRS03_CMD_READ | CDNS_SRS03_MULTI_BLK_READ |
			CDNS_SRS03_DMA_EN | CDNS_SRS03_BLK_CNT_EN | RES_TYPE_SEL_48);
		break;

	case SD_RSP_TYPE_R1:
		if ((cmd->cmd_idx == SD_WRITE_SINGLE_BLOCK) || (cmd->cmd_idx
			== SD_WRITE_MULTIPLE_BLOCK)) {
			op |= (CDNS_SRS03_DMA_EN | CDNS_SRS03_BLK_CNT_EN | RES_TYPE_SEL_48
			| CDNS_SRS03_RESP_CRCCE | CDNS_SRS03_CMD_IDX_CHK_EN);
		} else {
			op |= (CDNS_SRS03_DMA_EN | CDNS_SRS03_BLK_CNT_EN | CDNS_SRS03_CMD_READ
			| RES_TYPE_SEL_48 | CDNS_SRS03_RESP_CRCCE | CDNS_SRS03_CMD_IDX_CHK_EN);
		}
		break;

	default:
		op |= (CDNS_SRS03_DMA_EN | CDNS_SRS03_BLK_CNT_EN | CDNS_SRS03_CMD_READ |
			CDNS_SRS03_MULTI_BLK_READ | RES_TYPE_SEL_48 | CDNS_SRS03_RESP_CRCCE |
			CDNS_SRS03_CMD_IDX_CHK_EN);
		break;
	}

	timeout = CARD_REG_TIME_DELAY_US;
	if (!WAIT_FOR((sdhc_cdns_busy() == 0), timeout, k_msleep(1))) {
		k_panic();
	}

	sys_write32(~0, cdns_params.reg_base + SDHC_CDNS_SRS12);

	sys_write32(cmd->cmd_arg, cdns_params.reg_base + SDHC_CDNS_SRS02);
	sys_write32(RESET_SRS14, cdns_params.reg_base + SDHC_CDNS_SRS14);
	sys_write32(op | cmd_indx, cdns_params.reg_base + SDHC_CDNS_SRS03);

	timeout = CARD_REG_TIME_DELAY_US;
	if (!WAIT_FOR(((((sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS12)) &
			CDNS_SRS12_CC) == CDNS_SRS12_CC) | (((sys_read32(cdns_params.reg_base +
			SDHC_CDNS_SRS12)) & CDNS_SRS12_EINT) == CDNS_SRS12_EINT)),
			timeout, k_msleep(1))) {
		LOG_ERR("Response timeout SRS12");
		return -ETIMEDOUT;
	}

	value = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS12);
	status_check = value & CDNS_SRS12_ERR_MASK;
	if (status_check != 0U) {
		LOG_ERR("SD host controller send command failed, SRS12 = %X", status_check);
		return -EIO;
	}

	if ((op & RES_TYPE_SEL_48) || (op & RES_TYPE_SEL_136)) {
		cmd->resp_data[0] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS04);
		if (op & RES_TYPE_SEL_136) {
			cmd->resp_data[1] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS05);
			cmd->resp_data[2] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS06);
			cmd->resp_data[3] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS07);

			/* 136-bit: RTS=01b, Response field R[127:8] - RESP3[23:0],
			 * RESP2[31:0], RESP1[31:0], RESP0[31:0]
			 * Subsystem expects 128 bits response but cadence SDHC sends
			 * 120 bits response from R[127:8]. Bits manupulation to address
			 * the correct responses for the 136 bit response type.
			 */
			cmd->resp_data[3] = ((cmd->resp_data[3] << 8) | ((cmd->resp_data[2] >> 24)
				& CDNS_CSD_BYTE_MASK));
			cmd->resp_data[2] = ((cmd->resp_data[2] << 8) | ((cmd->resp_data[1] >> 24)
				& CDNS_CSD_BYTE_MASK));
			cmd->resp_data[1] = ((cmd->resp_data[1] << 8) | ((cmd->resp_data[0] >> 24)
				& CDNS_CSD_BYTE_MASK));
			cmd->resp_data[0] = (cmd->resp_data[0] << 8);
		}
	}

	return 0;
}

static const struct sdhc_cdns_ops cdns_sdmmc_ops = {
	.init			= sdhc_cdns_init,
	.send_cmd		= sdhc_cdns_send_cmd,
	.card_present		= sdhc_cdns_card_present,
	.set_ios		= sdhc_cdns_set_ios,
	.prepare		= sdhc_cdns_prepare,
	.cache_invd		= sdhc_cdns_cache_invd,
	.busy			= sdhc_cdns_busy,
	.reset			= sdhc_cdns_reset,
};

void sdhc_cdns_sdmmc_init(struct sdhc_cdns_params *params, struct sdmmc_device_info *info,
	const struct sdhc_cdns_ops  **cb_sdmmc_ops)
{
	__ASSERT_NO_MSG((params != NULL) &&
		((params->reg_base & MMC_BLOCK_MASK) == 0) &&
		((params->desc_size & MMC_BLOCK_MASK) == 0) &&
		((params->reg_phy & MMC_BLOCK_MASK) == 0) &&
		(params->desc_size > 0) &&
		(params->clk_rate > 0) &&
		((params->bus_width == MMC_BUS_WIDTH_1) ||
		(params->bus_width == MMC_BUS_WIDTH_4) ||
		(params->bus_width == MMC_BUS_WIDTH_8)));

	memcpy(&cdns_params, params, sizeof(struct sdhc_cdns_params));
	cdns_params.cdn_sdmmc_dev_type = info->cdn_sdmmc_dev_type;
	*cb_sdmmc_ops = &cdns_sdmmc_ops;

	cdns_sdhc_set_sdmmc_params(&sdhc_cdns_combo_phy_reg_info, &sdhc_cdns_sdmmc_reg_info);
}
