/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/debug/coredump.h>

#define ARCH_HDR_VER			1

struct x86_arch_block {
	uint32_t	vector;
	uint32_t	code;

	struct {
		uint32_t	eax;
		uint32_t	ecx;
		uint32_t	edx;
		uint32_t	ebx;
		uint32_t	esp;
		uint32_t	ebp;
		uint32_t	esi;
		uint32_t	edi;
		uint32_t	eip;
		uint32_t	eflags;
		uint32_t	cs;
	} r;
} __packed;

/*
 * This might be too large for stack space if defined
 * inside function. So do it here.
 */
static struct x86_arch_block arch_blk;

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

	arch_blk.vector = z_x86_exception_vector;
	arch_blk.code = esf->errorCode;

	/*
	 * 16 registers expected by GDB.
	 * Not all are in ESF but the GDB stub
	 * will need to send all 16 as one packet.
	 * The stub will need to send undefined
	 * for registers not presented in coredump.
	 */
	arch_blk.r.eax = esf->eax;
	arch_blk.r.ebx = esf->ebx;
	arch_blk.r.ecx = esf->ecx;
	arch_blk.r.edx = esf->edx;
	arch_blk.r.esp = esf->esp;
	arch_blk.r.ebp = esf->ebp;
	arch_blk.r.esi = esf->esi;
	arch_blk.r.edi = esf->edi;
	arch_blk.r.eip = esf->eip;
	arch_blk.r.eflags = esf->eflags;
	arch_blk.r.cs = esf->cs & 0xFFFFU;

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_X86;
}
