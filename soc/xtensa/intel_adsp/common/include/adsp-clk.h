/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_

#include <stdint.h>

struct cavs_clock_info {
	uint32_t default_freq;
	uint32_t current_freq;
	uint32_t lowest_freq;
};

void cavs_clock_init(void);

/** @brief Set cAVS clock frequency
 *
 * Set xtensa core clock speed.
 *
 * @param freq Clock frequency index to be set
 *
 * @return 0 on success, -EINVAL if freq_idx is not valid
 */
int cavs_clock_set_freq(uint32_t freq_idx);

/** @brief Get list of cAVS clock information
 *
 * Returns an array of clock information, one for each core.
 *
 * @return array with clock information
 */
struct cavs_clock_info *cavs_clocks_get(void);

/* Device tree defined constants */
#define CAVS_SHIM_BASE          DT_REG_ADDR(DT_NODELABEL(shim))
#define CAVS_SHIM_CLKCTL        (*((volatile uint32_t *)(CAVS_SHIM_BASE + 0x78)))

#define CAVS_CLOCK_FREQ_ENC     DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_enc)
#define CAVS_CLOCK_FREQ_MASK    DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_mask)
#define CAVS_CLOCK_FREQ_LEN     DT_PROP_LEN(DT_NODELABEL(clkctl), adsp_clkctl_freq_enc)

#define CAVS_CLOCK_FREQ_DEFAULT DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_default)
#define CAVS_CLOCK_FREQ_LOWEST  DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_freq_lowest)

#define CAVS_CLOCK_FREQ(name)   DT_PROP(DT_NODELABEL(clkctl), adsp_clkctl_clk_##name)

#if DT_PROP(DT_NODELABEL(clkctl), wovcro_supported)
#define CAVS_CLOCK_HAS_WOVCRO
#endif

#define CAVS_CLOCK_FREQ_LPRO  CAVS_CLOCK_FREQ(lpro)
#define CAVS_CLOCK_FREQ_HPRO  CAVS_CLOCK_FREQ(hpro)
#ifdef CAVS_CLOCK_HAS_WOVCRO
#define CAVS_CLOCK_FREQ_WOVCRO  CAVS_CLOCK_FREQ(wovcro)
#endif

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_ */
