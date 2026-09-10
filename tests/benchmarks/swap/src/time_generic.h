/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
static ALWAYS_INLINE void time_setup(void)
{
}

static ALWAYS_INLINE uint32_t time(void)
{
	return k_cycle_get_32();
}

static ALWAYS_INLINE uint32_t time_delta(uint32_t t0, uint32_t t1)
{
	return t1 - t0;
}
