/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Prefetch and cache logic of the MSPM0 CPU subsystem (CPUSS). It caches instruction and
 * literal fetches from flash, is read-only and has no maintenance operation: turning it
 * off drops its contents.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <soc_cpuss.h>

#define CPUSS_CTL           (DT_REG_ADDR(DT_NODELABEL(cpuss)) + CPUSS_CTL_OFFSET)
#define SYSCTL_SHUTDNSTORE0 (DT_REG_ADDR(DT_NODELABEL(sysctl)) + 0x1400U)

/* A CTL write only takes effect after an access to the SYSCTL shutdown storage (CPU_ERR_02) */
static void cpuss_ctl_write(uint32_t ctl)
{
	sys_write32(ctl, CPUSS_CTL);
	(void)sys_read32(SYSCTL_SHUTDNSTORE0);
}

void cache_instr_enable(void)
{
	unsigned int key = irq_lock();

	cpuss_ctl_write(sys_read32(CPUSS_CTL) | CPUSS_CTL_ICACHE);
	irq_unlock(key);
}

void cache_instr_disable(void)
{
	unsigned int key = irq_lock();

	cpuss_ctl_write(sys_read32(CPUSS_CTL) & ~CPUSS_CTL_ICACHE);
	irq_unlock(key);
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	unsigned int key = irq_lock();
	uint32_t ctl = sys_read32(CPUSS_CTL);

	/* Also drops the prefetched and cached literals */
	cpuss_ctl_write(0U);
	cpuss_ctl_write(ctl);
	irq_unlock(key);

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
