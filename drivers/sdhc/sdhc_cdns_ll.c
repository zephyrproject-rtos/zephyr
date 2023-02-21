/*
 * Copyright (C) 2023 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include "sdhc_cdns_ll.h"
#include <socfpga_system_manager.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdhc_cdns_ll, CONFIG_SDHC_LOG_LEVEL);

/* card busy and present */
#define CARD_BUSY				1
#define CARD_NOT_BUSY				0
#define CARD_PRESENT				1
#define CARD_NOT_PRESENT			0

/* 500 ms delay to read the RINST register */
#define DELAY_MS_SRS_READ			500

/* SRS12 error mask */
#define SRS12_ERR_MASK				0xFFFF8000

/* TODO: check DV dfi_init val=0 */
#define IO_MASK_END_DATA			0x0

/* TODO: check DV dfi_init val=2; DDR Mode */
#define IO_MASK_END_DATA_DDR			0x2
#define IO_MASK_START_DATA			0x0
#define DATA_SELECT_OE_END_DATA			0x1

#define TIMEOUT_IN_US(x)			(x)

/* General define */
#define SDHC_REG_MASK				0xFFFFFFFF
#define SD_HOST_BLOCK_SIZE			0x200

#define CDMMC_DMA_MAX_BUFFER_SIZE		64*1024
#define CDNSMMC_ADDRESS_MASK			U(0x0f)

static int cdns_init(void);
static int cdns_send_cmd(struct sdmmc_cmd *cmd, struct sdhc_data *data);
static int cdns_set_ios(unsigned int clk, unsigned int width);
static int cdns_prepare(uint32_t lba, uintptr_t buf, struct sdhc_data *data);
static int cdns_cache_invd(int lba, uintptr_t buf, size_t size);
static int cdns_busy(void);
static int cdns_reset(void);
static int cdns_card_present(void);

static const struct sdmmc_ops cdns_sdmmc_ops = {
	.init			= cdns_init,
	.send_cmd		= cdns_send_cmd,
	.card_present		= cdns_card_present,
	.set_ios		= cdns_set_ios,
	.prepare		= cdns_prepare,
	.cache_invd		= cdns_cache_invd,
	.busy			= cdns_busy,
	.reset			= cdns_reset,
};

static struct cdns_sdmmc_params cdns_params;
struct sdmmc_combo_phy sdmmc_combo_phy_reg;
struct sdmmc_sdhc sdmmc_sdhc_reg;

int sdmmc_write_phy_reg(uint32_t phy_reg_addr, uint32_t phy_reg_addr_value,
			uint32_t phy_reg_data, uint32_t phy_reg_data_value)
{
	uint32_t data = 0;
	uint32_t value = 0;

	/* Get PHY register address, write HRS04*/
	value = sys_read32(phy_reg_addr);
	value &= ~PHY_REG_ADDR_MASK;
	value |= phy_reg_addr_value;
	sys_write32(value, phy_reg_addr);
	data = sys_read32(phy_reg_addr);
	if ((data & PHY_REG_ADDR_MASK) != phy_reg_addr_value) {
		return -ENXIO;
	}

	/* Get PHY register data, write HRS05 */
	value &= ~PHY_REG_DATA_MASK;
	value |= phy_reg_data_value;
	sys_write32(value, phy_reg_data);
	k_busy_wait(DELAY_MS_SRS_READ);
	data = sys_read32(phy_reg_data);

	if (data != phy_reg_data_value) {
		LOG_ERR("PHY_REG_DATA is not set properly");
		return -ENXIO;
	}

	return 0;
}

/* Will romove once ATF driver is developed */
void sdmmc_pin_config(void)
{
	/* temp use base + addr. Official must change to common method */
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN0SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN1SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN2SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN3SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN4SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN5SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN6SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN7SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN8SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN9SEL);
	sys_write32(PINMUX_SDMMC_SEL, cdns_params.reg_pinmux+PIN10SEL);
}

int wait_ics(uint16_t timeout, uint32_t cdn_srs_res)
{
	/* Wait status command response ready */
	if (!WAIT_FOR(((sys_read32(cdn_srs_res) & ICS) == ICS),
			timeout, k_busy_wait(1))) {
		LOG_ERR("Failed in wait ics");
		return -ETIMEDOUT;
	}

	return 0;
}

static int cdns_busy(void)
{
	unsigned int data;

	data = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS09);
	return (data & STATUS_DATA_BUSY) ? CARD_BUSY : CARD_NOT_BUSY;
}

