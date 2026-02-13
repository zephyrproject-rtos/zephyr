/*
 * Copyright (c) 2021 Facebook, Inc. and its affiliates
 * Copyright (c) 2026 Mirai SHINJO
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/coredump.h>

#ifndef CONFIG_64BIT
#define ARCH_HDR_VER 3
#else
#define ARCH_HDR_VER 4
#endif

extern uintptr_t z_riscv_get_sp_before_exc(const struct arch_esf *esf);

/*
 * Register block layout in GDB register-number order: x0, x1, ..., x31, pc
 *
 * Keep a fixed 33-slot layout; unavailable registers remain zero
 */
struct riscv_arch_block {
#ifdef CONFIG_64BIT
	struct {
		uint64_t zero;
		uint64_t ra;
		uint64_t sp;
		uint64_t gp;
		uint64_t tp;
		uint64_t t0;
		uint64_t t1;
		uint64_t t2;
		uint64_t s0;
		uint64_t s1;
		uint64_t a0;
		uint64_t a1;
		uint64_t a2;
		uint64_t a3;
		uint64_t a4;
		uint64_t a5;
		uint64_t a6;
		uint64_t a7;
		uint64_t s2;
		uint64_t s3;
		uint64_t s4;
		uint64_t s5;
		uint64_t s6;
		uint64_t s7;
		uint64_t s8;
		uint64_t s9;
		uint64_t s10;
		uint64_t s11;
		uint64_t t3;
		uint64_t t4;
		uint64_t t5;
		uint64_t t6;
		uint64_t pc;
	} r;
#else /* !CONFIG_64BIT */
	struct {
		uint32_t zero;
		uint32_t ra;
		uint32_t sp;
		uint32_t gp;
		uint32_t tp;
		uint32_t t0;
		uint32_t t1;
		uint32_t t2;
		uint32_t s0;
		uint32_t s1;
		uint32_t a0;
		uint32_t a1;
		uint32_t a2;
		uint32_t a3;
		uint32_t a4;
		uint32_t a5;
		uint32_t a6;
		uint32_t a7;
		uint32_t s2;
		uint32_t s3;
		uint32_t s4;
		uint32_t s5;
		uint32_t s6;
		uint32_t s7;
		uint32_t s8;
		uint32_t s9;
		uint32_t s10;
		uint32_t s11;
		uint32_t t3;
		uint32_t t4;
		uint32_t t5;
		uint32_t t6;
		uint32_t pc;
	} r;
#endif /* CONFIG_64BIT */
} __packed;

/*
 * This might be too large for stack space if defined
 * inside function. So do it here.
 */
static struct riscv_arch_block arch_blk;

#if defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP)
static uintptr_t riscv_coredump_fault_sp;
#endif

void arch_coredump_info_dump(const struct arch_esf *esf)
{
	struct coredump_arch_hdr_t hdr = {
		.id = COREDUMP_ARCH_HDR_ID,
		.hdr_version = ARCH_HDR_VER,
		.num_bytes = sizeof(arch_blk),
	};
	uintptr_t gp_val = 0;
	uintptr_t tp_val = 0;

	/* Nothing to process */
	if (esf == NULL) {
		return;
	}

	(void)memset(&arch_blk, 0, sizeof(arch_blk));

	/* zero/sp/gp/tp/s0 are available regardless of RV32E/EXCEPTION_DEBUG */
	arch_blk.r.zero = 0;
	arch_blk.r.ra = esf->ra;
	arch_blk.r.sp = z_riscv_get_sp_before_exc(esf);
#if defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP)
	riscv_coredump_fault_sp = arch_blk.r.sp;
#endif

	__asm__ volatile("mv %0, gp" : "=r"(gp_val));
	arch_blk.r.gp = gp_val;

	__asm__ volatile("mv %0, tp" : "=r"(tp_val));
	arch_blk.r.tp = tp_val;

	arch_blk.r.t0 = esf->t0;
	arch_blk.r.t1 = esf->t1;
	arch_blk.r.t2 = esf->t2;
	arch_blk.r.s0 = esf->s0;
	arch_blk.r.a0 = esf->a0;
	arch_blk.r.a1 = esf->a1;
	arch_blk.r.a2 = esf->a2;
	arch_blk.r.a3 = esf->a3;
	arch_blk.r.a4 = esf->a4;
	arch_blk.r.a5 = esf->a5;

#if !defined(CONFIG_RISCV_ISA_RV32E)
	/* RV32E has only x0-x15, so x16-x31 remain zero from memset() */
	arch_blk.r.a6 = esf->a6;
	arch_blk.r.a7 = esf->a7;
	arch_blk.r.t3 = esf->t3;
	arch_blk.r.t4 = esf->t4;
	arch_blk.r.t5 = esf->t5;
	arch_blk.r.t6 = esf->t6;
#endif /* !CONFIG_RISCV_ISA_RV32E */

	/* s1-s11 are available only when EXCEPTION_DEBUG saves esf->csf */
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf->csf != NULL) {
		arch_blk.r.s1 = esf->csf->s1;
#if !defined(CONFIG_RISCV_ISA_RV32E)
		/* RV32E has no s2-s11, so those slots remain zero */
		arch_blk.r.s2 = esf->csf->s2;
		arch_blk.r.s3 = esf->csf->s3;
		arch_blk.r.s4 = esf->csf->s4;
		arch_blk.r.s5 = esf->csf->s5;
		arch_blk.r.s6 = esf->csf->s6;
		arch_blk.r.s7 = esf->csf->s7;
		arch_blk.r.s8 = esf->csf->s8;
		arch_blk.r.s9 = esf->csf->s9;
		arch_blk.r.s10 = esf->csf->s10;
		arch_blk.r.s11 = esf->csf->s11;
#endif /* !CONFIG_RISCV_ISA_RV32E */
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	arch_blk.r.pc = esf->mepc;

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_RISC_V;
}

#if defined(CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_THREADS) ||                                          \
	defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP)
uintptr_t arch_coredump_stack_ptr_get(const struct k_thread *thread)
{
#if defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP)
	if (thread == _current) {
		return riscv_coredump_fault_sp;
	}
#endif

	return thread->callee_saved.sp;
}
#endif /* CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_THREADS || CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP */

#if defined(CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK)
void arch_coredump_priv_stack_dump(struct k_thread *thread)
{
	uintptr_t start_addr, end_addr;

	/* See: zephyr/include/zephyr/arch/riscv/arch.h */
	if (IS_ENABLED(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)) {
		start_addr = thread->arch.priv_stack_start + Z_RISCV_STACK_GUARD_SIZE;
	} else {
		start_addr = thread->stack_info.start - CONFIG_PRIVILEGED_STACK_SIZE;
	}
	end_addr = Z_STACK_PTR_ALIGN(thread->arch.priv_stack_start + K_KERNEL_STACK_RESERVED +
				     CONFIG_PRIVILEGED_STACK_SIZE);

	coredump_memory_dump(start_addr, end_addr);
}
#endif /* CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK */
