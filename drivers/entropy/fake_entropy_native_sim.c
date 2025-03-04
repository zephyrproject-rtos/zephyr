/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Pseudo-random entropy generator for native simulator based target boards:
 * Following the principle of reproducibility of the native_sim board
 * this entropy device will always generate the same random sequence when
 * initialized with the same seed
 *
 * This entropy source should only be used for testing.
 */

#include <zephyr/devicetree.h>
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_native_posix_rng)
#define DT_DRV_COMPAT zephyr_native_posix_rng
#warning "zephyr,native-posix-rng is deprecated in favor of zephyr,native-sim-rng"
#else
#define DT_DRV_COMPAT zephyr_native_sim_rng
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/arch/posix/posix_trace.h>
#include "soc.h"
#include "cmdline.h" /* native_sim command line options header */
#include "nsi_host_trampolines.h"
#include "fake_entropy_native_bottom.h"

static unsigned int seed = 0x5678;
static bool seed_random;
static bool seed_set;

static int entropy_native_sim_get_entropy(const struct device *dev,
					    uint8_t *buffer,
					    uint16_t length)
{
	ARG_UNUSED(dev);

	while (length) {
		/*
		 * Note that only 1 thread (Zephyr thread or HW models), runs at
		 * a time, therefore there is no need to use random_r()
		 */
		long value = nsi_host_random();

		/* The host random() provides a number between 0 and 2**31-1. Bit 32 is always 0.
		 * So let's just use the lower 3 bytes discarding the upper 7 bits
		 */
		size_t to_copy = MIN(length, 3);

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return 0;
}

static int entropy_native_sim_get_entropy_isr(const struct device *dev,
						uint8_t *buf,
						uint16_t len, uint32_t flags)
{
	ARG_UNUSED(flags);

	/*
	 * entropy_native_sim_get_entropy() is also safe for ISRs
	 * and always produces data.
	 */
	entropy_native_sim_get_entropy(dev, buf, len);

	return len;
}

static int entropy_native_sim_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	if (seed_set || seed_random ||
	    IS_ENABLED(CONFIG_FAKE_ENTROPY_NATIVE_SIM_SEED_BY_DEFAULT)) {
		entropy_native_seed(seed, seed_random);
	}
	posix_print_warning("WARNING: "
			    "Using a test - not safe - entropy source\n");
	return 0;
}

static DEVICE_API(entropy, entropy_native_sim_api_funcs) = {
	.get_entropy     = entropy_native_sim_get_entropy,
	.get_entropy_isr = entropy_native_sim_get_entropy_isr
};

DEVICE_DT_INST_DEFINE(0,
		    entropy_native_sim_init, NULL,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_native_sim_api_funcs);

static void seed_was_set(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	seed_set = true;
}

static void add_fake_entropy_option(void)
{
	static struct args_struct_t entropy_options[] = {
		{
			.option = "seed",
			.name = "r_seed",
			.type = 'u',
			.dest = (void *)&seed,
			.call_when_found = seed_was_set,
			.descript = "A 32-bit integer seed value for the entropy device, such as "
				    "97229 (decimal), 0x17BCD (hex), or 0275715 (octal)"
		},
		{
			.is_switch = true,
			.option = "seed-random",
			.type = 'b',
			.dest = (void *)&seed_random,
			.descript = "Seed the random generator from /dev/urandom. "
				    "Note your test may not be reproducible if you set this option"
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(entropy_options);
}

NATIVE_TASK(add_fake_entropy_option, PRE_BOOT_1, 10);