static int cdns_card_present(void)
{
	uint32_t timeout = TIMEOUT_IN_US(100000);

	if (!WAIT_FOR((((sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS09)) & CI) == CI),
			timeout, k_busy_wait(1))) {
		LOG_ERR("Card detection timeout");
		return -ETIMEDOUT;
	}

	return CARD_PRESENT;
}

int cdns_vol_reset(void)
{
	/* Reset embedded card */
	sys_write32((7 << BVS) | (1 << BP), (cdns_params.reg_base + SDHC_CDNS_SRS10));
	k_busy_wait(DELAY_MS_SRS_READ);
	sys_write32((7 << BVS) | (0 << BP), (cdns_params.reg_base + SDHC_CDNS_SRS10));
	k_busy_wait(DELAY_MS_SRS_READ);

	/* Turn on supply voltage */
	/* BVS = 7, BP = 1, BP2 only in UHS2 mode */
	sys_write32((7 << BVS) | (1 << BP), (cdns_params.reg_base + SDHC_CDNS_SRS10));
	k_busy_wait(DELAY_MS_SRS_READ);
	return 0;
}

void set_sdmmc_var(struct sdmmc_combo_phy *sdmmc_combo_phy_reg,
	struct sdmmc_sdhc *sdmmc_sdhc_reg)
{
	/* Values are taken by the reference of cadence IP documents */
	sdmmc_combo_phy_reg->cp_clk_wr_delay = 0;
	sdmmc_combo_phy_reg->cp_clk_wrdqs_delay = 0;
	sdmmc_combo_phy_reg->cp_data_select_oe_end = 0;
	sdmmc_combo_phy_reg->cp_dll_bypass_mode = 1;
	sdmmc_combo_phy_reg->cp_dll_locked_mode = 0;
	sdmmc_combo_phy_reg->cp_dll_start_point = 0;
	sdmmc_combo_phy_reg->cp_gate_cfg_always_on = 1;
	sdmmc_combo_phy_reg->cp_io_mask_always_on = 0;
	sdmmc_combo_phy_reg->cp_io_mask_end = 0;
	sdmmc_combo_phy_reg->cp_io_mask_start = 0;
	sdmmc_combo_phy_reg->cp_rd_del_sel = 52;
	sdmmc_combo_phy_reg->cp_read_dqs_cmd_delay = 0;
	sdmmc_combo_phy_reg->cp_read_dqs_delay = 0;
	sdmmc_combo_phy_reg->cp_sw_half_cycle_shift = 0;
	sdmmc_combo_phy_reg->cp_sync_method = 1;
	sdmmc_combo_phy_reg->cp_underrun_suppress = 1;
	sdmmc_combo_phy_reg->cp_use_ext_lpbk_dqs = 1;
	sdmmc_combo_phy_reg->cp_use_lpbk_dqs = 1;
	sdmmc_combo_phy_reg->cp_use_phony_dqs = 1;
	sdmmc_combo_phy_reg->cp_use_phony_dqs_cmd = 1;

	sdmmc_sdhc_reg->sdhc_extended_rd_mode = 1;
	sdmmc_sdhc_reg->sdhc_extended_wr_mode = 1;
	sdmmc_sdhc_reg->sdhc_hcsdclkadj = 0;
	sdmmc_sdhc_reg->sdhc_idelay_val = 0;
	sdmmc_sdhc_reg->sdhc_rdcmd_en = 1;
	sdmmc_sdhc_reg->sdhc_rddata_en = 1;
	sdmmc_sdhc_reg->sdhc_rw_compensate = 9;
	sdmmc_sdhc_reg->sdhc_sdcfsh = 0;
	sdmmc_sdhc_reg->sdhc_sdcfsl = 1;
	sdmmc_sdhc_reg->sdhc_wrcmd0_dly = 1;
	sdmmc_sdhc_reg->sdhc_wrcmd0_sdclk_dly = 0;
	sdmmc_sdhc_reg->sdhc_wrcmd1_dly = 0;
	sdmmc_sdhc_reg->sdhc_wrcmd1_sdclk_dly = 0;
	sdmmc_sdhc_reg->sdhc_wrdata0_dly = 1;
	sdmmc_sdhc_reg->sdhc_wrdata0_sdclk_dly = 0;
	sdmmc_sdhc_reg->sdhc_wrdata1_dly = 0;
	sdmmc_sdhc_reg->sdhc_wrdata1_sdclk_dly = 0;
}

static int cdns_program_phy_reg(struct sdmmc_combo_phy *sdmmc_combo_phy_reg,
	struct sdmmc_sdhc *sdmmc_sdhc_reg)
{
	uint32_t value = 0;
	int ret = 0;

