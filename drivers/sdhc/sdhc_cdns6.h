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

/*
 * SD6HC host registers (HRS block) that carry PHY delay settings.
 *
 * Only the fields the driver touches are named. Field layout follows the
 * Agilex 5 HPS Technical Reference Manual, Combo DLL PHY section:
 * https://docs.altera.com/r/docs/814346/26.1.1/hard-processor-system-technical-reference-manual-agilex-5-socs/combo-dll-phy
 */

/* Indirect PHY register access: address in HRS04, data in HRS05 */
#define SDHC_CDNS6_HRS04 0x10U
#define SDHC_CDNS6_HRS05 0x14U

/* IO delay */
#define SDHC_CDNS6_HRS07 0x1CU

/* PHY control and status */
#define SDHC_CDNS6_HRS09                   0x24U
#define SDHC_CDNS6_HRS09_RDDATA_EN         BIT(16)
#define SDHC_CDNS6_HRS09_RDCMD_EN          BIT(15)
#define SDHC_CDNS6_HRS09_EXTENDED_WR_MODE  BIT(3)
#define SDHC_CDNS6_HRS09_EXTENDED_RD_MODE  BIT(2)
#define SDHC_CDNS6_HRS09_PHY_INIT_COMPLETE BIT(1)
#define SDHC_CDNS6_HRS09_PHY_SW_RESET      BIT(0)

/* HRS09 bits owned by the timing profile; the reset bits are managed separately */
#define SDHC_CDNS6_HRS09_PROFILE_MASK                                                              \
	(SDHC_CDNS6_HRS09_RDDATA_EN | SDHC_CDNS6_HRS09_RDCMD_EN |                                  \
	 SDHC_CDNS6_HRS09_EXTENDED_WR_MODE | SDHC_CDNS6_HRS09_EXTENDED_RD_MODE)

/* SDCLK start point adjustment */
#define SDHC_CDNS6_HRS10            0x28U
#define SDHC_CDNS6_HRS10_HCSDCLKADJ GENMASK(19, 16)

/* CMD/DAT output delay */
#define SDHC_CDNS6_HRS16 0x40U

/* Combo PHY registers, reached through HRS04/HRS05 */
#define SDHC_CDNS6_PHY_DQ_TIMING             0x2000U
#define SDHC_CDNS6_PHY_DQS_TIMING            0x2004U
#define SDHC_CDNS6_PHY_GATE_LPBK_CTRL        0x2008U
#define SDHC_CDNS6_PHY_DLL_MASTER_CTRL       0x200CU
#define SDHC_CDNS6_PHY_DLL_SLAVE_CTRL        0x2010U
#define SDHC_CDNS6_PHY_CTRL                  0x2080U
#define SDHC_CDNS6_PHY_CTRL_PHONY_DQS_TIMING GENMASK(9, 4)

/* Time for the PHY to report init complete after its reset is released */
#define SDHC_CDNS6_PHY_INIT_TIMEOUT_US 3000U

/*
 * Timing modes share PHY settings in pairs: SD default speed and SDR12 use
 * one profile, high speed and SDR25 the other. A profile holds the five
 * Combo PHY registers and the four HRS registers programmed when the host
 * switches timing mode, taken from the cdns,phy-cfg-* and cdns,ctrl-cfg-*
 * devicetree properties.
 */
enum sdhc_cdns6_profile_id {
	SDHC_CDNS6_PROFILE_SD_DS,
	SDHC_CDNS6_PROFILE_SD_HS,
	SDHC_CDNS6_PROFILE_COUNT,
};

struct sdhc_cdns6_profile {
	uint32_t dqs_timing;
	uint32_t gate_lpbk_ctrl;
	uint32_t dll_slave_ctrl;
	uint32_t dq_timing;
	uint32_t dll_master_ctrl;
	uint32_t hrs09;
	uint32_t hrs10;
	uint32_t hrs16;
	uint32_t hrs07;
};

struct sdhc_cdns6_config {
	struct sdhc_cdns6_profile profiles[SDHC_CDNS6_PROFILE_COUNT];
};

/* Profile for each supported SDHC_TIMING_* value; index 0 is not a timing mode */
static const uint8_t sdhc_cdns6_profile_of_timing[] = {
	[SDHC_TIMING_LEGACY] = SDHC_CDNS6_PROFILE_SD_DS,
	[SDHC_TIMING_SDR12] = SDHC_CDNS6_PROFILE_SD_DS,
	[SDHC_TIMING_HS] = SDHC_CDNS6_PROFILE_SD_HS,
	[SDHC_TIMING_SDR25] = SDHC_CDNS6_PROFILE_SD_HS,
};

static inline mem_addr_t sdhc_cdns6_hrs(const struct device *dev, uint32_t reg)
{
	return DEVICE_MMIO_NAMED_GET(dev, host) + reg;
}

static inline void sdhc_cdns6_hrs_update(const struct device *dev, uint32_t reg, uint32_t mask,
					 uint32_t val)
{
	mem_addr_t addr = sdhc_cdns6_hrs(dev, reg);

	sys_write32((sys_read32(addr) & ~mask) | (val & mask), addr);
}

static inline uint32_t sdhc_cdns6_phy_read(const struct device *dev, uint32_t reg)
{
	sys_write32(reg, sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS04));
	return sys_read32(sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS05));
}

static inline void sdhc_cdns6_phy_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(reg, sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS04));
	sys_write32(val, sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS05));
}

