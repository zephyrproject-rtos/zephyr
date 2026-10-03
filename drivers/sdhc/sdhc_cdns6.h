/*
 * Copyright (C) 2023 Intel Corporation
 * Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_CDNS6_H
#define ZEPHYR_DRIVERS_SDHC_SDHC_CDNS6_H

#include <errno.h>

#include <zephyr/drivers/sdhc.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* HRS - Host Register Set (specific to Cadence) */
/* PHY address access port */
#define SDHC_CDNS_HRS04 0x10

/* PHY data access port */
#define SDHC_CDNS_HRS05 0x14

/* IO Delay Information */
#define SDHC_CDNS_HRS07               0x1C
#define SDHC_CDNS_HRS07_RW_COMPENSATE GENMASK(20, 16)
#define SDHC_CDNS_HRS07_IDELAY_VAL    GENMASK(4, 0)

/* PHY Control and Status */
#define SDHC_CDNS_HRS09                   0x24
#define SDHC_CDNS_HRS09_RDDATA_EN         BIT(16)
#define SDHC_CDNS_HRS09_RDCMD_EN          BIT(15)
#define SDHC_CDNS_HRS09_EXTENDED_WR_MODE  BIT(3)
#define SDHC_CDNS_HRS09_EXTENDED_RD_MODE  BIT(2)
#define SDHC_CDNS_HRS09_PHY_INIT_COMPLETE BIT(1)
#define SDHC_CDNS_HRS09_PHY_SW_RESET      BIT(0)

/* SDCLK adjustment */
#define SDHC_CDNS_HRS10            0x28
#define SDHC_CDNS_HRS10_HCSDCLKADJ GENMASK(19, 16)

/* CMD/DAT output delay */
#define SDHC_CDNS_HRS16 0x40

/* PHY Special Function Registers */
#define COMBOPHY_DQ_TIMING_REG_ADDR       0x2000
#define COMBOPHY_DQS_TIMING_REG_ADDR      0x2004
#define COMBOPHY_GATE_LPBK_CTRL_REG_ADDR  0x2008
#define COMBOPHY_DLL_MASTER_CTRL_REG_ADDR 0x200C
#define COMBOPHY_DLL_SLAVE_CTRL_REG_ADDR  0x2010

#define SDHC_CDNS_PHY_CFG_NUM  5
#define SDHC_CDNS_CTRL_CFG_NUM 4

struct sdhc_cdns6_config {
	uint32_t sd_ds_phy_cfg[SDHC_CDNS_PHY_CFG_NUM];
	uint32_t sd_hs_phy_cfg[SDHC_CDNS_PHY_CFG_NUM];
	uint32_t emmc_sdr_phy_cfg[SDHC_CDNS_PHY_CFG_NUM];
	uint32_t emmc_ddr_phy_cfg[SDHC_CDNS_PHY_CFG_NUM];
	uint32_t emmc_hs200_phy_cfg[SDHC_CDNS_PHY_CFG_NUM];
	uint32_t emmc_hs400_phy_cfg[SDHC_CDNS_PHY_CFG_NUM];

	uint32_t sd_ds_ctrl_cfg[SDHC_CDNS_CTRL_CFG_NUM];
	uint32_t sd_hs_ctrl_cfg[SDHC_CDNS_CTRL_CFG_NUM];
	uint32_t emmc_sdr_ctrl_cfg[SDHC_CDNS_CTRL_CFG_NUM];
	uint32_t emmc_ddr_ctrl_cfg[SDHC_CDNS_CTRL_CFG_NUM];
	uint32_t emmc_hs200_ctrl_cfg[SDHC_CDNS_CTRL_CFG_NUM];
	uint32_t emmc_hs400_ctrl_cfg[SDHC_CDNS_CTRL_CFG_NUM];
};

struct sdhc_cdns6_data {
	uint32_t data;
};

