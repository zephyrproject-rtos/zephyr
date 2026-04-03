/*
 * Copyright (c) 2022 Huawei Technologies SASU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/debug/coredump.h>

/*
 * v1: 22 registers (x0-x18, lr, spsr, elr)
 * v2: 24 registers (v1 + fp, sp) - needed for GDB stack unwinding
 */
#define ARCH_HDR_VER			2

struct arm64_arch_block {
	struct {
		uint64_t x0;
		uint64_t x1;
		uint64_t x2;
		uint64_t x3;
		uint64_t x4;
		uint64_t x5;
		uint64_t x6;
		uint64_t x7;
		uint64_t x8;
		uint64_t x9;
		uint64_t x10;
		uint64_t x11;
		uint64_t x12;
		uint64_t x13;
		uint64_t x14;
		uint64_t x15;
		uint64_t x16;
		uint64_t x17;
		uint64_t x18;
		uint64_t lr;
		uint64_t spsr;
		uint64_t elr;
		uint64_t fp;
		uint64_t sp;
	} r;
} __packed;


/*
 * Register block takes up too much stack space
 * if defined within function. So define it here.
 */
static struct arm64_arch_block arch_blk;

void arch_coredump_info_dump(const struct arch_esf *esf)
{
	/* Target architecture information header */
	/* Information just relevant to the python parser */
	struct coredump_arch_hdr_t hdr = {
		.id = COREDUMP_ARCH_HDR_ID,
		.hdr_version = ARCH_HDR_VER,
		.num_bytes = sizeof(arch_blk),
	};

	/* Nothing to process */
	if (esf == NULL) {
		return;
	}

	(void)memset(&arch_blk, 0, sizeof(arch_blk));

	/*
	 * Copies the thread registers to a memory block that will be printed out
	 * The thread registers are already provided by structure struct arch_esf
	 */
	arch_blk.r.x0 = esf->x0;
	arch_blk.r.x1 = esf->x1;
	arch_blk.r.x2 = esf->x2;
	arch_blk.r.x3 = esf->x3;
	arch_blk.r.x4 = esf->x4;
	arch_blk.r.x5 = esf->x5;
	arch_blk.r.x6 = esf->x6;
	arch_blk.r.x7 = esf->x7;
	arch_blk.r.x8 = esf->x8;
	arch_blk.r.x9 = esf->x9;
	arch_blk.r.x10 = esf->x10;
	arch_blk.r.x11 = esf->x11;
	arch_blk.r.x12 = esf->x12;
	arch_blk.r.x13 = esf->x13;
	arch_blk.r.x14 = esf->x14;
	arch_blk.r.x15 = esf->x15;
	arch_blk.r.x16 = esf->x16;
	arch_blk.r.x17 = esf->x17;
	arch_blk.r.x18 = esf->x18;
	arch_blk.r.lr = esf->lr;
	arch_blk.r.spsr = esf->spsr;
	arch_blk.r.elr = esf->elr;

#ifdef CONFIG_FRAME_POINTER
	arch_blk.r.fp = esf->fp;
#else
	arch_blk.r.fp = 0;
#endif
	/*
	 * The exception entry macro does: sub sp, sp, ___esf_t_SIZEOF
	 * So the pre-fault SP is right above the saved ESF.
	 * Note: for EL0 exceptions this is SP_EL1, not the user SP_EL0.
	 */
	arch_blk.r.sp = (uintptr_t)esf + sizeof(struct arch_esf);
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	/*
	 * When SAFE_EXCEPTION_STACK is enabled, for EL0 exceptions the vector
	 * entry saves the original sp_el0 into esf->sp.
	 */
	if ((esf->spsr & SPSR_MODE_MASK) == SPSR_MODE_EL0T) {
		arch_blk.r.sp = esf->sp;
	}
#endif

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_ARM64;
}