	/* program PHY_DQS_TIMING_REG */
	value = (CP_USE_EXT_LPBK_DQS(sdmmc_combo_phy_reg->cp_use_ext_lpbk_dqs))
		| (CP_USE_LPBK_DQS(sdmmc_combo_phy_reg->cp_use_lpbk_dqs))
		| (CP_USE_PHONY_DQS(sdmmc_combo_phy_reg->cp_use_phony_dqs))
		| (CP_USE_PHONY_DQS_CMD(sdmmc_combo_phy_reg->cp_use_phony_dqs_cmd));
	ret = sdmmc_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DQS_TIMING_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		return ret;
	}

	/* program PHY_GATE_LPBK_CTRL_REG */
	value = (CP_SYNC_METHOD(sdmmc_combo_phy_reg->cp_sync_method))
		| (CP_SW_HALF_CYCLE_SHIFT(sdmmc_combo_phy_reg->cp_sw_half_cycle_shift))
		| (CP_RD_DEL_SEL(sdmmc_combo_phy_reg->cp_rd_del_sel))
		| (CP_UNDERRUN_SUPPRESS(sdmmc_combo_phy_reg->cp_underrun_suppress))
		| (CP_GATE_CFG_ALWAYS_ON(sdmmc_combo_phy_reg->cp_gate_cfg_always_on));
	ret = sdmmc_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_GATE_LPBK_CTRL_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		return -ret;
	}

	/* program PHY_DLL_MASTER_CTRL_REG */
	value = (CP_DLL_BYPASS_MODE(sdmmc_combo_phy_reg->cp_dll_bypass_mode))
			| (CP_DLL_START_POINT(sdmmc_combo_phy_reg->cp_dll_start_point));
	ret = sdmmc_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DLL_MASTER_CTRL_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		return ret;
	}

	/* program PHY_DLL_SLAVE_CTRL_REG */
	value = (CP_READ_DQS_CMD_DELAY(sdmmc_combo_phy_reg->cp_read_dqs_cmd_delay))
		| (CP_CLK_WRDQS_DELAY(sdmmc_combo_phy_reg->cp_clk_wrdqs_delay))
		| (CP_CLK_WR_DELAY(sdmmc_combo_phy_reg->cp_clk_wr_delay))
		| (CP_READ_DQS_DELAY(sdmmc_combo_phy_reg->cp_read_dqs_delay));
	ret = sdmmc_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DLL_SLAVE_CTRL_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		return ret;
	}

	/* program PHY_CTRL_REG */
	sys_write32(cdns_params.combophy + PHY_CTRL_REG, cdns_params.reg_base
		+ SDHC_CDNS_HRS04);
	value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS05);

	/* phony_dqs_timing=0 */
	value &= ~(CP_PHONY_DQS_TIMING_MASK << CP_PHONY_DQS_TIMING_SHIFT);
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS05);

	/* switch off DLL_RESET */
	do {
		value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
		value |= SDHC_PHY_SW_RESET;
		sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS09);
		value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
	/* polling PHY_INIT_COMPLETE */
	} while ((value & SDHC_PHY_INIT_COMPLETE) != SDHC_PHY_INIT_COMPLETE);

	/* program PHY_DQ_TIMING_REG */
	sdmmc_combo_phy_reg->cp_io_mask_end = 0U;
	value = (CP_IO_MASK_ALWAYS_ON(sdmmc_combo_phy_reg->cp_io_mask_always_on))
		| (CP_IO_MASK_END(sdmmc_combo_phy_reg->cp_io_mask_end))
		| (CP_IO_MASK_START(sdmmc_combo_phy_reg->cp_io_mask_start))
		| (CP_DATA_SELECT_OE_END(sdmmc_combo_phy_reg->cp_data_select_oe_end));

	ret = sdmmc_write_phy_reg(cdns_params.reg_base + SDHC_CDNS_HRS04,
			cdns_params.combophy + PHY_DQ_TIMING_REG, cdns_params.reg_base
			+ SDHC_CDNS_HRS05, value);
	if (ret != 0U) {
		return ret;
	}
	return 0;
}

static int cdns_cache_invd(int lba, uintptr_t buf, size_t size)
{
	arch_dcache_invd_range((void *)buf, size);

	return 0;
}

static int cdns_prepare(uint32_t dma_start_addr, uintptr_t dma_buff,
	struct sdhc_data *data)
{
	struct cdns_idmac_desc *desc;
	uint32_t desc_cnt, i;
	uintptr_t base;
	uint64_t desc_base;
	uint32_t size = data->blocks * data->block_size;

