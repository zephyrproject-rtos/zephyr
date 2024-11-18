/*
 * Copyright (c) 2021 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/coredump.h>

#ifndef CONFIG_64BIT
#define ARCH_HDR_VER 1
#else
#define ARCH_HDR_VER 2
#endif

uintptr_t z_riscv_get_sp_before_exc(const struct arch_esf *esf);

struct riscv_arch_block {
#ifdef CONFIG_64BIT
	struct {
		uint64_t ra;
		uint64_t tp;
		uint64_t t0;
		uint64_t t1;
		uint64_t t2;
		uint64_t a0;
		uint64_t a1;
		uint64_t a2;
		uint64_t a3;
		uint64_t a4;
		uint64_t a5;
		uint64_t a6;
		uint64_t a7;
		uint64_t t3;
		uint64_t t4;
		uint64_t t5;
		uint64_t t6;
		uint64_t pc;
		uint64_t sp;
	} r;
#else /* !CONFIG_64BIT */
	struct {
		uint32_t ra;
		uint32_t tp;
		uint32_t t0;
		uint32_t t1;
		uint32_t t2;
		uint32_t a0;
		uint32_t a1;
		uint32_t a2;
		uint32_t a3;
		uint32_t a4;
		uint32_t a5;
#if !defined(CONFIG_RISCV_ISA_RV32E)
		uint32_t a6;
		uint32_t a7;
		uint32_t t3;
		uint32_t t4;
		uint32_t t5;
		uint32_t t6;
#endif /* !CONFIG_RISCV_ISA_RV32E */
		uint32_t pc;
		uint32_t sp;
	} r;
#endif /* CONFIG_64BIT */
} __packed;

/*
 * This might be too large for stack space if defined
 * inside function. So do it here.
 */
static struct riscv_arch_block arch_blk;

void arch_coredump_info_dump(const struct arch_esf *esf)
{
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
	 * 33 registers expected by GDB. Not all are in ESF but the GDB stub will need
	 * to send all 33 as one packet. The stub will need to send undefined for
	 * registers not presented in coredump.
	 */
	arch_blk.r.ra = esf->ra;
	arch_blk.r.t0 = esf->t0;
	arch_blk.r.t1 = esf->t1;
	arch_blk.r.t2 = esf->t2;
	arch_blk.r.a0 = esf->a0;
	arch_blk.r.a1 = esf->a1;
	arch_blk.r.a2 = esf->a2;
	arch_blk.r.a3 = esf->a3;
	arch_blk.r.a4 = esf->a4;
	arch_blk.r.a5 = esf->a5;
#if !defined(CONFIG_RISCV_ISA_RV32E)
	arch_blk.r.t3 = esf->t3;
	arch_blk.r.t4 = esf->t4;
	arch_blk.r.t5 = esf->t5;
	arch_blk.r.t6 = esf->t6;
	arch_blk.r.a6 = esf->a6;
	arch_blk.r.a7 = esf->a7;
#endif /* !CONFIG_RISCV_ISA_RV32E */
	arch_blk.r.pc = esf->mepc;
	arch_blk.r.sp = z_riscv_get_sp_before_exc(esf);

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_RISC_V;
}

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
