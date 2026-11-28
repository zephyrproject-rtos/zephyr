/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread pointer read for ARMv5
 *
 * The "thread and process ID" registers the AAPCS helper normally reads
 * (TPIDRURW/TPIDRURO, CP15 c13 c0 2/3) only exist from ARMv6K on, so the
 * thread pointer is fetched from the current thread instead.
 *
 * Per the run-time ABI this helper may only clobber r0.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(__aeabi_read_tp)

SECTION_FUNC(TEXT, __aeabi_read_tp)
	get_cpu	r0
	ldr	r0, [r0, #___cpu_t_current_OFFSET]
	ldr	r0, [r0, #_thread_offset_to_tls]
	bx	lr
