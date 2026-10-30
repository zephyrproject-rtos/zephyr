/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * lib_coex.a trace format strings: only their addresses are emitted, but the
 * sections are ALLOC, so place them in rodata rather than as orphans.
 */
. = ALIGN(4);
__ctrace_start__ = .;
KEEP(*(.COEXTRACE))
__ctrace_end__ = .;
