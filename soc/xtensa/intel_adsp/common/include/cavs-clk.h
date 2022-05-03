/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_

#include <stdint.h>

#include <soc/clk.h>

enum cavs_clock_freq;

struct cavs_clock_info {
	uint32_t default_freq;
	uint32_t current_freq;
	uint32_t lowest_freq;
};

extern const uint32_t cavs_clock_freq_enc[];
extern const uint32_t cavs_clock_freq_status_mask[];

void cavs_clock_init(void);

/** @brief Set cAVS clock frequency
 *
 * Set xtensa core clock speed.
 *
 * @param freq Clock frequency to be set
 */
void cavs_clock_set_freq(enum cavs_clock_freq freq);

/** @brief Get list of cAVS clock information
 *
 * Returns an array of clock information, one for each core.
 *
 * @return array with clock information
 */
struct cavs_clock_info *cavs_clocks_get(void);

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_CLK_H_ */
