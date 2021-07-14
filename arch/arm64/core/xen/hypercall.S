/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <xen/public/arch-arm.h>
#include <xen/public/xen.h>

#define HYPERCALL(hypercall)			\
GTEXT(HYPERVISOR_##hypercall);			\
SECTION_FUNC(TEXT, HYPERVISOR_##hypercall)	\
	mov x16, #__HYPERVISOR_##hypercall;	\
	hvc XEN_HYPERCALL_TAG;			\
	ret;

_ASM_FILE_PROLOGUE

HYPERCALL(console_io);
HYPERCALL(grant_table_op);
HYPERCALL(sched_op);
HYPERCALL(event_channel_op);
HYPERCALL(hvm_op);
HYPERCALL(memory_op);
