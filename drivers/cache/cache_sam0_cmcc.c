/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Cortex M Cache Controller (CMCC) of the SAM D5x/E5x: a 4 KB unified write-through cache on
 * the Cortex-M4F I-Code and D-Code buses. It holds what the core reads from the code region
 * (internal flash, NVM user and calibration pages, QSPI memory window), not SRAM, and is managed
 * as the i-cache.
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/cache.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <soc.h>

static struct k_spinlock cache_sam0_cmcc_lock;

static inline bool cache_sam0_cmcc_enabled(void)
{
	return CMCC->SR.bit.CSTS != 0U;
}

static inline void cache_sam0_cmcc_disable(void)
{
	CMCC->CTRL.reg = 0U;
	while (cache_sam0_cmcc_enabled()) {
	}
}

void cache_instr_enable(void)
{
	K_SPINLOCK(&cache_sam0_cmcc_lock) {
		/* The code region may have changed while the cache was disabled */
		cache_sam0_cmcc_disable();
		CMCC->MAINT0.reg = CMCC_MAINT0_INVALL;
		CMCC->CTRL.reg = CMCC_CTRL_CEN;
	}
}

void cache_instr_disable(void)
{
	K_SPINLOCK(&cache_sam0_cmcc_lock) {
		cache_sam0_cmcc_disable();
	}
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	K_SPINLOCK(&cache_sam0_cmcc_lock) {
		bool enabled = cache_sam0_cmcc_enabled();

		/* The data sheet requires the cache to be disabled to invalidate it */
		cache_sam0_cmcc_disable();
		CMCC->MAINT0.reg = CMCC_MAINT0_INVALL;

		if (enabled) {
			CMCC->CTRL.reg = CMCC_CTRL_CEN;
		}
	}

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}