	__ASSERT_NO_MSG(((buf & CDNSMMC_ADDRESS_MASK) == 0) &&
			(cdns_params.desc_size > 0) &&
			((cdns_params.reg_base & MMC_BLOCK_MASK) == 0) &&
			((cdns_params.desc_base & MMC_BLOCK_MASK) == 0) &&
			((cdns_params.desc_size & MMC_BLOCK_MASK) == 0));

	arch_dcache_flush_range((void *)dma_buff, size);

	desc_cnt = (size + (CDMMC_DMA_MAX_BUFFER_SIZE) - 1) / (CDMMC_DMA_MAX_BUFFER_SIZE);
	__ASSERT_NO_MSG(desc_cnt * sizeof(struct cdns_idmac_desc) < cdns_params.desc_size);

	if (desc_cnt > CONFIG_CDNS_DESC_COUNT) {
		LOG_ERR("Requested data transfer length %d is greater than configured length %d",
				size, (CONFIG_CDNS_DESC_COUNT * CDMMC_DMA_MAX_BUFFER_SIZE));
		return -EINVAL;
	}

	base = cdns_params.reg_base;
	desc = (struct cdns_idmac_desc *)cdns_params.desc_base;
	desc_base = (uint64_t)desc;
	i = desc_cnt;

	while (--i) {
		desc->attr = ADMA_DESC_ATTR_VALID | ADMA_DESC_TRANSFER_DATA;
		desc->reserved = 0;
		desc->len = MAX_64KB_PAGE;
		desc->addr_lo = (dma_buff & 0xffffffff) + (CDMMC_DMA_MAX_BUFFER_SIZE * i);
		desc->addr_hi = (dma_buff >> 32) & 0xffffffff;
		size -= CDMMC_DMA_MAX_BUFFER_SIZE;
		desc++;
	}

	desc->attr = ADMA_DESC_ATTR_VALID | ADMA_DESC_TRANSFER_DATA |
			ADMA_DESC_ATTR_END;
	desc->reserved = 0;
	desc->len = size;
	desc->addr_lo = (dma_buff & 0xffffffff);
	desc->addr_hi = (dma_buff >> 32) & 0xffffffff;

	sys_write32((uint32_t)desc_base, cdns_params.reg_base + SDHC_CDNS_SRS22);
	sys_write32((uint32_t)(desc_base >> 32), cdns_params.reg_base + SDHC_CDNS_SRS23);
	arch_dcache_flush_range((void *)cdns_params.desc_base,
				desc_cnt * CDMMC_DMA_MAX_BUFFER_SIZE);

	sys_write32((data->block_size << BLOCK_SIZE | data->blocks << BLK_COUNT_CT
		| SDMA_BUF), cdns_params.reg_base + SDHC_CDNS_SRS01);

	return 0;
}

static void host_set_clk(int clk)
{
	uint32_t ret = 0;
	uint32_t sdclkfsval = 0;
	uint32_t dtcvval = 0xe;

	sdclkfsval = (cdns_params.clk_rate / 2000) / clk;
	sys_write32(0, cdns_params.reg_base + SDHC_CDNS_SRS11);
	sys_write32((dtcvval << DTCV) | (sdclkfsval << SDCLKFS) |
		(1 << ICE), cdns_params.reg_base + SDHC_CDNS_SRS11);

	ret = wait_ics(TIMEOUT_IN_US(5000), cdns_params.reg_base + SDHC_CDNS_SRS11);
	if (ret != 0) {
		LOG_ERR("Waiting ICS timeout");
	}

	/* Enable DLL reset */
	sys_write32(sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) &
			~0x00000001, (cdns_params.reg_base + SDHC_CDNS_HRS09));
	/* Set extended_wr_mode */
	sys_write32((sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09)
			& 0xFFFFFFF7) | (1 << EXTENDED_WR_MODE), (cdns_params.reg_base
			+ SDHC_CDNS_HRS09));
	/* Release DLL reset */
	sys_write32(sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) |
			1, cdns_params.reg_base + SDHC_CDNS_HRS09);
	sys_write32(sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) |
			(3 << RDCMD_EN), cdns_params.reg_base + SDHC_CDNS_HRS09);

	do {
		sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
	} while (~sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) & (1 << 1));

	sys_write32((dtcvval << DTCV) | (sdclkfsval << SDCLKFS) | (1 << ICE) |
			(1 << SDCE), cdns_params.reg_base + SDHC_CDNS_SRS11);

	sys_write32(0xFFFFFFFF, cdns_params.reg_base + SDHC_CDNS_SRS13);
}

