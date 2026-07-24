/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. */
/*
 * Place the trampoline stub in the TRAMPOLINE memory region at
 * VIRT_DRAM (0x80000000) so QEMU's reset stub can reach it.
 */
SECTION_PROLOGUE(trampoline,,)
{
	KEEP(*(.trampoline*))
} > TRAMPOLINE
