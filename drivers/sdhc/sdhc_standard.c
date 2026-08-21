/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "sdhc_standard.h"

LOG_MODULE_REGISTER(sdhc_standard, CONFIG_SDHC_LOG_LEVEL);

uint8_t sdhc_standard_read8(struct sdhc_standard_host *host, uint32_t reg)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->read8 != NULL);

	return host->read8(host->dev, reg);
}

void sdhc_standard_write8(struct sdhc_standard_host *host, uint32_t reg, uint8_t value)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->write8 != NULL);

	host->write8(host->dev, reg, value);
}

uint16_t sdhc_standard_read16(struct sdhc_standard_host *host, uint32_t reg)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->read16 != NULL);

	return host->read16(host->dev, reg);
}

void sdhc_standard_write16(struct sdhc_standard_host *host, uint32_t reg, uint16_t value)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->write16 != NULL);

	host->write16(host->dev, reg, value);
}

uint32_t sdhc_standard_read32(struct sdhc_standard_host *host, uint32_t reg)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->read32 != NULL);

	return host->read32(host->dev, reg);
}

void sdhc_standard_write32(struct sdhc_standard_host *host, uint32_t reg, uint32_t value)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->write32 != NULL);

	host->write32(host->dev, reg, value);
}

uint64_t sdhc_standard_read64(struct sdhc_standard_host *host, uint32_t reg)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->read64 != NULL);

	return host->read64(host->dev, reg);
}

void sdhc_standard_write64(struct sdhc_standard_host *host, uint32_t reg, uint64_t value)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(host->write64 != NULL);

	host->write64(host->dev, reg, value);
}

void sdhc_standard_capabilities_init(struct sdhc_standard_host *host, struct sdhc_host_caps *caps)
{
	__ASSERT_NO_MSG(host != NULL);
	__ASSERT_NO_MSG(caps != NULL);

	uint64_t val = sdhc_standard_read64(host, SDHC_REG_CAPABILITIES);

	LOG_DBG("SDHC standard capabilities: 0x%016" PRIx64 "\n", val);

	caps->timeout_clk_freq = (val & SDHC_REG_CAPABILITIES_TCF_MASK);
	caps->timeout_clk_unit = (val & SDHC_REG_CAPABILITIES_TCU_MASK) != 0 ? 1 : 0;
	caps->sd_base_clk =
		(val & SDHC_REG_CAPABILITIES_BCF_MASK) >> SDHC_REG_CAPABILITIES_BCF_SHIFT;
	caps->max_blk_len =
		(val & SDHC_REG_CAPABILITIES_MBL_MASK) >> SDHC_REG_CAPABILITIES_MBL_SHIFT;
	caps->bus_8_bit_support = (val & SDHC_REG_CAPABILITIES_8B_MASK) != 0 ? 1 : 0;
	caps->adma_2_support = (val & SDHC_REG_CAPABILITIES_ADMA2_MASK) != 0 ? 1 : 0;
	caps->high_spd_support = (val & SDHC_REG_CAPABILITIES_HSS_MASK) != 0 ? 1 : 0;
	caps->sdma_support = (val & SDHC_REG_CAPABILITIES_SDMA_MASK) != 0 ? 1 : 0;
	caps->suspend_res_support = (val & SDHC_REG_CAPABILITIES_SRS_MASK) != 0 ? 1 : 0;
	caps->vol_330_support = (val & SDHC_REG_CAPABILITIES_VS33_MASK) != 0 ? 1 : 0;
	caps->vol_300_support = (val & SDHC_REG_CAPABILITIES_VS30_MASK) != 0 ? 1 : 0;
	caps->vol_180_support = (val & SDHC_REG_CAPABILITIES_VS18_MASK) != 0 ? 1 : 0;
	caps->address_64_bit_support_v4 = 0;
	caps->address_64_bit_support_v3 = (val & SDHC_REG_CAPABILITIES_64B_MASK) != 0 ? 1 : 0;
	caps->sdio_async_interrupt_support = (val & SDHC_REG_CAPABILITIES_AIS_MASK) != 0 ? 1 : 0;
	caps->slot_type = (val & SDHC_REG_CAPABILITIES_ST_MASK) >> SDHC_REG_CAPABILITIES_ST_SHIFT;
	caps->sdr50_support = (val & SDHC_REG_CAPABILITIES_SDR50_MASK) != 0 ? 1 : 0;
	caps->sdr104_support = (val & SDHC_REG_CAPABILITIES_SDR104_MASK) != 0 ? 1 : 0;
	caps->ddr50_support = (val & SDHC_REG_CAPABILITIES_DDR50_MASK) != 0 ? 1 : 0;
	caps->uhs_2_support = 0;
	caps->drv_type_a_support = (val & SDHC_REG_CAPABILITIES_DTA_MASK) != 0 ? 1 : 0;
	caps->drv_type_c_support = (val & SDHC_REG_CAPABILITIES_DTC_MASK) != 0 ? 1 : 0;
	caps->drv_type_d_support = (val & SDHC_REG_CAPABILITIES_DTD_MASK) != 0 ? 1 : 0;
	caps->retune_timer_count =
		(val & SDHC_REG_CAPABILITIES_TCRT_MASK) >> SDHC_REG_CAPABILITIES_TCRT_SHIFT;
	caps->sdr50_needs_tuning = (val & SDHC_REG_CAPABILITIES_UTSDR50_MASK) != 0 ? 1 : 0;
	caps->retuning_mode =
		(val & SDHC_REG_CAPABILITIES_RTM_MASK) >> SDHC_REG_CAPABILITIES_RTM_SHIFT;
	caps->clk_multiplier =
		(val & SDHC_REG_CAPABILITIES_CM_MASK) >> SDHC_REG_CAPABILITIES_CM_SHIFT;
	caps->adma3_support = 0;
	caps->vdd2_180_support = 0;
}