static int cdns_set_ios(unsigned int clk, unsigned int width)
{
	uint32_t _status = 0;

	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		sys_write32(BIT1 | _status, (cdns_params.reg_base + SDHC_CDNS_SRS10));
		break;
	case SDHC_BUS_WIDTH4BIT:
		sys_write32(BIT4 | _status, (cdns_params.reg_base + SDHC_CDNS_SRS10));
		break;
	case SDHC_BUS_WIDTH8BIT:
		sys_write32(BIT8 | _status, (cdns_params.reg_base + SDHC_CDNS_SRS10));
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	host_set_clk(clk);

	return 0;
}

int sdmmc_write_sd_host_reg(uint32_t addr, uint32_t data)
{
	uint32_t value = 0;

	value = sys_read32(addr);
	value &= ~SDHC_REG_MASK;
	value |= data;
	sys_write32(value, addr);
	value = sys_read32(addr);
	if (value != data) {
		LOG_ERR("SD host address is not set properly\n");
		return -ENXIO;
	}

	return 0;
}

static int cdns_init_hrs_io(struct sdmmc_combo_phy *sdmmc_combo_phy_reg,
	struct sdmmc_sdhc *sdmmc_sdhc_reg)
{
	uint32_t value = 0;
	int ret = 0;

	/* program HRS09, register 42 */
	value = (SDHC_RDDATA_EN(sdmmc_sdhc_reg->sdhc_rddata_en))
		| (SDHC_RDCMD_EN(sdmmc_sdhc_reg->sdhc_rdcmd_en))
		| (SDHC_EXTENDED_WR_MODE(sdmmc_sdhc_reg->sdhc_extended_wr_mode))
		| (SDHC_EXTENDED_RD_MODE(sdmmc_sdhc_reg->sdhc_extended_rd_mode));
	ret = sdmmc_write_sd_host_reg(cdns_params.reg_base + SDHC_CDNS_HRS09, value);
	if (ret != 0U) {
		LOG_ERR("Program HRS09 failed");
		return ret;
	}

	/* program HRS10, register 43 */
	value = (SDHC_HCSDCLKADJ(sdmmc_sdhc_reg->sdhc_hcsdclkadj));
	ret = sdmmc_write_sd_host_reg(cdns_params.reg_base + SDHC_CDNS_HRS10, value);
	if (ret != 0U) {
		LOG_ERR("Program HRS10 failed");
		return ret;
	}

	/* program HRS16, register 48 */
	value = (SDHC_WRDATA1_SDCLK_DLY(sdmmc_sdhc_reg->sdhc_wrdata1_sdclk_dly))
		| (SDHC_WRDATA0_SDCLK_DLY(sdmmc_sdhc_reg->sdhc_wrdata0_sdclk_dly))
		| (SDHC_WRCMD1_SDCLK_DLY(sdmmc_sdhc_reg->sdhc_wrcmd1_sdclk_dly))
		| (SDHC_WRCMD0_SDCLK_DLY(sdmmc_sdhc_reg->sdhc_wrcmd0_sdclk_dly))
		| (SDHC_WRDATA1_DLY(sdmmc_sdhc_reg->sdhc_wrdata1_dly))
		| (SDHC_WRDATA0_DLY(sdmmc_sdhc_reg->sdhc_wrdata0_dly))
		| (SDHC_WRCMD1_DLY(sdmmc_sdhc_reg->sdhc_wrcmd1_dly))
		| (SDHC_WRCMD0_DLY(sdmmc_sdhc_reg->sdhc_wrcmd0_dly));
	ret = sdmmc_write_sd_host_reg(cdns_params.reg_base + SDHC_CDNS_HRS16, value);
	if (ret != 0U) {
		LOG_ERR("Program HRS16 failed");
		return ret;
	}

	/* program HRS07, register 40 */
	value = (SDHC_RW_COMPENSATE(sdmmc_sdhc_reg->sdhc_rw_compensate))
		| (SDHC_IDELAY_VAL(sdmmc_sdhc_reg->sdhc_idelay_val));
	ret = sdmmc_write_sd_host_reg(cdns_params.reg_base + SDHC_CDNS_HRS07, value);
	if (ret != 0U) {
		LOG_ERR("Program HRS07 failed");
		return ret;
	}

	return ret;
}

