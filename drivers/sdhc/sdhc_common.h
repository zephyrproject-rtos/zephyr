/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_COMMON_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_COMMON_H_

#include <zephyr/drivers/sdhc.h>

/**
 * @brief SD host controller common configuration
 */
struct sdhc_common_config {
	uint32_t max_current_330;       /**< Max current of 3.3V */
	uint32_t max_current_300;       /**< Max current of 3.0V */
	uint32_t max_current_180;       /**< Max current of 1.8V */
	uint32_t min_bus_freq;          /**< Min bus frequency */
	uint32_t max_bus_freq;          /**< Max bus frequency */
	uint32_t power_delay_ms;        /**< Delay for power up */
	uint8_t bus_width;              /**< Data bus width */
	bool mmc_hs200_1_8v;            /**< HS200 support */
	bool mmc_hs400_1_8v;            /**< HS400 support */
	bool mmc_hs400_enhanced_strobe; /**< HS400 enhanced strobe mode support */
};

/**
 * @brief SD host controller common configuration initialization for instance
 */
#define SDHC_COMMON_CONFIG_DT_INST_INIT(n) \
	{ \
		.max_current_330 = DT_INST_PROP(n, max_current_330),                     \
		.max_current_300 = DT_INST_PROP(n, max_current_300),                     \
		.max_current_180 = DT_INST_PROP(n, max_current_180),                     \
		.min_bus_freq = DT_INST_PROP(n, min_bus_freq),                           \
		.max_bus_freq = DT_INST_PROP(n, max_bus_freq),                           \
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),                       \
		.bus_width = DT_INST_PROP_OR(n, bus_width, 1),                           \
		.mmc_hs200_1_8v = DT_INST_PROP(n, mmc_hs200_1_8v),                       \
		.mmc_hs400_1_8v = DT_INST_PROP(n, mmc_hs400_1_8v),                       \
		.mmc_hs400_enhanced_strobe = DT_INST_PROP(n, mmc_hs400_enhanced_strobe), \
	}

/**
 * @brief Convert enum sd_voltage to a human-readable string
 *
 * @param voltage Enum value
 * @return String like "1.8V" or "3.3V", or "Unknown" for out-of-range values
 */
static inline const char *sd_voltage_str(enum sd_voltage voltage)
{
	static const char *const sig_vol_str[] = {
		[0] = "Unset",		 [SD_VOL_3_3_V] = "3.3V", [SD_VOL_3_0_V] = "3.0V",
		[SD_VOL_1_8_V] = "1.8V", [SD_VOL_1_2_V] = "1.2V",
	};

	if (voltage >= 0 && voltage < ARRAY_SIZE(sig_vol_str)) {
		return sig_vol_str[voltage];
	} else {
		return "Unknown";
	}
}

/**
 * @brief Convert enum timing to a human-readable string
 *
 * @param timing Enum value
 * @return String like "LEGACY" or "HS200", or "Unknown" for out-of-range values
 */
static inline const char *sdhc_timing_mode_str(enum sdhc_timing_mode timing)
{
	static const char *const timing_str[] = {
		[0] = "Unset",
		[SDHC_TIMING_LEGACY] = "LEGACY",
		[SDHC_TIMING_HS] = "HS",
#ifdef CONFIG_SDHC_SUPPORTS_UHS
		[SDHC_TIMING_SDR12] = "SDR12",
		[SDHC_TIMING_SDR25] = "SDR25",
		[SDHC_TIMING_SDR50] = "SDR50",
		[SDHC_TIMING_SDR104] = "SDR104",
		[SDHC_TIMING_DDR50] = "DDR50",
		[SDHC_TIMING_DDR52] = "DDR52",
		[SDHC_TIMING_HS200] = "HS200",
		[SDHC_TIMING_HS400] = "HS400",
#endif /* CONFIG_SDHC_SUPPORTS_UHS */
	};

	if (timing >= 0 && timing < ARRAY_SIZE(timing_str)) {
		return timing_str[timing];
	} else {
		return "Unknown";
	}
}

/* SDHC common DT properties initialization */
void sdhc_common_dt_props_init(struct sdhc_host_props *props, const struct sdhc_common_config *cfg);

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_COMMON_H_ */
