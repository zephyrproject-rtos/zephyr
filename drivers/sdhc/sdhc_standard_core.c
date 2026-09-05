/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "sdhc_standard_core.h"

LOG_MODULE_REGISTER(sdhc_standard, CONFIG_SDHC_LOG_LEVEL);

uint8_t sdhc_standard_read8(const struct device *dev, uint32_t reg)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);
	uint32_t reg_byte_offset = reg % 4;
	uint32_t reg_addr = reg - reg_byte_offset;
	uint32_t reg_value;

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->read8 != NULL) {
		return cfg->ops->read8(dev, reg);
	}
#endif
	reg_value = sys_read32(base + reg_addr);
	reg_value = (reg_value >> (8 * reg_byte_offset)) & 0x000000ff;

	return reg_value;
}

void sdhc_standard_write8(const struct device *dev, uint32_t reg, uint8_t value)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);
	uint32_t reg_byte_offset = reg % 4;
	uint32_t reg_addr = reg - reg_byte_offset;
	uint32_t reg_value;

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->write8 != NULL) {
		cfg->ops->write8(dev, reg, value);
		return;
	}
#endif
	reg_value = sys_read32(base + reg_addr);
	reg_value &= ~(0x000000ff << reg_byte_offset);
	reg_value |= (value << reg_byte_offset);
	sys_write32(base + reg_addr, reg_value);
}

uint16_t sdhc_standard_read16(const struct device *dev, uint32_t reg)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);
	uint32_t reg_byte_offset = reg % 4;
	uint32_t reg_addr = reg - reg_byte_offset;
	uint32_t reg_value;

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->read16 != NULL) {
		return cfg->ops->read16(dev, reg);
	}
#endif
	reg_value = sys_read32(base + reg_addr);
	reg_value = (reg_value >> (8 * reg_byte_offset)) & 0x0000ffff;

	return reg_value;
}

void sdhc_standard_write16(const struct device *dev, uint32_t reg, uint16_t value)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);
	uint32_t reg_byte_offset = reg % 4;
	uint32_t reg_addr = reg - reg_byte_offset;
	uint32_t reg_value;

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->write16 != NULL) {
		cfg->ops->write16(dev, reg, value);
		return;
	}
#endif
	reg_value = sys_read32(base + reg_addr);
	reg_value &= ~(0x0000ffff << reg_byte_offset);
	reg_value |= (value << reg_byte_offset);
	sys_write32(base + reg_addr, reg_value);
}

uint32_t sdhc_standard_read32(const struct device *dev, uint32_t reg)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->read32 != NULL) {
		return cfg->ops->read32(dev, reg);
	}
#endif
	return sys_read32(base + reg);
}

void sdhc_standard_write32(const struct device *dev, uint32_t reg, uint32_t value)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->write32 != NULL) {
		cfg->ops->write32(dev, reg, value);
		return;
	}
#endif
	sys_write32(base + reg, value);
}

uint64_t sdhc_standard_read64(const struct device *dev, uint32_t reg)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);
	uint32_t lo;
	uint32_t hi;

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->read64 != NULL) {
		return cfg->ops->read64(dev, reg);
	}
#endif
	lo = sys_read32(base + reg);
	hi = sys_read32(base + reg + 4);
	return (((uint64_t)hi << 32) | lo);
}

void sdhc_standard_write64(const struct device *dev, uint32_t reg, uint64_t value)
{
	__ASSERT_NO_MSG(dev != NULL);

	uint32_t base = SDHC_STANDARD_DEV_MMIO_GET(dev);

#ifdef CONFIG_SDHC_STANDARD_CORE_IO_ACCESSORS
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);

	if (cfg->ops->write64 != NULL) {
		cfg->ops->write64(dev, reg, value);
		return;
	}
#endif
	sys_write32(base + reg, value & 0xffffffff);
	sys_write32(base + reg + 4, (value >> 32) & 0xffffffff);
}

static void sdhc_standard_capabilities_init(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	struct sdhc_standard_host_common_data *data = DEV_DATA(dev);
	struct sdhc_host_caps *caps = &data->props.host_caps;
	uint64_t val = sdhc_standard_read64(dev, SDHC_REG_CAPABILITIES);

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

void sdhc_standard_host_init(const struct device *dev)
{
	const struct sdhc_standard_host_common_config *cfg = DEV_CFG(dev);
	struct sdhc_standard_host_common_data *data = DEV_DATA(dev);

	DEVICE_MMIO_NAMED_MAP(dev, sdhc_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	/* Device tree properties init */
	sdhc_common_dt_props_init(&data->props, &cfg->common);

	/* Host capabilities init */
	sdhc_standard_capabilities_init(dev);
}
