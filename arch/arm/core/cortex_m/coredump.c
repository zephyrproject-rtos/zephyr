/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <debug/coredump.h>

#define ARCH_HDR_VER			1

uint32_t z_arm_coredump_fault_sp;

struct arm_arch_block {
	struct {
		uint32_t	r0;
		uint32_t	r1;
		uint32_t	r2;
		uint32_t	r3;
		uint32_t	r12;
		uint32_t	lr;
		uint32_t	pc;
		uint32_t	xpsr;
		uint32_t	sp;
	} r;
} __packed;

/*
 * This might be too large for stack space if defined
 * inside function. So do it here.
 */
static struct arm_arch_block arch_blk;

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

	/*
	 * 17 registers expected by GDB.
	 * Not all are in ESF but the GDB stub
	 * will need to send all 17 as one packet.
	 * The stub will need to send undefined
	 * for registers not presented in coredump.
	 */
	arch_blk.r.r0 = esf->basic.r0;
	arch_blk.r.r1 = esf->basic.r1;
	arch_blk.r.r2 = esf->basic.r2;
	arch_blk.r.r3 = esf->basic.r3;
	arch_blk.r.r12 = esf->basic.ip;
	arch_blk.r.lr = esf->basic.lr;
	arch_blk.r.pc = esf->basic.pc;
	arch_blk.r.xpsr = esf->basic.xpsr;

	arch_blk.r.sp = z_arm_coredump_fault_sp;

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_ARM_CORTEX_M;
}
