/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/arch/hexagon/exception.h>
#include <zephyr/arch/hexagon/thread.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>

#define ARCH_HDR_VER 1

struct hexagon_arch_block {
	struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t r3;
		uint32_t r4;
		uint32_t r5;
		uint32_t r6;
		uint32_t r7;
		uint32_t r8;
		uint32_t r9;
		uint32_t r10;
		uint32_t r11;
		uint32_t r12;
		uint32_t r13;
		uint32_t r14;
		uint32_t r15;
		uint32_t r16;
		uint32_t r17;
		uint32_t r18;
		uint32_t r19;
		uint32_t r20;
		uint32_t r21;
		uint32_t r22;
		uint32_t r23;
		uint32_t r24;
		uint32_t r25;
		uint32_t r26;
		uint32_t r27;
		uint32_t r28;
		uint32_t r29_sp;
		uint32_t r30_fp;
		uint32_t r31_lr;
		uint32_t pc;
		uint32_t event_type;
		uint32_t g0;
		uint32_t g1;
		uint32_t g2;
		uint32_t g3;
	} r;
} __packed;

static struct hexagon_arch_block arch_blk;
static uintptr_t hexagon_fault_sp;

void z_hexagon_coredump_set_fault_sp(uintptr_t sp)
{
	hexagon_fault_sp = sp;
}

void arch_coredump_info_dump(const struct arch_esf *esf)
{
	struct coredump_arch_hdr_t hdr = {
		.id = COREDUMP_ARCH_HDR_ID,
		.hdr_version = ARCH_HDR_VER,
		.num_bytes = sizeof(arch_blk),
	};

	if (esf == NULL) {
		return;
	}

	(void)memset(&arch_blk, 0, sizeof(arch_blk));

#ifdef CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP
	/* Clear a previous dump's value before recording this fault. */
	hexagon_fault_sp = esf->r29_sp;
#endif

	arch_blk.r.r0 = esf->r0;
	arch_blk.r.r1 = esf->r1;
	arch_blk.r.r2 = esf->r2;
	arch_blk.r.r3 = esf->r3;
	arch_blk.r.r4 = esf->r4;
	arch_blk.r.r5 = esf->r5;
	arch_blk.r.r6 = esf->r6;
	arch_blk.r.r7 = esf->r7;
	arch_blk.r.r8 = esf->r8;
	arch_blk.r.r9 = esf->r9;
	arch_blk.r.r10 = esf->r10;
	arch_blk.r.r11 = esf->r11;
	arch_blk.r.r12 = esf->r12;
	arch_blk.r.r13 = esf->r13;
	arch_blk.r.r14 = esf->r14;
	arch_blk.r.r15 = esf->r15;
	arch_blk.r.r16 = esf->r16;
	arch_blk.r.r17 = esf->r17;
	arch_blk.r.r18 = esf->r18;
	arch_blk.r.r19 = esf->r19;
	arch_blk.r.r20 = esf->r20;
	arch_blk.r.r21 = esf->r21;
	arch_blk.r.r22 = esf->r22;
	arch_blk.r.r23 = esf->r23;
	arch_blk.r.r24 = esf->r24;
	arch_blk.r.r25 = esf->r25;
	arch_blk.r.r26 = esf->r26;
	arch_blk.r.r27 = esf->r27;
	arch_blk.r.r28 = esf->r28;
	arch_blk.r.r29_sp = esf->r29_sp;
	arch_blk.r.r30_fp = esf->r30_fp;
	arch_blk.r.r31_lr = esf->r31_lr;
	arch_blk.r.pc = esf->pc;
	arch_blk.r.event_type = esf->event_type;
	arch_blk.r.g0 = esf->event_info[0];
	arch_blk.r.g1 = esf->event_info[1];
	arch_blk.r.g2 = esf->event_info[2];
	arch_blk.r.g3 = esf->event_info[3];

	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_HEXAGON;
}

#ifdef CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP
uintptr_t arch_coredump_stack_ptr_get(const struct k_thread *thread)
{
	uintptr_t start;
	uintptr_t sp;
	size_t size;

	if (thread == NULL) {
		return 0U;
	}

	start = thread->stack_info.start;
	size = thread->stack_info.size;
	if (start > UINTPTR_MAX - size) {
		return start;
	}

	if (thread == _current) {
		sp = hexagon_fault_sp;
	} else {
		sp = (uintptr_t)thread->switch_handle;
	}

	/* A stack pointer at the exclusive end is a valid empty stack. */
	if (sp < start || sp - start > size) {
		return start;
	}

	return sp;
}
#endif

#ifdef CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK
void arch_coredump_priv_stack_dump(struct k_thread *thread)
{
	uintptr_t start_addr;
	uintptr_t end_addr;

	if (thread == NULL) {
		return;
	}

	start_addr = (uintptr_t)thread->arch.priv_stack;
	if (start_addr > UINTPTR_MAX - sizeof(thread->arch.priv_stack)) {
		return;
	}

	end_addr = start_addr + sizeof(thread->arch.priv_stack);
	if (end_addr == start_addr) {
		return;
	}

	coredump_memory_dump(start_addr, end_addr);
}
#endif
