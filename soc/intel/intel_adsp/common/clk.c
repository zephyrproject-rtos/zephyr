/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <adsp_clk.h>
#include <adsp_shim.h>

static struct adsp_cpu_clock_info platform_cpu_clocks[CONFIG_MP_MAX_NUM_CPUS];
static struct k_spinlock lock;

int adsp_clock_freq_enc[] = ADSP_CPU_CLOCK_FREQ_ENC;
int adsp_clock_freq_mask[] = ADSP_CPU_CLOCK_FREQ_MASK;

#define HW_CLK_CHANGE_TIMEOUT_USEC 10000

static void select_cpu_clock_hw(uint32_t freq_idx)
{
	uint32_t enc = adsp_clock_freq_enc[freq_idx];

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	uint32_t clk_ctl = ADSP_CLKCTL;

	clk_ctl &= ~ADSP_CLKCTL_OSC_SOURCE_MASK;
	clk_ctl |= (enc & ADSP_CLKCTL_OSC_SOURCE_MASK);

	ADSP_CLKCTL = clk_ctl;
#else
	uint32_t status_mask = adsp_clock_freq_mask[freq_idx];

	/* Request clock */
	ADSP_CLKCTL |= enc;

	/* Wait for requested clock to be on */
	if (!WAIT_FOR((ADSP_CLKCTL & status_mask) == status_mask,
		      HW_CLK_CHANGE_TIMEOUT_USEC, k_busy_wait(1))) {
		k_panic();
	}

	/* Switch to requested clock */
	ADSP_CLKCTL = (ADSP_CLKCTL & ~ADSP_CLKCTL_OSC_SOURCE_MASK) |
			    enc;

	/* Release other clocks */
	ADSP_CLKCTL &= ~ADSP_CLKCTL_OSC_REQUEST_MASK | enc;
#endif
}

int adsp_clock_set_cpu_freq(uint32_t freq_idx)
{
	k_spinlock_key_t k;
	int i;

	if (freq_idx >= ADSP_CPU_CLOCK_FREQ_LEN) {
		return -EINVAL;
	}

	k = k_spin_lock(&lock);

	select_cpu_clock_hw(freq_idx);

	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		platform_cpu_clocks[i].current_freq = freq_idx;
	}

	k_spin_unlock(&lock, k);

	return 0;
}

struct adsp_cpu_clock_info *adsp_cpu_clocks_get(void)
{
	return platform_cpu_clocks;
}

void adsp_clock_init(void)
{
	uint32_t platform_lowest_freq_idx = ADSP_CPU_CLOCK_FREQ_LOWEST;
	int i;

#ifdef ADSP_CLOCK_HAS_WOVCRO
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	ACE_DfPMCCU.dfclkctl |= ACE_CLKCTL_WOVCRO;
	if (ACE_DfPMCCU.dfclkctl & ACE_CLKCTL_WOVCRO) {
		ACE_DfPMCCU.dfclkctl = ACE_DfPMCCU.dfclkctl & ~ACE_CLKCTL_WOVCRO;
	} else {
		platform_lowest_freq_idx = ADSP_CPU_CLOCK_FREQ_IPLL;
	}
#else
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_WOVCRO;
	if (CAVS_SHIM.clkctl & CAVS_CLKCTL_WOVCRO) {
		CAVS_SHIM.clkctl = CAVS_SHIM.clkctl & ~CAVS_CLKCTL_WOVCRO;
	} else {
		platform_lowest_freq_idx = ADSP_CPU_CLOCK_FREQ_LPRO;
	}
#endif /* CONFIG_SOC_SERIES_INTEL_ADSP_ACE */
#endif /* ADSP_CLOCK_HAS_WOVCRO */

	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		platform_cpu_clocks[i].default_freq = ADSP_CPU_CLOCK_FREQ_DEFAULT;
		platform_cpu_clocks[i].current_freq = ADSP_CPU_CLOCK_FREQ_DEFAULT;
		platform_cpu_clocks[i].lowest_freq = platform_lowest_freq_idx;
	}
}

struct adsp_clock_source_desc adsp_clk_src_info[ADSP_CLOCK_SOURCE_COUNT] = {
#ifndef CONFIG_DAI_DMIC_HW_IOCLK
	[ADSP_CLOCK_SOURCE_XTAL_OSC] = { DT_PROP(DT_NODELABEL(sysclk), clock_frequency) },
#else
	/* Temporarily use the values from the configuration until set xtal frequency via ipc
	 * support is added.
	 */
	[ADSP_CLOCK_SOURCE_XTAL_OSC] = { CONFIG_DAI_DMIC_HW_IOCLK },
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(audioclk), okay)
	[ADSP_CLOCK_SOURCE_AUDIO_CARDINAL] = { DT_PROP(DT_NODELABEL(audioclk), clock_frequency) },
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pllclk), okay)
	[ADSP_CLOCK_SOURCE_AUDIO_PLL_FIXED] = { DT_PROP(DT_NODELABEL(pllclk), clock_frequency) },
#endif
	[ADSP_CLOCK_SOURCE_MLCK_INPUT] = { 0 },
#ifdef ADSP_CLOCK_HAS_WOVCRO
	[ADSP_CLOCK_SOURCE_WOV_RING_OSC] = { DT_PROP(DT_NODELABEL(sysclk), clock_frequency) },
#endif
};

bool adsp_clock_source_is_supported(int source)
{
	if (source < 0 || source >= ADSP_CLOCK_SOURCE_COUNT) {
		return false;
	}

	return !!adsp_clk_src_info[source].frequency;
}

uint32_t adsp_clock_source_frequency(int source)
{
	if (source < 0 || source >= ADSP_CLOCK_SOURCE_COUNT) {
		return 0;
	}

	return adsp_clk_src_info[source].frequency;
}