static int cdns_hc_set_clk(struct cdns_sdmmc_params *cdn_sdmmc_dev_type_params)
{
	uint32_t ret = 0;
	uint32_t dtcvval, sdclkfsval;

	dtcvval = DTC_VAL;
	sdclkfsval = 0;

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
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_HS400es)) {
		sdclkfsval = 1;
	} else if ((cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == SD_UHS_SDR104) ||
		(cdn_sdmmc_dev_type_params->cdn_sdmmc_dev_type == EMMC_HS200)) {
		sdclkfsval = 0;
	}

	sys_write32(0, cdns_params.reg_base + SDHC_CDNS_SRS11);
	sys_write32((dtcvval << DTCV) |
		(sdclkfsval << SDCLKFS) | (1 << ICE), cdns_params.reg_base + SDHC_CDNS_SRS11);
	ret = wait_ics(TIMEOUT_IN_US(5000), cdns_params.reg_base + SDHC_CDNS_SRS11);
	if (ret != 0) {
		LOG_ERR("Waiting ICS timeout");
		return ret;
	}

	/* Enable DLL reset */
	sys_write32(sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) &
			~0x00000001, (cdns_params.reg_base + SDHC_CDNS_HRS09));
	/* Set extended_wr_mode */
	sys_write32((sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) &
			0xFFFFFFF7) | (1 << EXTENDED_WR_MODE), (cdns_params.reg_base
			+ SDHC_CDNS_HRS09));
	/* Release DLL reset */
	sys_write32(sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) |
			1, cdns_params.reg_base + SDHC_CDNS_HRS09);
	sys_write32(sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) |
			(3 << RDCMD_EN), cdns_params.reg_base + SDHC_CDNS_HRS09);
	do {
		sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
	} while (~sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09) & (1 << 1));

	sys_write32((dtcvval << DTCV) |
		(sdclkfsval << SDCLKFS) | (1 << ICE) | (1 << SDCE), cdns_params.reg_base
		+ SDHC_CDNS_SRS11);

	sys_write32(0xFFFFFFFF, cdns_params.reg_base + SDHC_CDNS_SRS13);
	return 0;
}

static int cdns_reset(void)
{
	uint32_t value = 0;
	int32_t timeout;

	value = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS11);
	value &= ~(0xFFFF);
	value |= 0x0;
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_SRS11);
	k_busy_wait(DELAY_MS_SRS_READ);

	/* Software reset */
	sys_write32(1, cdns_params.reg_base + SDHC_CDNS_HRS00);
	/* Wait status command response ready */
	timeout = TIMEOUT_IN_US(100000);
	if (!WAIT_FOR(((sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS00) & 1) == 0),
			timeout, k_busy_wait(1))) {
		LOG_ERR("Software reset is not completed...timedout");
		return -ETIMEDOUT;
	}

	/* Step 1, switch on DLL_RESET */
	value = sys_read32(cdns_params.reg_base + SDHC_CDNS_HRS09);
	value &= ~SDHC_PHY_SW_RESET;
	sys_write32(value, cdns_params.reg_base + SDHC_CDNS_HRS09);

	return 0;
}

static int cdns_init(void)
{
	int ret = 0;

	ret = cdns_program_phy_reg(&sdmmc_combo_phy_reg, &sdmmc_sdhc_reg);
	if (ret != 0U) {
		LOG_ERR("Program phy reg init failed");
		return ret;
	}

	ret = cdns_init_hrs_io(&sdmmc_combo_phy_reg, &sdmmc_sdhc_reg);
	if (ret != 0U) {
		LOG_ERR("Program init for HRS reg is failed");
		return ret;
	}

	ret = cdns_card_present();
	if (ret != CARD_PRESENT) {
		LOG_ERR("SD card does not detect");
		return -ETIMEDOUT;
	}

	ret = cdns_vol_reset();
	if (ret != 0U) {
		LOG_ERR("eMMC card reset failed");
		return ret;
	}

	ret = cdns_hc_set_clk(&cdns_params);
	if (ret != 0U) {
		LOG_ERR("hc set clk failed");
		return ret;
	}

	return 0;
}

void srs10_value_toggle(uint8_t write_val, uint8_t prev_val)
{
	uint32_t data_op = 0U;

	data_op = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS10);
	sys_write32((data_op & (prev_val << 0)), cdns_params.reg_base + SDHC_CDNS_SRS10);
	sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS10);
	sys_write32(data_op | (write_val << 0), cdns_params.reg_base + SDHC_CDNS_SRS10);
}

void srs11_srs15_config(uint32_t srs11_val, uint32_t srs15_val)
{
	uint32_t data = 0U;

	data = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS11);
	sys_write32((data | srs15_val), cdns_params.reg_base + SDHC_CDNS_SRS11);
	data = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS15);
	sys_write32((data | srs15_val), cdns_params.reg_base + SDHC_CDNS_SRS15);
}

