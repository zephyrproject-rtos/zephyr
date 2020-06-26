/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Tomasz Lauda <tomasz.lauda@linux.intel.com>
 *         Janusz Jankowski <janusz.jankowski@linux.intel.com>
 */
#define RELATIVE_FILE "zephyr/clk.c"

#include <sof/common.h>
#include <sof/drivers/ssp.h>
#include <sof/lib/clk.h>
#include <sof/lib/memory.h>
#include <sof/lib/notifier.h>
#include <sof/sof.h>
#include <sof/spinlock.h>

const struct freq_table platform_cpu_freq[] = {
	{ 32000000, 32000 },
	{ 80000000, 80000 },
	{ 160000000, 160000 },
	{ 320000000, 320000 },
	{ 320000000, 320000 },
	{ 160000000, 160000 },
};

const uint32_t cpu_freq_enc[] = {
	0x6,
	0x2,
	0x1,
	0x4,
	0x0,
	0x5,
};

STATIC_ASSERT(NUM_CPU_FREQ == ARRAY_SIZE(platform_cpu_freq),
	      invalid_number_of_cpu_frequencies);

const struct freq_table platform_ssp_freq[] = {
	{ 24000000, 24000 },
};

const uint32_t platform_ssp_freq_sources[] = {
	0,
};

STATIC_ASSERT(NUM_SSP_FREQ == ARRAY_SIZE(platform_ssp_freq),
	      invalid_number_of_ssp_frequencies);

const struct freq_table *ssp_freq = platform_ssp_freq;
const uint32_t *ssp_freq_sources = platform_ssp_freq_sources;

static int clock_platform_set_cpu_freq(int clock, int freq_idx)
{
	uint32_t enc = cpu_freq_enc[freq_idx];

	/* set CPU frequency request for CCU */
	io_reg_update_bits(SHIM_BASE + SHIM_CSR, SHIM_CSR_DCS_MASK,
			   enc);

	return 0;
}

static SHARED_DATA struct clock_info platform_clocks_info[] = {
	{
		.freqs_num = NUM_CPU_FREQ,
		.freqs = platform_cpu_freq,
		.default_freq_idx = CPU_DEFAULT_IDX,
		.current_freq_idx = CPU_DEFAULT_IDX,
		.notification_id = NOTIFIER_ID_CPU_FREQ,
		.notification_mask = NOTIFIER_TARGET_CORE_MASK(0),
		.set_freq = clock_platform_set_cpu_freq,
	},
	{
		.freqs_num = NUM_SSP_FREQ,
		.freqs = platform_ssp_freq,
		.default_freq_idx = SSP_DEFAULT_IDX,
		.current_freq_idx = SSP_DEFAULT_IDX,
		.notification_id = NOTIFIER_ID_SSP_FREQ,
		.notification_mask = NOTIFIER_TARGET_CORE_ALL_MASK,
		.set_freq = NULL,
	}
};

STATIC_ASSERT(ARRAY_SIZE(platform_clocks_info) == NUM_CLOCKS,
	      invalid_number_of_clocks);

void platform_clock_init(struct sof *sof)
{
	int i;

	sof->clocks = platform_clocks_info;

	for (i = 0; i < NUM_CLOCKS; i++)
		spinlock_init(&sof->clocks[i].lock);

	platform_shared_commit(sof->clocks, sizeof(*sof->clocks) * NUM_CLOCKS);
}
