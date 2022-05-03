/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>

#include <cavs-clk.h>
#include <cavs-shim.h>

static struct cavs_clock_info platform_clocks[CONFIG_MP_NUM_CPUS];
static struct k_spinlock lock;

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V15
const uint32_t cavs_clock_freq_enc[] = {
	0x3,
	0x1,
	0x0
};
#elif CONFIG_SOC_SERIES_INTEL_CAVS_V18
const uint32_t cavs_clock_freq_enc[] = {
	CAVS_CLKCTL_RLROSCC | CAVS_CLKCTL_LMCS,
	CAVS_CLKCTL_RHROSCC | CAVS_CLKCTL_OCS | CAVS_CLKCTL_LMCS
};

const uint32_t cavs_clock_freq_status_mask[] = {
	CAVS_CLKCTL_RLROSCC,
	CAVS_CLKCTL_RHROSCC
};
#elif CONFIG_SOC_SERIES_INTEL_CAVS_V20
const uint32_t cavs_clock_freq_enc[] = {
	CAVS_CLKCTL_RLROSCC | CAVS_CLKCTL_LMCS,
	CAVS_CLKCTL_RHROSCC | CAVS_CLKCTL_OCS | CAVS_CLKCTL_LMCS
};

const uint32_t cavs_clock_freq_status_mask[] = {
	CAVS_CLKCTL_RLROSCC,
	CAVS_CLKCTL_RHROSCC
};
#elif CONFIG_SOC_SERIES_INTEL_CAVS_V25
const uint32_t cavs_clock_freq_enc[] = {
	CAVS_CLKCTL_WOVCROSC | CAVS_CLKCTL_WOVCRO | CAVS_CLKCTL_LMCS,
	CAVS_CLKCTL_RLROSCC | CAVS_CLKCTL_LMCS,
	CAVS_CLKCTL_RHROSCC | CAVS_CLKCTL_OCS | CAVS_CLKCTL_LMCS
};

const uint32_t cavs_clock_freq_status_mask[] = {
	CAVS_CLKCTL_WOVCRO,
	CAVS_CLKCTL_RLROSCC,
	CAVS_CLKCTL_RHROSCC
};
#endif

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V15
static void select_cpu_clock_hw(enum cavs_clock_freq freq)
{
	uint8_t cpu_id = _current_cpu->id;
	uint32_t enc = cavs_clock_freq_enc[freq] << (8 + cpu_id * 2);
	uint32_t mask = CAVS15_CLKCTL_DPCS_MASK(cpu_id);

	CAVS_SHIM.clkctl &= ~CAVS15_CLKCTL_HDCS;
	CAVS_SHIM.clkctl = (CAVS_SHIM.clkctl & ~mask) | (enc & mask);
}
#else
static void select_cpu_clock_hw(enum cavs_clock_freq freq)
{
	uint32_t enc = cavs_clock_freq_enc[freq];
	uint32_t status_mask = cavs_clock_freq_status_mask[freq];

	/* Request clock */
	CAVS_SHIM.clkctl |= enc;

	/* Wait for requested clock to be on */
	while ((CAVS_SHIM.clkctl & status_mask) != status_mask) {
		k_busy_wait(10);
	}

	/* Switch to requested clock */
	CAVS_SHIM.clkctl = (CAVS_SHIM.clkctl & ~CAVS_CLKCTL_OSC_SOURCE_MASK) |
			    enc;

	/* Release other clocks */
	CAVS_SHIM.clkctl &= ~CAVS_CLKCTL_OSC_REQUEST_MASK | enc;
}
#endif

void cavs_clock_set_freq(enum cavs_clock_freq freq)
{
	k_spinlock_key_t k;
	int i;

	k = k_spin_lock(&lock);

	select_cpu_clock_hw(freq);
	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++)
		platform_clocks[i].current_freq = freq;

	k_spin_unlock(&lock, k);
}

struct cavs_clock_info *cavs_clocks_get(void)
{
	return platform_clocks;
}

void cavs_clock_init(void)
{
	enum cavs_clock_freq platform_lowest_freq_idx = CAVS_CLOCK_FREQ_LOWEST;
	int i;

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_WOVCRO;
	if (CAVS_SHIM.clkctl & CAVS_CLKCTL_WOVCRO)
		CAVS_SHIM.clkctl = CAVS_SHIM.clkctl & ~CAVS_CLKCTL_WOVCRO;
	else
		platform_lowest_freq_idx = CAVS_CLOCK_FREQ_WOVCRO;
#endif

	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		platform_clocks[i].default_freq = CAVS_CLOCK_FREQ_DEFAULT;
		platform_clocks[i].current_freq = CAVS_CLOCK_FREQ_DEFAULT;
		platform_clocks[i].lowest_freq = platform_lowest_freq_idx;
	}
}