static inline void sdhc_cdns6_write_phy_reg(const struct device *dev, uint32_t addr, uint32_t val)
{
	sys_write32(addr, DEVICE_MMIO_NAMED_GET(dev, host) + SDHC_CDNS_HRS04);
	sys_write32(val, DEVICE_MMIO_NAMED_GET(dev, host) + SDHC_CDNS_HRS05);
}

static int sdhc_cdns6_phy_dll_switch(const struct device *dev, bool reset)
{
	mem_addr_t reg = DEVICE_MMIO_NAMED_GET(dev, host) + SDHC_CDNS_HRS09;
	int ret = 0;

	/*
	 * PHY_SW_RESET is active-low: clearing it asserts reset,
	 * setting it releases reset.
	 */
	if (reset) {
		sys_clear_bits(reg, SDHC_CDNS_HRS09_PHY_SW_RESET);
	} else {
		sys_set_bits(reg, SDHC_CDNS_HRS09_PHY_SW_RESET);

		/* Wait until HRS09.PHY_INIT_COMPLETE is set, within 3000us */
		if (!WAIT_FOR((sys_read32(reg) & SDHC_CDNS_HRS09_PHY_INIT_COMPLETE), 3000,
			      k_busy_wait(1))) {
			ret = -ETIMEDOUT;
		}
	}

	return ret;
}

static int sdhc_cdns6_phy_config(const struct device *dev)
{
	const struct sdhc_cdns6_config *cfg = SDHC_CDNS_QUIRK_CONFIG(dev);
	struct sdhc_cdns_data *slot_data = dev->data;
	mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, host);
	const uint32_t *phy_cfg;
	const uint32_t *ctrl_cfg;
	bool is_tuned = true;
	int ret;

	switch (slot_data->timing_mode) {
	case SDHC_TIMING_SDR12:
	case SDHC_TIMING_LEGACY:
		phy_cfg = cfg->sd_ds_phy_cfg;
		ctrl_cfg = cfg->sd_ds_ctrl_cfg;
		is_tuned = false;
		break;

	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR25:
		phy_cfg = cfg->sd_hs_phy_cfg;
		ctrl_cfg = cfg->sd_hs_ctrl_cfg;
		break;

	case SDHC_TIMING_SDR50:
		phy_cfg = cfg->emmc_sdr_phy_cfg;
		ctrl_cfg = cfg->emmc_sdr_ctrl_cfg;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		phy_cfg = cfg->emmc_ddr_phy_cfg;
		ctrl_cfg = cfg->emmc_ddr_ctrl_cfg;
		break;

	case SDHC_TIMING_SDR104:
	case SDHC_TIMING_HS200:
		phy_cfg = cfg->emmc_hs200_phy_cfg;
		ctrl_cfg = cfg->emmc_hs200_ctrl_cfg;
		break;

	case SDHC_TIMING_HS400:
		phy_cfg = cfg->emmc_hs400_phy_cfg;
		ctrl_cfg = cfg->emmc_hs400_ctrl_cfg;
		break;

	default:
		return -EINVAL;
	}

	/* Switch On the DLL Reset */
	(void)sdhc_cdns6_phy_dll_switch(dev, true);

	sdhc_cdns6_write_phy_reg(dev, COMBOPHY_DQS_TIMING_REG_ADDR, phy_cfg[0]);
	sdhc_cdns6_write_phy_reg(dev, COMBOPHY_GATE_LPBK_CTRL_REG_ADDR, phy_cfg[1]);
	sdhc_cdns6_write_phy_reg(dev, COMBOPHY_DLL_MASTER_CTRL_REG_ADDR, phy_cfg[4]);

	if (is_tuned) {
		sdhc_cdns6_write_phy_reg(dev, COMBOPHY_DLL_SLAVE_CTRL_REG_ADDR, 0);
	} else {
		sdhc_cdns6_write_phy_reg(dev, COMBOPHY_DLL_SLAVE_CTRL_REG_ADDR, phy_cfg[2]);
	}

	/* Switch Off the DLL Reset */
	ret = sdhc_cdns6_phy_dll_switch(dev, false);
	if (ret != 0) {
		return ret;
	}

	/* Set PHY DQ TIMING control register */
	sdhc_cdns6_write_phy_reg(dev, COMBOPHY_DQ_TIMING_REG_ADDR, phy_cfg[3]);

	/* Set HRS09 register */
	sys_clear_bits(base + SDHC_CDNS_HRS09,
		       SDHC_CDNS_HRS09_EXTENDED_WR_MODE | SDHC_CDNS_HRS09_EXTENDED_RD_MODE |
			       SDHC_CDNS_HRS09_RDDATA_EN | SDHC_CDNS_HRS09_RDCMD_EN);
	sys_set_bits(base + SDHC_CDNS_HRS09, ctrl_cfg[0]);

	/* Set HRS10 register */
	sys_clear_bits(base + SDHC_CDNS_HRS10, SDHC_CDNS_HRS10_HCSDCLKADJ);
	sys_set_bits(base + SDHC_CDNS_HRS10, ctrl_cfg[1]);

	/* Set HRS16 register */
	sys_write32(ctrl_cfg[2], base + SDHC_CDNS_HRS16);

	/* Set HRS07 register */
	sys_write32(ctrl_cfg[3], base + SDHC_CDNS_HRS07);

	return 0;
}

