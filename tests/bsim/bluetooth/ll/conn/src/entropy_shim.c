/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test-only entropy shim for reproducing zephyrproject-rtos/zephyr#126663.
 *
 * Replaces the nRF RNG entropy device of the simulated board (see
 * entropy_shim.overlay). get_entropy() always fills the buffer from a
 * software PRNG. get_entropy_isr() models the documented behaviour of
 * entropy_get_entropy_isr() when ENTROPY_BUSYWAIT is not set:
 *
 *  - short read mode: only half of the requested bytes are available,
 *  - error mode: -EBUSY, as a driver can report when shared hardware is
 *    held by the interrupted thread.
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>

#include "bsim_args_runner.h"

static uint32_t shim_prng_state = 0x2b1a5cf3U;

static void shim_fill(uint8_t *buffer, uint16_t length)
{
	while (length-- > 0U) {
		shim_prng_state ^= shim_prng_state << 13;
		shim_prng_state ^= shim_prng_state >> 17;
		shim_prng_state ^= shim_prng_state << 5;
		*buffer++ = (uint8_t)(shim_prng_state >> 24);
	}
}

static int entropy_shim_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	shim_fill(buffer, length);

	return 0;
}

static int entropy_shim_get_entropy_isr(const struct device *dev, uint8_t *buffer, uint16_t length,
					uint32_t flags)
{
	ARG_UNUSED(dev);

	if ((flags & ENTROPY_BUSYWAIT) != 0U) {
		shim_fill(buffer, length);

		return length;
	}

#if defined(CONFIG_BT_CTLR_TEST_ENTROPY_SHIM_ISR_SHORT_READ)
	static uint16_t retry_pending;

	if (length == retry_pending) {
		/* Continuation of a buffer that was short-read on the first
		 * call: the pool has been refilled, serve it fully.
		 */
		retry_pending = 0U;
		shim_fill(buffer, length);

		return length;
	}

	/* First call for a new buffer models an entropy pool with only half
	 * of the requested bytes available.
	 */
	const uint16_t available = length / 2U;

	retry_pending = length - available;
	shim_fill(buffer, available);
	printk("entropy_shim: short read %u of %u bytes\n", available, length);

	return available;
#elif defined(CONFIG_BT_CTLR_TEST_ENTROPY_SHIM_ISR_ERROR)
	printk("entropy_shim: returning -EBUSY\n");

	return -EBUSY;
#else
	shim_fill(buffer, length);

	return length;
#endif
}

static int entropy_shim_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Seed per simulated device so that both ends of a link do not draw
	 * identical streams.
	 */
	shim_prng_state ^= k_cycle_get_32();
	shim_prng_state += bsim_args_get_global_device_nbr() * 0x9e3779b9U;

	return 0;
}

static DEVICE_API(entropy, entropy_shim_api) = {
	.get_entropy = entropy_shim_get_entropy,
	.get_entropy_isr = entropy_shim_get_entropy_isr,
};

DEVICE_DT_DEFINE(DT_NODELABEL(rng), entropy_shim_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_ENTROPY_INIT_PRIORITY, &entropy_shim_api);