static int cdns_send_cmd(struct sdmmc_cmd *cmd, struct sdhc_data *data)
{
	uint32_t op = 0;
	uint8_t write_value = 0, prev_val = 0;
	uint32_t value;
	uintptr_t base;
	int32_t timeout;
	uint32_t cmd_indx;
	uint32_t status = 0, srs15_val = 0, srs11_val = 0;
	uint32_t status_check = 0;

	__ASSERT_NO_MSG(cmd);
	base = cdns_params.reg_base;
	cmd_indx = (cmd->cmd_idx) << COM_IDX;

	if (data) {
		switch (cmd->cmd_idx) {
		case SD_SWITCH:
			op = DATA_PRESENT;
			write_value = ADMA2_32 | DT_WIDTH;
			prev_val = ADMA2_32 | DT_WIDTH;
			srs10_value_toggle(write_value, prev_val);
			srs11_val = READ_CLK | ICE | ICS | SDCE;
			srs15_val = BIT_AD_64 | HV4E | V18SE;
			srs11_srs15_config(srs11_val, srs15_val);
			break;

		case SD_WRITE_SINGLE_BLOCK:

		case SD_READ_SINGLE_BLOCK:
			op = DATA_PRESENT;
			write_value = ADMA2_32 | HS_EN | DT_WIDTH | LEDC;
			prev_val = ADMA2_32 | HS_EN | DT_WIDTH;
			srs10_value_toggle(write_value, prev_val);
			srs15_val = PVE | BIT_AD_64 | HV4E | SDR104_MODE | V18SE;
			srs11_val = READ_CLK | ICE | ICS | SDCE;
			srs11_srs15_config(srs11_val, srs15_val);
			sys_write32(SAAR, cdns_params.reg_base + SDHC_CDNS_SRS00);
			break;

		case SD_WRITE_MULTIPLE_BLOCK:

		case SD_READ_MULTIPLE_BLOCK:
			op = DATA_PRESENT | AUTO_CMD_EN | MULTI_BLK_READ;
			write_value = ADMA2_32 | HS_EN | DT_WIDTH | LEDC;
			prev_val = ADMA2_32 | HS_EN | DT_WIDTH;
			srs10_value_toggle(write_value, prev_val);
			srs15_val = PVE | BIT_AD_64 | HV4E | SDR104_MODE | V18SE;
			srs11_val = READ_CLK | ICE | ICS | SDCE;
			srs11_srs15_config(srs11_val, srs15_val);
			sys_write32(SAAR, cdns_params.reg_base + SDHC_CDNS_SRS00);
			break;

		case SD_APP_SEND_SCR:
			op = DATA_PRESENT;
			write_value = ADMA2_32 | LEDC;
			prev_val = LEDC;
			srs10_value_toggle(write_value, prev_val);
			srs15_val = BIT_AD_64 | HV4E | V18SE;
			srs11_val = READ_CLK | ICE | ICS | SDCE;
			srs11_srs15_config(srs11_val, srs15_val);
			break;

		default:
			write_value = LEDC;
			prev_val = 0x0;
			srs10_value_toggle(write_value, prev_val);
			op = 0;
			break;
		}
	} else {
		switch (cmd->cmd_idx) {
		case SD_GO_IDLE_STATE:
			write_value = LEDC;
			prev_val = 0x0;
			srs10_value_toggle(write_value, prev_val);
			srs15_val = HV4E;
			srs11_val = ICE | ICS | SDCE;
			srs11_srs15_config(srs11_val, srs15_val);
			break;

		case SD_ALL_SEND_CID:
			write_value = LEDC;
			prev_val = 0x0;
			srs10_value_toggle(write_value, prev_val);
			srs15_val = HV4E | V18SE;
			srs11_val = ICE | ICS | SDCE;
			srs11_srs15_config(srs11_val, srs15_val);
			break;

		case SD_SEND_IF_COND:
			op = CMD_IDX_CHK_ENABLE;
			write_value = LEDC;
			prev_val = 0x0;
			srs10_value_toggle(write_value, prev_val);
			srs15_val = HV4E;
			srs11_val = READ_CLK | ICE | ICS | SDCE;
			srs11_srs15_config(srs11_val, srs15_val);
			break;

		case SD_STOP_TRANSMISSION:
			op = CMD_STOP_ABORT_CMD;
			break;

		case SD_SEND_STATUS:
			break;

		case SD_SELECT_CARD:
			op = MULTI_BLK_READ;

		case SD_APP_CMD:

		default:
			write_value = LEDC;
			prev_val = 0x0;
			srs10_value_toggle(write_value, prev_val);
			op = 0;
			break;
		}
	}

	switch (cmd->resp_type) {
	case MMC_RESPONSE_NONE:
		op |= CMD_READ | MULTI_BLK_READ | DMA_ENABLED | BLK_CNT_EN;
		break;

	case MMC_RESPONSE_R2:
		op |= CMD_READ | MULTI_BLK_READ | DMA_ENABLED | BLK_CNT_EN |
			RES_TYPE_SEL_136 | CMD_CHECK_RESP_CRC;
		break;

	case MMC_RESPONSE_R3:
		op |= CMD_READ | MULTI_BLK_READ | DMA_ENABLED | BLK_CNT_EN |
			RES_TYPE_SEL_48;
		break;

	case MMC_RESPONSE_R1:
		if ((cmd->cmd_idx == SD_WRITE_SINGLE_BLOCK) || (cmd->cmd_idx
			== SD_WRITE_MULTIPLE_BLOCK)) {
			op |= DMA_ENABLED | BLK_CNT_EN | RES_TYPE_SEL_48
			| CMD_CHECK_RESP_CRC | CMD_IDX_CHK_ENABLE;
		} else {
			op |= DMA_ENABLED | BLK_CNT_EN | CMD_READ | RES_TYPE_SEL_48
			| CMD_CHECK_RESP_CRC | CMD_IDX_CHK_ENABLE;
		}
		break;

	default:
		op |= DMA_ENABLED | BLK_CNT_EN | CMD_READ | MULTI_BLK_READ |
			RES_TYPE_SEL_48 | CMD_CHECK_RESP_CRC | CMD_IDX_CHK_ENABLE;
		break;
	}

	timeout = TIMEOUT_IN_US(100000);
	if (!WAIT_FOR((cdns_busy() == 0), timeout, k_busy_wait(1))) {
		k_panic();
	}

	sys_write32(~0, cdns_params.reg_base + SDHC_CDNS_SRS12);

	sys_write32(cmd->cmd_arg, cdns_params.reg_base + SDHC_CDNS_SRS02);
	sys_write32(0x00000000, cdns_params.reg_base + SDHC_CDNS_SRS14);
	sys_write32(op | cmd_indx, cdns_params.reg_base + SDHC_CDNS_SRS03);

	timeout = TIMEOUT_IN_US(100000);
	if (!WAIT_FOR(((((sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS12)) &
			INT_CMD_DONE) == 1) | (((sys_read32(cdns_params.reg_base +
			SDHC_CDNS_SRS12)) & ERROR_INT) == ERROR_INT)), timeout, k_busy_wait(1))) {
		LOG_ERR("Response timeout SRS12");
		return -ETIMEDOUT;
	}

	value = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS12);
	status_check = value & SRS12_ERR_MASK;
	if (status_check != 0U) {
		LOG_ERR("SD host controller send command failed, SRS12 = %x", status);
		return -1;
	}

	if ((op & RES_TYPE_SEL_48) || (op & RES_TYPE_SEL_136)) {
		cmd->resp_data[0] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS04);
		if (op & RES_TYPE_SEL_136) {
			cmd->resp_data[1] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS05);
			cmd->resp_data[2] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS06);
			cmd->resp_data[3] = sys_read32(cdns_params.reg_base + SDHC_CDNS_SRS07);
		}
	}

	return 0;
}

