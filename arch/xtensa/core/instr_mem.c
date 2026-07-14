/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/instr_mem.h>
#include <zephyr/sys/word_granular_access.h>

void *arch_memset_instr(void *buf, int c, size_t n)
{
#ifndef CONFIG_ARCH_HAS_WORD_GRANULAR_ACCESS_INSTR_MEM
	return memset(buf, c, n);
#else
	return memset_word_granular_access(buf, c, n);
#endif
}

void *arch_memcpy_to_instr(void *d, const void *s, size_t n)
{
#ifndef CONFIG_ARCH_HAS_WORD_GRANULAR_ACCESS_INSTR_MEM
	return memcpy(d, s, n);
#else
	return memcpy_to_word_granular_access(d, s, n);
#endif
}

void *arch_memcpy_from_instr(void *d, const void *s, size_t n)
{
#ifndef CONFIG_ARCH_HAS_WORD_GRANULAR_ACCESS_INSTR_MEM
	return memcpy(d, s, n);
#else
	return memcpy_from_word_granular_access(d, s, n);
#endif
}
