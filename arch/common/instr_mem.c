/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/instr_mem.h>

__weak bool arch_is_instr_mem(const void *addr, size_t len)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(len);

#ifndef CONFIG_HARVARD
	return true;
#else
	return false;
#endif
}

__weak void *arch_memset_instr(void *buf, int c, size_t n)
{
	return memset(buf, c, n);
}

__weak void *arch_memcpy_to_instr(void *d, const void *s, size_t n)
{
	return memcpy(d, s, n);
}

__weak void *arch_memcpy_from_instr(void *d, const void *s, size_t n)
{
	return memcpy(d, s, n);
}
