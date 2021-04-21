/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <debug/coredump.h>

#define ARCH_HDR_VER			1

struct x86_64_arch_block {
	uint64_t	vector;
	uint64_t	code;

	struct {
		uint64_t	rax;
		uint64_t	rcx;
		uint64_t	rdx;
		uint64_t	rsi;
		uint64_t	rdi;
		uint64_t	rsp;
		uint64_t	r8;
		uint64_t	r9;
		uint64_t	r10;
		uint64_t	r11;
		uint64_t	rip;
		uint64_t	eflags;
		uint64_t	cs;
		uint64_t	ss;
		uint64_t	rbp;

#ifdef CONFIG_EXCEPTION_DEBUG
		uint64_t	rbx;
		uint64_t	r12;
		uint64_t	r13;
		uint64_t	r14;
		uint64_t	r15;
#endif
	} r;
} __packed;

/*
 * Register block takes up too much stack space
 * if defined within function. So define it here.
 */
static struct x86_64_arch_block arch_blk;

void arch_coredump_info_dump(const z_arch_esf_t *esf)
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

	arch_blk.vector = esf->vector;
	arch_blk.code = esf->code;

	/*
	 * 34 registers expected by GDB.
	 * Not all are in ESF but the GDB stub
	 * will need to send all 34 as one packet.
	 * The stub will need to send undefined
	 * for registers not presented in coredump.
	 */
	arch_blk.r.rax = esf->rax;
	arch_blk.r.rcx = esf->rcx;
	arch_blk.r.rdx = esf->rdx;
	arch_blk.r.rsi = esf->rsi;
	arch_blk.r.rdi = esf->rdi;
	arch_blk.r.rsp = esf->rsp;
	arch_blk.r.rip = esf->rip;
	arch_blk.r.r8  = esf->r8;
	arch_blk.r.r9  = esf->r9;
	arch_blk.r.r10 = esf->r10;
	arch_blk.r.r11 = esf->r11;

	arch_blk.r.eflags = esf->rflags;
	arch_blk.r.cs = esf->cs & 0xFFFFU;
	arch_blk.r.ss = esf->ss;

	arch_blk.r.rbp = esf->rbp;

#ifdef CONFIG_EXCEPTION_DEBUG
	arch_blk.r.rbx = esf->rbx;
	arch_blk.r.r12 = esf->r12;
	arch_blk.r.r13 = esf->r13;
	arch_blk.r.r14 = esf->r14;
	arch_blk.r.r15 = esf->r15;
#endif

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_X86_64;
}
