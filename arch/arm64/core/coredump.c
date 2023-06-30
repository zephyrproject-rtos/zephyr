/*
 * Copyright (c) 2022 Huawei Technologies SASU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/debug/coredump.h>

/* Identify the version of this block (in case of architecture changes).
 * To be interpreted by the target architecture specific block parser.
 */
#define ARCH_HDR_VER			1

/* Structure to store the architecture registers passed arch_coredump_info_dump
 * As callee saved registers are not provided in z_arch_esf_t structure in Zephyr
 * we just need 22 registers.
 */
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
	} r;
} __packed;


/*
 * Register block takes up too much stack space
 * if defined within function. So define it here.
 */
static struct arm64_arch_block arch_blk;

void arch_coredump_info_dump(const z_arch_esf_t *esf)
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
	 * The thread registers are already provided by structure z_arch_esf_t
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

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_ARM64;
}