/* HRS09.PHY_SW_RESET is active low: 0 holds the PHY DLL in reset */
static inline void sdhc_cdns6_phy_hold_reset(const struct device *dev)
{
	sys_clear_bits(sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS09), SDHC_CDNS6_HRS09_PHY_SW_RESET);
}

static int sdhc_cdns6_phy_release_reset(const struct device *dev)
{
	mem_addr_t hrs09 = sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS09);
	bool done;

	sys_set_bits(hrs09, SDHC_CDNS6_HRS09_PHY_SW_RESET);

	done = WAIT_FOR((sys_read32(hrs09) & SDHC_CDNS6_HRS09_PHY_INIT_COMPLETE) != 0U,
			SDHC_CDNS6_PHY_INIT_TIMEOUT_US, k_busy_wait(1));

	return done ? 0 : -ETIMEDOUT;
}

static int sdhc_cdns6_apply_profile(const struct device *dev, enum sdhc_cdns6_profile_id id)
{
	const struct sdhc_cdns6_config *cfg = SDHC_CDNS_QUIRK_CONFIG(dev);
	const struct sdhc_cdns6_profile *profile = &cfg->profiles[id];
	uint32_t phy_ctrl;
	int ret;

	/* DLL related PHY registers are only writable while the PHY is held in reset */
	sdhc_cdns6_phy_hold_reset(dev);

	sdhc_cdns6_phy_write(dev, SDHC_CDNS6_PHY_DQS_TIMING, profile->dqs_timing);
	sdhc_cdns6_phy_write(dev, SDHC_CDNS6_PHY_GATE_LPBK_CTRL, profile->gate_lpbk_ctrl);
	sdhc_cdns6_phy_write(dev, SDHC_CDNS6_PHY_DLL_MASTER_CTRL, profile->dll_master_ctrl);
	sdhc_cdns6_phy_write(dev, SDHC_CDNS6_PHY_DLL_SLAVE_CTRL, profile->dll_slave_ctrl);

	phy_ctrl = sdhc_cdns6_phy_read(dev, SDHC_CDNS6_PHY_CTRL);
	phy_ctrl &= ~SDHC_CDNS6_PHY_CTRL_PHONY_DQS_TIMING;
	sdhc_cdns6_phy_write(dev, SDHC_CDNS6_PHY_CTRL, phy_ctrl);

	ret = sdhc_cdns6_phy_release_reset(dev);
	if (ret != 0) {
		return ret;
	}

	sdhc_cdns6_phy_write(dev, SDHC_CDNS6_PHY_DQ_TIMING, profile->dq_timing);

	sdhc_cdns6_hrs_update(dev, SDHC_CDNS6_HRS09, SDHC_CDNS6_HRS09_PROFILE_MASK, profile->hrs09);
	sdhc_cdns6_hrs_update(dev, SDHC_CDNS6_HRS10, SDHC_CDNS6_HRS10_HCSDCLKADJ, profile->hrs10);
	sys_write32(profile->hrs16, sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS16));
	sys_write32(profile->hrs07, sdhc_cdns6_hrs(dev, SDHC_CDNS6_HRS07));

	return 0;
}

static int sdhc_cdns6_phy_init(const struct device *dev)
{
	return sdhc_cdns6_apply_profile(dev, SDHC_CDNS6_PROFILE_SD_DS);
}

static int sdhc_cdns6_phy_config(const struct device *dev)
{
	const struct sdhc_cdns_data *host = dev->data;
	uint32_t timing = host->timing_mode;

	if (timing == 0U || timing >= ARRAY_SIZE(sdhc_cdns6_profile_of_timing)) {
		return -ENOTSUP;
	}

	return sdhc_cdns6_apply_profile(dev, sdhc_cdns6_profile_of_timing[timing]);
}

#define SDHC_CDNS6_PROFILE(n, mode, phy, ctrl)                                                     \
	[SDHC_CDNS6_PROFILE_##mode] = {                                                            \
		.dqs_timing = DT_INST_PROP_BY_IDX(n, phy, 0),                                      \
		.gate_lpbk_ctrl = DT_INST_PROP_BY_IDX(n, phy, 1),                                  \
		.dll_slave_ctrl = DT_INST_PROP_BY_IDX(n, phy, 2),                                  \
		.dq_timing = DT_INST_PROP_BY_IDX(n, phy, 3),                                       \
		.dll_master_ctrl = DT_INST_PROP_BY_IDX(n, phy, 4),                                 \
		.hrs09 = DT_INST_PROP_BY_IDX(n, ctrl, 0),                                          \
		.hrs10 = DT_INST_PROP_BY_IDX(n, ctrl, 1),                                          \
		.hrs16 = DT_INST_PROP_BY_IDX(n, ctrl, 2),                                          \
		.hrs07 = DT_INST_PROP_BY_IDX(n, ctrl, 3),                                          \
	}

#define QUIRK_SDHC_CDNS6_DEFINE(n)                                                                 \
	static const struct sdhc_cdns6_config sdhc_cdns_quirk_config_##n = {                       \
		.profiles = {                                                                      \
			SDHC_CDNS6_PROFILE(n, SD_DS, cdns_phy_cfg_sd_ds, cdns_ctrl_cfg_sd_ds),     \
			SDHC_CDNS6_PROFILE(n, SD_HS, cdns_phy_cfg_sd_hs, cdns_ctrl_cfg_sd_hs),     \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	static const struct sdhc_cdns_quirks sdhc_cdns_quirks_##n = {                              \
		.phy_init = sdhc_cdns6_phy_init,                                                   \
		.phy_config = sdhc_cdns6_phy_config,                                               \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_SDHC_CDNS6_DEFINE)

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_CDNS6_H */
