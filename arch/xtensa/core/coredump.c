/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/debug/coredump.h>
#include <xtensa-asm2.h>

#define ARCH_HDR_VER			1
#define XTENSA_BLOCK_HDR_VER	1

enum xtensa_soc_code {
	XTENSA_SOC_UNKNOWN = 0,
	XTENSA_SOC_SAMPLE_CONTROLLER,
	XTENSA_SOC_ESP32,
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

	/* Future versions of Xtensa coredump
	 * may expand minimum set of registers
	 *
	 * (This should stay the second field for the same
	 * reason as the first once we have more versions)
	 */
	uint16_t	version;

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

	arch_blk.version = XTENSA_BLOCK_HDR_VER;

#if CONFIG_SOC_XTENSA_SAMPLE_CONTROLLER
	arch_blk.soc = XTENSA_SOC_SAMPLE_CONTROLLER;
#elif CONFIG_SOC_ESP32
	arch_blk.soc = XTENSA_SOC_ESP32;
#else
	arch_blk.soc = XTENSA_SOC_UNKNOWN;
#endif

	__asm__ volatile("rsr.exccause %0" : "=r"(arch_blk.r.exccause));

	int *bsa = *(int **)esf;

	arch_blk.r.pc = bsa[BSA_PC_OFF/4];
	__asm__ volatile("rsr.excvaddr %0" : "=r"(arch_blk.r.excvaddr));
	arch_blk.r.ps = bsa[BSA_PS_OFF/4];
#if XCHAL_HAVE_S32C1I
	arch_blk.r.scompare1 = bsa[BSA_SCOMPARE1_OFF];
#endif
	arch_blk.r.sar = bsa[BSA_SAR_OFF/4];
	arch_blk.r.a0 = bsa[BSA_A0_OFF/4];
	arch_blk.r.a1 =  (uint32_t)((char *)bsa) + BASE_SAVE_AREA_SIZE;
	arch_blk.r.a2 = bsa[BSA_A2_OFF/4];
	arch_blk.r.a3 = bsa[BSA_A3_OFF/4];
	if (bsa - esf > 4) {
		arch_blk.r.a4 = bsa[-4];
		arch_blk.r.a5 = bsa[-3];
		arch_blk.r.a6 = bsa[-2];
		arch_blk.r.a7 = bsa[-1];
	}
	if (bsa - esf > 8) {
		arch_blk.r.a8 = bsa[-8];
		arch_blk.r.a9 = bsa[-7];
		arch_blk.r.a10 = bsa[-6];
		arch_blk.r.a11 = bsa[-5];
	}
	if (bsa - esf > 12) {
		arch_blk.r.a12 = bsa[-12];
		arch_blk.r.a13 = bsa[-11];
		arch_blk.r.a14 = bsa[-10];
		arch_blk.r.a15 = bsa[-9];
	}
	#if XCHAL_HAVE_LOOPS
	arch_blk.r.lbeg = bsa[BSA_LBEG_OFF/4];
	arch_blk.r.lend = bsa[BSA_LEND_OFF/4];
	arch_blk.r.lcount = bsa[BSA_LCOUNT_OFF/4];
	#endif

	/* Send for output */
	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_XTENSA;
}