void cdns_sdmmc_init(struct cdns_sdmmc_params *params, struct sdmmc_device_info *info,
	const struct sdmmc_ops **cb_sdmmc_ops)
{
	__ASSERT_NO_MSG((params != NULL) &&
	       ((params->reg_base & MMC_BLOCK_MASK) == 0) &&
	       ((params->desc_base & MMC_BLOCK_MASK) == 0) &&
	       ((params->desc_size & MMC_BLOCK_MASK) == 0) &&
		   ((params->reg_pinmux & MMC_BLOCK_MASK) == 0) &&
		   ((params->reg_phy & MMC_BLOCK_MASK) == 0) &&
	       (params->desc_size > 0) &&
	       (params->clk_rate > 0) &&
	       ((params->bus_width == MMC_BUS_WIDTH_1) ||
		(params->bus_width == MMC_BUS_WIDTH_4) ||
		(params->bus_width == MMC_BUS_WIDTH_8)));

	memcpy(&cdns_params, params, sizeof(struct cdns_sdmmc_params));
	cdns_params.cdn_sdmmc_dev_type = info->cdn_sdmmc_dev_type;
	*cb_sdmmc_ops = &cdns_sdmmc_ops;

	sdmmc_pin_config();

	set_sdmmc_var(&sdmmc_combo_phy_reg, &sdmmc_sdhc_reg);
}
