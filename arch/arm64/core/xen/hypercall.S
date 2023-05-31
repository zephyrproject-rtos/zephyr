/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2021-2023 EPAM Systems
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/xen/public/arch-arm.h>
#include <zephyr/xen/public/xen.h>

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
HYPERCALL(xen_version);

#ifdef CONFIG_XEN_DOM0
HYPERCALL(domctl);
#endif
