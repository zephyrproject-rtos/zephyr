/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>

#include "wrap_max32_icc.h"

static bool cache_instr_is_enabled(void)
{
	if (Wrap_MXC_ICC_IsEnabled(MXC_ICC)) {
		return true;
	}

	return false;
}

void cache_instr_enable(void)
{
	Wrap_MXC_ICC_Enable(MXC_ICC);
}

void cache_instr_disable(void)
{
	if (!cache_instr_is_enabled()) {
		return;
	}

	Wrap_MXC_ICC_Disable(MXC_ICC);
}

int cache_instr_flush_all(void)
{
	if (!cache_instr_is_enabled()) {
		return -EAGAIN;
	}

	Wrap_MXC_ICC_Flush(MXC_ICC);

	return 0;
}

int cache_instr_invd_all(void)
{
	if (!cache_instr_is_enabled()) {
		return -EAGAIN;
	}

	Wrap_MXC_ICC_Invalidate(MXC_ICC);

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	if (!cache_instr_is_enabled()) {
		return -EAGAIN;
	}

	Wrap_MXC_ICC_Invalidate(MXC_ICC);

	Wrap_MXC_ICC_WaitForReady(MXC_ICC);

	return 0;
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