static int sdhc_cdns6_phy_init(const struct device *dev)
{
	struct sdhc_cdns_data *slot_data = dev->data;

	slot_data->timing_mode = SDHC_TIMING_LEGACY;
	return sdhc_cdns6_phy_config(dev);
}

/*
 * Values are taken from IP documents and calc_setting.py script.
 */
#define SDHC_CDNS6_PHY_CTRL_CFGS_INIT                                                              \
	.sd_ds_phy_cfg = {0x00780000, 0x81A40040, 0x00000000, 0x00000001, 0x00800004},             \
	.sd_hs_phy_cfg = {0x00780000, 0x81A40040, 0x00000000, 0x00000001, 0x00800004},             \
	.emmc_sdr_phy_cfg = {0x00380004, 0x01A00040, 0x00000000, 0x00000001, 0x00800004},          \
	.emmc_ddr_phy_cfg = {0x00380004, 0x01A00040, 0x00000000, 0x10000001, 0x00800004},          \
	.emmc_hs200_phy_cfg = {0x00380004, 0x01A00040, 0x00DADA00, 0x00000001, 0x00000004},        \
	.emmc_hs400_phy_cfg = {0x00280004, 0x01A00040, 0x00DAD800, 0x00000001, 0x00000004},        \
                                                                                                   \
	.sd_ds_ctrl_cfg = {0x0001800C, 0x00020000, 0x00000000, 0x00080000},                        \
	.sd_hs_ctrl_cfg = {0x0001800C, 0x00030000, 0x00000000, 0x00080000},                        \
	.emmc_sdr_ctrl_cfg = {0x0001800C, 0x00030000, 0x00000000, 0x00080000},                     \
	.emmc_ddr_ctrl_cfg = {0x0001800C, 0x00020000, 0x11000001, 0x00090001},                     \
	.emmc_hs200_ctrl_cfg = {0x00018000, 0x00080000, 0x00000000, 0x00090000},                   \
	.emmc_hs400_ctrl_cfg = {0x00018000, 0x00080000, 0x11000000, 0x00080000}


#define QUIRK_SDHC_CDNS6_DEFINE(n)                                                                 \
	static struct sdhc_cdns6_data sdhc_cdns_quirk_data_##n;                                    \
	                                                                                           \
	static const struct sdhc_cdns6_config sdhc_cdns_quirk_config_##n = {                       \
		SDHC_CDNS6_PHY_CTRL_CFGS_INIT,                                                     \
	};                                                                                         \
	                                                                                           \
	static const struct sdhc_cdns_quirks sdhc_cdns_quirks_##n = {                              \
		.phy_init = sdhc_cdns6_phy_init,                                                   \
		.phy_config = sdhc_cdns6_phy_config,                                               \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_SDHC_CDNS6_DEFINE)

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_CDNS6_H */
