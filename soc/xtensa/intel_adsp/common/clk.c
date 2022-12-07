/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <adsp_clk.h>
#include <adsp_shim.h>

static struct adsp_clock_info platform_clocks[CONFIG_MP_MAX_NUM_CPUS];
static struct k_spinlock lock;

int adsp_clock_freq_enc[] = ADSP_CLOCK_FREQ_ENC;
#ifndef CONFIG_SOC_INTEL_CAVS_V15
int adsp_clock_freq_mask[] = ADSP_CLOCK_FREQ_MASK;
#endif

#ifdef CONFIG_SOC_INTEL_CAVS_V15
static void select_cpu_clock_hw(uint32_t freq)
{
	uint8_t cpu_id = _current_cpu->id;
	uint32_t enc = adsp_clock_freq_enc[freq] << (8 + cpu_id * 2);
	uint32_t mask = CAVS_CLKCTL_DPCS_MASK(cpu_id);

	ADSP_CLKCTL &= ~CAVS_CLKCTL_HDCS;
	ADSP_CLKCTL = (ADSP_CLKCTL & ~mask) | (enc & mask);
}
#else
static void select_cpu_clock_hw(uint32_t freq_idx)
{
	uint32_t enc = adsp_clock_freq_enc[freq_idx];
	uint32_t status_mask = adsp_clock_freq_mask[freq_idx];

	/* Request clock */
	ADSP_CLKCTL |= enc;

	/* Wait for requested clock to be on */
	while ((ADSP_CLKCTL & status_mask) != status_mask) {
		k_busy_wait(10);
	}

	/* Switch to requested clock */
	ADSP_CLKCTL = (ADSP_CLKCTL & ~ADSP_CLKCTL_OSC_SOURCE_MASK) |
			    enc;

	/* Release other clocks */
	ADSP_CLKCTL &= ~ADSP_CLKCTL_OSC_REQUEST_MASK | enc;
}
#endif

int adsp_clock_set_freq(uint32_t freq_idx)
{
	k_spinlock_key_t k;
	int i;

	if (freq_idx >= ADSP_CLOCK_FREQ_LEN) {
		return -EINVAL;
	}

	k = k_spin_lock(&lock);

	select_cpu_clock_hw(freq_idx);

	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		platform_clocks[i].current_freq = freq_idx;
	}

	k_spin_unlock(&lock, k);

	return 0;
}

struct adsp_clock_info *adsp_clocks_get(void)
{
	return platform_clocks;
}

void adsp_clock_init(void)
{
	uint32_t platform_lowest_freq_idx = ADSP_CLOCK_FREQ_LOWEST;
	int i;

#ifdef ADSP_CLOCK_HAS_WOVCRO
#ifdef CONFIG_SOC_SERIES_INTEL_ACE
	ACE_DfPMCCU.dfclkctl |= ACE_CLKCTL_WOVCRO;
	if (ACE_DfPMCCU.dfclkctl & ACE_CLKCTL_WOVCRO) {
		ACE_DfPMCCU.dfclkctl = ACE_DfPMCCU.dfclkctl & ~ACE_CLKCTL_WOVCRO;
	} else {
		platform_lowest_freq_idx = ADSP_CLOCK_FREQ_LPRO;
	}
#else
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_WOVCRO;
	if (CAVS_SHIM.clkctl & CAVS_CLKCTL_WOVCRO) {
		CAVS_SHIM.clkctl = CAVS_SHIM.clkctl & ~CAVS_CLKCTL_WOVCRO;
	} else {
		platform_lowest_freq_idx = ADSP_CLOCK_FREQ_LPRO;
	}
#endif /* CONFIG_SOC_SERIES_INTEL_ACE */
#endif /* ADSP_CLOCK_HAS_WOVCRO */

	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		platform_clocks[i].default_freq = ADSP_CLOCK_FREQ_DEFAULT;
		platform_clocks[i].current_freq = ADSP_CLOCK_FREQ_DEFAULT;
		platform_clocks[i].lowest_freq = platform_lowest_freq_idx;
	}
}
