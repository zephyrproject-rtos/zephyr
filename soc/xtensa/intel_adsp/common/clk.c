/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>

#include <adsp-clk.h>
#include <adsp_shim.h>

static struct cavs_clock_info platform_clocks[CONFIG_MP_NUM_CPUS];
static struct k_spinlock lock;

int cavs_clock_freq_enc[] = CAVS_CLOCK_FREQ_ENC;
#ifndef CONFIG_SOC_INTEL_CAVS_V15
int cavs_clock_freq_mask[] = CAVS_CLOCK_FREQ_MASK;
#endif

#ifdef CONFIG_SOC_INTEL_CAVS_V15
static void select_cpu_clock_hw(uint32_t freq)
{
	uint8_t cpu_id = _current_cpu->id;
	uint32_t enc = cavs_clock_freq_enc[freq] << (8 + cpu_id * 2);
	uint32_t mask = CAVS_CLKCTL_DPCS_MASK(cpu_id);

	CAVS_SHIM_CLKCTL &= ~CAVS_CLKCTL_HDCS;
	CAVS_SHIM_CLKCTL = (CAVS_SHIM_CLKCTL & ~mask) | (enc & mask);
}
#else
static void select_cpu_clock_hw(uint32_t freq_idx)
{
	uint32_t enc = cavs_clock_freq_enc[freq_idx];
	uint32_t status_mask = cavs_clock_freq_mask[freq_idx];

	/* Request clock */
	CAVS_SHIM_CLKCTL |= enc;

	/* Wait for requested clock to be on */
	while ((CAVS_SHIM_CLKCTL & status_mask) != status_mask) {
		k_busy_wait(10);
	}

	/* Switch to requested clock */
	CAVS_SHIM_CLKCTL = (CAVS_SHIM_CLKCTL & ~CAVS_CLKCTL_OSC_SOURCE_MASK) |
			    enc;

	/* Release other clocks */
	CAVS_SHIM_CLKCTL &= ~CAVS_CLKCTL_OSC_REQUEST_MASK | enc;
}
#endif

int cavs_clock_set_freq(uint32_t freq_idx)
{
	k_spinlock_key_t k;
	int i;

	if (freq_idx >= CAVS_CLOCK_FREQ_LEN) {
		return -EINVAL;
	}

	k = k_spin_lock(&lock);

	select_cpu_clock_hw(freq_idx);
	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		platform_clocks[i].current_freq = freq_idx;
	}

	k_spin_unlock(&lock, k);

	return 0;
}

struct cavs_clock_info *cavs_clocks_get(void)
{
	return platform_clocks;
}

void cavs_clock_init(void)
{
	uint32_t platform_lowest_freq_idx = CAVS_CLOCK_FREQ_LOWEST;
	int i;

#ifdef CAVS_CLOCK_HAS_WOVCRO
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_WOVCRO;
	if (CAVS_SHIM.clkctl & CAVS_CLKCTL_WOVCRO) {
		CAVS_SHIM.clkctl = CAVS_SHIM.clkctl & ~CAVS_CLKCTL_WOVCRO;
	} else {
		platform_lowest_freq_idx = CAVS_CLOCK_FREQ_LPRO;
	}
#endif

	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		platform_clocks[i].default_freq = CAVS_CLOCK_FREQ_DEFAULT;
		platform_clocks[i].current_freq = CAVS_CLOCK_FREQ_DEFAULT;
		platform_clocks[i].lowest_freq = platform_lowest_freq_idx;
	}
}
