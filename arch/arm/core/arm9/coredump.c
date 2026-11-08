/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Core dump support for ARMv5 cores
 */

#include <string.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>

#define ARCH_HDR_VER 1

/* r4-r11 hold the values of the faulting context */
#define ARMV5_COREDUMP_CALLEE_VALID BIT(0)

struct armv5_arch_block {
	struct {
		uint32_t r[13]; /* r0-r12 */
		uint32_t sp;
		uint32_t lr;
		uint32_t pc;
		uint32_t cpsr;
	} r;
	uint32_t flags;
	/*
	 * offsetof(struct k_thread, callee_saved), where a switched out thread
	 * keeps r4-r11, sp and lr as they were when it called
	 * z_arm_context_switch().
	 */
	uint32_t callee_saved_offset;
} __packed;

static struct armv5_arch_block arch_blk;

/* Stack pointer of the faulting context, for arch_coredump_stack_ptr_get() */
static uintptr_t fault_sp;

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

	arch_blk.r.r[0] = esf->basic.r0;
	arch_blk.r.r[1] = esf->basic.r1;
	arch_blk.r.r[2] = esf->basic.r2;
	arch_blk.r.r[3] = esf->basic.r3;
	arch_blk.r.r[12] = esf->basic.ip;
	arch_blk.r.lr = esf->basic.lr;
	arch_blk.r.pc = esf->basic.pc;
	arch_blk.r.cpsr = esf->basic.xpsr;

	/*
	 * The exception entry stores the basic frame right below the stack
	 * pointer of the faulting context. Without EXTRA_EXCEPTION_INFO, esf is
	 * that frame; with it, esf is a copy and extra_info.msp points at the
	 * frame.
	 */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	fault_sp = esf->extra_info.msp + sizeof(struct __basic_sf);

	if (esf->extra_info.callee != NULL) {
		const _callee_saved_t *callee = esf->extra_info.callee;

		arch_blk.r.r[4] = callee->v1;
		arch_blk.r.r[5] = callee->v2;
		arch_blk.r.r[6] = callee->v3;
		arch_blk.r.r[7] = callee->v4;
		arch_blk.r.r[8] = callee->v5;
		arch_blk.r.r[9] = callee->v6;
		arch_blk.r.r[10] = callee->v7;
		arch_blk.r.r[11] = callee->v8;
		arch_blk.flags |= ARMV5_COREDUMP_CALLEE_VALID;
	}
#else
	fault_sp = POINTER_TO_UINT(&esf->basic) + sizeof(struct __basic_sf);
#endif
	arch_blk.r.sp = fault_sp;
	arch_blk.callee_saved_offset = offsetof(struct k_thread, callee_saved);

	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_ARM_ARMV5;
}

uintptr_t arch_coredump_stack_ptr_get(const struct k_thread *thread)
{
	if (thread == _current) {
		return fault_sp;
	}

	return thread->callee_saved.psp;
}
