/*
 * Copyright (c) 2023 Frontgrade Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/debug/coredump.h>

#define ARCH_HDR_VER 1

struct arch_block {
	struct {
		uint32_t out[8];
		uint32_t global[8];
		uint32_t psr;
		uint32_t pc;
		uint32_t npc;
		uint32_t wim;
		uint32_t tbr;
		uint32_t y;
	} r;
} __packed;

static struct arch_block arch_blk;

void arch_coredump_info_dump(const z_arch_esf_t *esf)
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

	for (int i = 0; i < 8; i++) {
		arch_blk.r.out[i] = esf->out[i];
		arch_blk.r.global[i] = esf->global[i];
	}
	arch_blk.r.psr = esf->psr;
	arch_blk.r.pc = esf->pc;
	arch_blk.r.npc = esf->npc;
	arch_blk.r.wim = esf->wim;
	arch_blk.r.tbr = esf->tbr;
	arch_blk.r.y = esf->y;

	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_SPARC;
}
