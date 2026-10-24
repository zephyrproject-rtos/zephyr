/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Cortex-M Cache Controller (CMCC) of the Atmel SAM4S and SAM4E: a unified write-through
 * cache on the I-Code and D-Code buses, between the core and the internal flash. It holds
 * instructions and data read from the flash alike, not SRAM, and is managed as the i-cache.
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/cache.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <soc.h>

static struct k_spinlock cache_sam_cmcc_lock;

static inline bool cache_sam_cmcc_enabled(void)
{
	return (CMCC->CMCC_SR & CMCC_SR_CSTS) != 0U;
}

static inline void cache_sam_cmcc_disable(void)
{
	CMCC->CMCC_CTRL = 0U;
	while (cache_sam_cmcc_enabled()) {
	}
}

void cache_instr_enable(void)
{
	K_SPINLOCK(&cache_sam_cmcc_lock) {
		/* The flash may have changed while it was disabled, or under a bootloader */
		cache_sam_cmcc_disable();
		CMCC->CMCC_MAINT0 = CMCC_MAINT0_INVALL;
		CMCC->CMCC_CTRL = CMCC_CTRL_CEN;
	}
}

void cache_instr_disable(void)
{
	K_SPINLOCK(&cache_sam_cmcc_lock) {
		cache_sam_cmcc_disable();
	}
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	K_SPINLOCK(&cache_sam_cmcc_lock) {
		bool enabled = cache_sam_cmcc_enabled();

		/* Disabled to invalidate, as Microchip documents for the CMCC */
		cache_sam_cmcc_disable();
		CMCC->CMCC_MAINT0 = CMCC_MAINT0_INVALL;

		if (enabled) {
			CMCC->CMCC_CTRL = CMCC_CTRL_CEN;
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
