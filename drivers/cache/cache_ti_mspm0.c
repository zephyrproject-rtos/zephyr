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

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <soc_cpuss.h>
#include <soc_sysctl.h>

static const struct device *const cpuss = DEVICE_DT_GET(DT_NODELABEL(cpuss));

static int cpuss_ctl_update(uint32_t mask, uint32_t val)
{
	const struct device *const sysctl = DEVICE_DT_GET(DT_NODELABEL(sysctl));
	uint32_t shutdnstore;
	int ret;

	ret = syscon_update_bits(cpuss, CPUSS_CTL_OFFSET, mask, val);
	if (ret != 0) {
		return ret;
	}

	/* CTL writes take effect only after a SYSCTL shutdown storage access (CPU_ERR_02) */
	return syscon_read_reg(sysctl, SYSCTL_SHUTDNSTORE0_OFFSET, &shutdnstore);
}

void cache_instr_enable(void)
{
	(void)cpuss_ctl_update(CPUSS_CTL_ICACHE, CPUSS_CTL_ICACHE);
}

void cache_instr_disable(void)
{
	(void)cpuss_ctl_update(CPUSS_CTL_ICACHE, 0U);
}

int cache_instr_flush_all(void)
{
	/* Instruction cache is read-only, so flush is a no-op */
	return 0;
}

int cache_instr_invd_all(void)
{
	unsigned int key = irq_lock();
	uint32_t ctl;
	int ret;

	ret = syscon_read_reg(cpuss, CPUSS_CTL_OFFSET, &ctl);
	if (ret != 0) {
		goto out;
	}

	/* Also drops the prefetched and cached literals */
	ret = cpuss_ctl_update(ctl, 0U);
	if (ret != 0) {
		goto out;
	}

	ret = cpuss_ctl_update(ctl, ctl);

out:
	irq_unlock(key);

	return ret;
}

int cache_instr_flush_and_invd_all(void)
{
	/* Instruction cache is read-only, so flush is a no-op before invalidate */
	return cache_instr_invd_all();
}

int cache_instr_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	/* Instruction cache is read-only, so flush is a no-op */
	return 0;
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
