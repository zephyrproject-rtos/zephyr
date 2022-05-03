/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS25_CLK_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS25_CLK_H_

#include <cavs-shim.h>

enum cavs_clock_freq {
	CAVS_CLOCK_FREQ_WOVCRO,
	CAVS_CLOCK_FREQ_LPRO,
	CAVS_CLOCK_FREQ_HPRO
};

#define CAVS_CLOCK_FREQ_DEFAULT CAVS_CLOCK_FREQ_HPRO
#define CAVS_CLOCK_FREQ_LOWEST  CAVS_CLOCK_FREQ_WOVCRO

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS25_CLK_H_ */
