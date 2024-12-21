/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/coredump.h>
#include <xtensa_asm2_context.h>
#include <zephyr/offsets.h>

#define ARCH_HDR_VER			1
#define XTENSA_BLOCK_HDR_VER		2

enum xtensa_soc_code {
	XTENSA_SOC_UNKNOWN = 0,
	XTENSA_SOC_SAMPLE_CONTROLLER,
	XTENSA_SOC_ESP32,
	XTENSA_SOC_INTEL_ADSP,
	XTENSA_SOC_ESP32S2,
	XTENSA_SOC_ESP32S3,
	XTENSA_SOC_DC233C,
};

struct xtensa_arch_block {
	/* Each Xtensa SOC can omit registers (e.g. loop
	 * registers) or assign different index numbers
	 * in xtensa-config.c. GDB identifies registers
	 * based on these indices
	 *
	 * (This must be the first field or the GDB server
	 * won't be able to unpack the struct while parsing)
	 */
	uint8_t		soc;

	/* Future versions of Xtensa coredump may expand
	 * minimum set of registers
	 *
	 * (This should stay the second field for the same
	 * reason as the first once we have more versions)
	 */
	uint16_t	version;

	uint8_t		toolchain;

	struct {
		/* Minimum set shown by GDB 'info registers',
		 * skipping user-defined register EXPSTATE
		 *
		 * WARNING: IF YOU CHANGE THE ORDER OF THE REGISTERS,
		 * YOU MUST UPDATE THE ORDER OF THE REGISTERS IN
		 * EACH OF THE XtensaSoc_ RegNum enums IN
		 * scripts/coredump/gdbstubs/arch/xtensa.py TO MATCH.
		 * See xtensa.py's map_register function for details
		 */
		uint32_t	pc;
		uint32_t	exccause;
		uint32_t	excvaddr;
		uint32_t	sar;
		uint32_t	ps;
#if XCHAL_HAVE_S32C1I
		uint32_t	scompare1;
#endif
		uint32_t	a0;
		uint32_t	a1;
		uint32_t	a2;
		uint32_t	a3;
		uint32_t	a4;
		uint32_t	a5;
		uint32_t	a6;
		uint32_t	a7;
		uint32_t	a8;
		uint32_t	a9;
		uint32_t	a10;
		uint32_t	a11;
		uint32_t	a12;
		uint32_t	a13;
		uint32_t	a14;
		uint32_t	a15;
#if XCHAL_HAVE_LOOPS
		uint32_t	lbeg;
		uint32_t	lend;
		uint32_t	lcount;
#endif
	} r;
} __packed;

/*
 * This might be too large for stack space if defined
 * inside function. So do it here.
 */
static struct xtensa_arch_block arch_blk;

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

	arch_blk.version = XTENSA_BLOCK_HDR_VER;

	#if CONFIG_SOC_XTENSA_SAMPLE_CONTROLLER
		arch_blk.soc = XTENSA_SOC_SAMPLE_CONTROLLER;
	#elif CONFIG_SOC_FAMILY_INTEL_ADSP
		arch_blk.soc = XTENSA_SOC_INTEL_ADSP;
	#elif CONFIG_SOC_SERIES_ESP32
		arch_blk.soc = XTENSA_SOC_ESP32;
	#elif CONFIG_SOC_SERIES_ESP32S2
		arch_blk.soc = XTENSA_SOC_ESP32S2;
	#elif CONFIG_SOC_SERIES_ESP32S3
		arch_blk.soc = XTENSA_SOC_ESP32S3;
	#elif CONFIG_SOC_XTENSA_DC233C
		arch_blk.soc = XTENSA_SOC_DC233C;
	#else
		arch_blk.soc = XTENSA_SOC_UNKNOWN;
	#endif

	/* Set in top-level CMakeLists.txt for use with Xtensa coredump */
	arch_blk.toolchain = XTENSA_TOOLCHAIN_VARIANT;

	__asm__ volatile("rsr.exccause %0" : "=r"(arch_blk.r.exccause));

	_xtensa_irq_stack_frame_raw_t *frame = (void *)esf;
	_xtensa_irq_bsa_t *bsa = frame->ptr_to_bsa;
	uintptr_t num_high_regs;
	int regs_blk_remaining;

	/* Calculate number of high registers. */
	num_high_regs = (uint8_t *)bsa - (uint8_t *)frame + sizeof(void *);
	num_high_regs /= sizeof(uintptr_t);

	/* And high registers are always comes in 4 in a block. */
	regs_blk_remaining = (int)num_high_regs / 4;

	arch_blk.r.pc = bsa->pc;
	__asm__ volatile("rsr.excvaddr %0" : "=r"(arch_blk.r.excvaddr));
	arch_blk.r.ps = bsa->ps;
#if XCHAL_HAVE_S32C1I
	arch_blk.r.scompare1 = bsa->scompare1;
#endif
	arch_blk.r.sar = bsa->sar;
	arch_blk.r.a0 = bsa->a0;
	arch_blk.r.a1 = (uint32_t)((char *)bsa) + sizeof(*bsa);
	arch_blk.r.a2 = bsa->a2;
	arch_blk.r.a3 = bsa->a3;
	if (regs_blk_remaining > 0) {
		regs_blk_remaining--;

		arch_blk.r.a4 = frame->blks[regs_blk_remaining].r0;
		arch_blk.r.a5 = frame->blks[regs_blk_remaining].r1;
		arch_blk.r.a6 = frame->blks[regs_blk_remaining].r2;
		arch_blk.r.a7 = frame->blks[regs_blk_remaining].r3;
	}
	if (regs_blk_remaining > 0) {
		regs_blk_remaining--;

		arch_blk.r.a8 = frame->blks[regs_blk_remaining].r0;
		arch_blk.r.a9 = frame->blks[regs_blk_remaining].r1;
		arch_blk.r.a10 = frame->blks[regs_blk_remaining].r2;
		arch_blk.r.a11 = frame->blks[regs_blk_remaining].r3;
	}
	if (regs_blk_remaining > 0) {
		arch_blk.r.a12 = frame->blks[regs_blk_remaining].r0;
		arch_blk.r.a13 = frame->blks[regs_blk_remaining].r1;
		arch_blk.r.a14 = frame->blks[regs_blk_remaining].r2;
		arch_blk.r.a15 = frame->blks[regs_blk_remaining].r3;
	}
	#if XCHAL_HAVE_LOOPS
	arch_blk.r.lbeg = bsa->lbeg;
	arch_blk.r.lend = bsa->lend;
	arch_blk.r.lcount = bsa->lcount;
	#endif

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_XTENSA;
}

#if defined(CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK)
void arch_coredump_priv_stack_dump(struct k_thread *thread)
{
	struct xtensa_thread_stack_header *hdr_stack_obj;
	uintptr_t start_addr, end_addr;

	hdr_stack_obj = (struct xtensa_thread_stack_header *)thread->stack_obj;

	start_addr = (uintptr_t)&hdr_stack_obj->privilege_stack[0];
	end_addr = start_addr + sizeof(hdr_stack_obj->privilege_stack);

	coredump_memory_dump(start_addr, end_addr);
}
#endif /* CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK */
