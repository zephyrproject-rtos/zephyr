/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_

#include <stdint.h>
#include <adsp_shim.h>

struct adsp_clock_info {
	uint32_t default_freq;
	uint32_t current_freq;
	uint32_t lowest_freq;
};

void adsp_clock_init(void);

/** @brief Set cAVS clock frequency
 *
 * Set xtensa core clock speed.
 *
 * @param freq Clock frequency index to be set
 *
 * @return 0 on success, -EINVAL if freq_idx is not valid
 */
int adsp_clock_set_freq(uint32_t freq_idx);

/** @brief Get list of cAVS clock information
 *
 * Returns an array of clock information, one for each core.
 *
 * @return array with clock information
 */
struct adsp_clock_info *adsp_clocks_get(void);

/* Device tree defined constants */
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
#define ADSP_CLKCTL		ACE_DfPMCCU.dfclkctl
#else
#define ADSP_CLKCTL		CAVS_SHIM.clkctl
#endif

#define ADSP_CLOCK_FREQ_ENC     DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_enc)
#define ADSP_CLOCK_FREQ_MASK    DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_mask)
#define ADSP_CLOCK_FREQ_LEN     DT_PROP_LEN(DT_NODELABEL(clkctl), adsp_clkctl_freq_enc)

#define ADSP_CLOCK_FREQ_DEFAULT DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_default)
#define ADSP_CLOCK_FREQ_LOWEST  DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_lowest)

#define ADSP_CLOCK_FREQ(name)   DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_clk_##name)

#if DT_PROP(DT_NODELABEL(clkctl), wovcro_supported)
#define ADSP_CLOCK_HAS_WOVCRO
#endif

#define ADSP_CLOCK_FREQ_LPRO  ADSP_CLOCK_FREQ(lpro)
#define ADSP_CLOCK_FREQ_HPRO  ADSP_CLOCK_FREQ(hpro)
#ifdef ADSP_CLOCK_HAS_WOVCRO
#define ADSP_CLOCK_FREQ_WOVCRO  ADSP_CLOCK_FREQ(wovcro)
#endif

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_ */
