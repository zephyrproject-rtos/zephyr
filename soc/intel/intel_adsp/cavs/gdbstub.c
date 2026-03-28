/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <xtensa_asm2_context.h>

#include <offsets.h>

static struct xtensa_register gdb_reg_list[] = {
	{
		/* PC */
		.idx = 0x20,
		.regno = 0x20,
		.byte_size = 4,
		.gpkt_offset = 0,
		.stack_offset = ___xtensa_irq_bsa_t_pc_OFFSET,
	},
	{
		/* AR0 */
		.idx = 0x100,
		.regno = 0x100,
		.byte_size = 4,
		.gpkt_offset = 4,
	},
	{
		/* AR1 */
		.idx = 0x101,
		.regno = 0x101,
		.byte_size = 4,
		.gpkt_offset = 8,
	},
	{
		/* AR2 */
		.idx = 0x102,
		.regno = 0x102,
		.byte_size = 4,
		.gpkt_offset = 12,
	},
	{
		/* AR3 */
		.idx = 0x103,
		.regno = 0x103,
		.byte_size = 4,
		.gpkt_offset = 16,
	},
	{
		/* AR4 */
		.idx = 0x104,
		.regno = 0x104,
		.byte_size = 4,
		.gpkt_offset = 20,
	},
	{
		/* AR5 */
		.idx = 0x105,
		.regno = 0x105,
		.byte_size = 4,
		.gpkt_offset = 24,
	},
	{
		/* AR6 */
		.idx = 0x106,
		.regno = 0x106,
		.byte_size = 4,
		.gpkt_offset = 28,
	},
	{
		/* AR7 */
		.idx = 0x107,
		.regno = 0x107,
		.byte_size = 4,
		.gpkt_offset = 32,
	},
	{
		/* AR8 */
		.idx = 0x108,
		.regno = 0x108,
		.byte_size = 4,
		.gpkt_offset = 36,
	},
	{
		/* AR9 */
		.idx = 0x109,
		.regno = 0x109,
		.byte_size = 4,
		.gpkt_offset = 40,
	},
	{
		/* AR10 */
		.idx = 0x10a,
		.regno = 0x10a,
		.byte_size = 4,
		.gpkt_offset = 44,
	},
	{
		/* AR11 */
		.idx = 0x10b,
		.regno = 0x10b,
		.byte_size = 4,
		.gpkt_offset = 48,
	},
	{
		/* AR12 */
		.idx = 0x10c,
		.regno = 0x10c,
		.byte_size = 4,
		.gpkt_offset = 52,
	},
	{
		/* AR13 */
		.idx = 0x10d,
		.regno = 0x10d,
		.byte_size = 4,
		.gpkt_offset = 56,
	},
	{
		/* AR14 */
		.idx = 0x10e,
		.regno = 0x10e,
		.byte_size = 4,
		.gpkt_offset = 60,
	},
	{
		/* AR15 */
		.idx = 0x10f,
		.regno = 0x10f,
		.byte_size = 4,
		.gpkt_offset = 64,
	},
	{
		/* AR16 */
		.idx = 0x110,
		.regno = 0x110,
		.byte_size = 4,
		.gpkt_offset = 68,
	},
	{
		/* AR17 */
		.idx = 0x111,
		.regno = 0x111,
		.byte_size = 4,
		.gpkt_offset = 72,
	},
	{
		/* AR18 */
		.idx = 0x112,
		.regno = 0x112,
		.byte_size = 4,
		.gpkt_offset = 76,
	},
	{
		/* AR19 */
		.idx = 0x113,
		.regno = 0x113,
		.byte_size = 4,
		.gpkt_offset = 80,
	},
	{
		/* AR20 */
		.idx = 0x114,
		.regno = 0x114,
		.byte_size = 4,
		.gpkt_offset = 84,
	},
	{
		/* AR21 */
		.idx = 0x115,
		.regno = 0x115,
		.byte_size = 4,
		.gpkt_offset = 88,
	},
	{
		/* AR22 */
		.idx = 0x116,
		.regno = 0x116,
		.byte_size = 4,
		.gpkt_offset = 92,
	},
	{
		/* AR23 */
		.idx = 0x117,
		.regno = 0x117,
		.byte_size = 4,
		.gpkt_offset = 96,
	},
	{
		/* AR24 */
		.idx = 0x118,
		.regno = 0x118,
		.byte_size = 4,
		.gpkt_offset = 100,
	},
	{
		/* AR25 */
		.idx = 0x119,
		.regno = 0x119,
		.byte_size = 4,
		.gpkt_offset = 104,
	},
	{
		/* AR26 */
		.idx = 0x11a,
		.regno = 0x11a,
		.byte_size = 4,
		.gpkt_offset = 108,
	},
	{
		/* AR27 */
		.idx = 0x11b,
		.regno = 0x11b,
		.byte_size = 4,
		.gpkt_offset = 112,
	},
	{
		/* AR28 */
		.idx = 0x11c,
		.regno = 0x11c,
		.byte_size = 4,
		.gpkt_offset = 116,
	},
	{
		/* AR29 */
		.idx = 0x11d,
		.regno = 0x11d,
		.byte_size = 4,
		.gpkt_offset = 120,
	},
	{
		/* AR30 */
		.idx = 0x11e,
		.regno = 0x11e,
		.byte_size = 4,
		.gpkt_offset = 124,
	},
	{
		/* AR31 */
		.idx = 0x11f,
		.regno = 0x11f,
		.byte_size = 4,
		.gpkt_offset = 128,
	},
	{
		/* AR32 */
		.idx = 0x120,
		.regno = 0x120,
		.byte_size = 4,
		.gpkt_offset = 132,
	},
	{
		/* AR33 */
		.idx = 0x121,
		.regno = 0x121,
		.byte_size = 4,
		.gpkt_offset = 136,
	},
	{
		/* AR34 */
		.idx = 0x122,
		.regno = 0x122,
		.byte_size = 4,
		.gpkt_offset = 140,
	},
	{
		/* AR35 */
		.idx = 0x123,
		.regno = 0x123,
		.byte_size = 4,
		.gpkt_offset = 144,
	},
	{
		/* AR36 */
		.idx = 0x124,
		.regno = 0x124,
		.byte_size = 4,
		.gpkt_offset = 148,
	},
	{
		/* AR37 */
		.idx = 0x125,
		.regno = 0x125,
		.byte_size = 4,
		.gpkt_offset = 152,
	},
	{
		/* AR38 */
		.idx = 0x126,
		.regno = 0x126,
		.byte_size = 4,
		.gpkt_offset = 156,
	},
	{
		/* AR39 */
		.idx = 0x127,
		.regno = 0x127,
		.byte_size = 4,
		.gpkt_offset = 160,
	},
	{
		/* AR40 */
		.idx = 0x128,
		.regno = 0x128,
		.byte_size = 4,
		.gpkt_offset = 164,
	},
	{
		/* AR41 */
		.idx = 0x129,
		.regno = 0x129,
		.byte_size = 4,
		.gpkt_offset = 168,
	},
	{
		/* AR42 */
		.idx = 0x12a,
		.regno = 0x12a,
		.byte_size = 4,
		.gpkt_offset = 172,
	},
	{
		/* AR43 */
		.idx = 0x12b,
		.regno = 0x12b,
		.byte_size = 4,
		.gpkt_offset = 176,
	},
	{
		/* AR44 */
		.idx = 0x12c,
		.regno = 0x12c,
		.byte_size = 4,
		.gpkt_offset = 180,
	},
	{
		/* AR45 */
		.idx = 0x12d,
		.regno = 0x12d,
		.byte_size = 4,
		.gpkt_offset = 184,
	},
	{
		/* AR46 */
		.idx = 0x12e,
		.regno = 0x12e,
		.byte_size = 4,
		.gpkt_offset = 188,
	},
	{
		/* AR47 */
		.idx = 0x12f,
		.regno = 0x12f,
		.byte_size = 4,
		.gpkt_offset = 192,
	},
	{
		/* AR48 */
		.idx = 0x130,
		.regno = 0x130,
		.byte_size = 4,
		.gpkt_offset = 196,
	},
	{
		/* AR49 */
		.idx = 0x131,
		.regno = 0x131,
		.byte_size = 4,
		.gpkt_offset = 200,
	},
	{
		/* AR50 */
		.idx = 0x132,
		.regno = 0x132,
		.byte_size = 4,
		.gpkt_offset = 204,
	},
	{
		/* AR51 */
		.idx = 0x133,
		.regno = 0x133,
		.byte_size = 4,
		.gpkt_offset = 208,
	},
	{
		/* AR52 */
		.idx = 0x134,
		.regno = 0x134,
		.byte_size = 4,
		.gpkt_offset = 212,
	},
	{
		/* AR53 */
		.idx = 0x135,
		.regno = 0x135,
		.byte_size = 4,
		.gpkt_offset = 216,
	},
	{
		/* AR54 */
		.idx = 0x136,
		.regno = 0x136,
		.byte_size = 4,
		.gpkt_offset = 220,
	},
	{
		/* AR55 */
		.idx = 0x137,
		.regno = 0x137,
		.byte_size = 4,
		.gpkt_offset = 224,
	},
	{
		/* AR56 */
		.idx = 0x138,
		.regno = 0x138,
		.byte_size = 4,
		.gpkt_offset = 228,
	},
	{
		/* AR57 */
		.idx = 0x139,
		.regno = 0x139,
		.byte_size = 4,
		.gpkt_offset = 232,
	},
	{
		/* AR58 */
		.idx = 0x13a,
		.regno = 0x13a,
		.byte_size = 4,
		.gpkt_offset = 236,
	},
	{
		/* AR59 */
		.idx = 0x13b,
		.regno = 0x13b,
		.byte_size = 4,
		.gpkt_offset = 240,
	},
	{
		/* AR60 */
		.idx = 0x13c,
		.regno = 0x13c,
		.byte_size = 4,
		.gpkt_offset = 244,
	},
	{
		/* AR61 */
		.idx = 0x13d,
		.regno = 0x13d,
		.byte_size = 4,
		.gpkt_offset = 248,
	},
	{
		/* AR62 */
		.idx = 0x13e,
		.regno = 0x13e,
		.byte_size = 4,
		.gpkt_offset = 252,
	},
	{
		/* AR63 */
		.idx = 0x13f,
		.regno = 0x13f,
		.byte_size = 4,
		.gpkt_offset = 256,
	},
	{
		/* LBEG */
		.idx = 0x200,
		.regno = 0x200,
		.byte_size = 4,
		.gpkt_offset = 260,
		.stack_offset = ___xtensa_irq_bsa_t_lbeg_OFFSET,
	},
	{
		/* LEND */
		.idx = 0x201,
		.regno = 0x201,
		.byte_size = 4,
		.gpkt_offset = 264,
		.stack_offset = ___xtensa_irq_bsa_t_lend_OFFSET,
	},
	{
		/* LCOUNT */
		.idx = 0x202,
		.regno = 0x202,
		.byte_size = 4,
		.gpkt_offset = 268,
		.stack_offset = ___xtensa_irq_bsa_t_lcount_OFFSET,
	},
	{
		/* SAR */
		.idx = 0x203,
		.regno = 0x203,
		.byte_size = 4,
		.gpkt_offset = 272,
		.stack_offset = ___xtensa_irq_bsa_t_sar_OFFSET,
	},
	{
		/* BR */
		.idx = 0x204,
		.regno = 0x204,
		.byte_size = 4,
		.is_read_only = 1,
	},
	{
		/* PREFCTL */
		.idx = 0x228,
		.regno = 0x228,
		.byte_size = 4,
		.is_read_only = 1,
	},
	{
		/* WINDOWBASE */
		.idx = 0x248,
		.regno = 0x248,
		.byte_size = 4,
		.gpkt_offset = 280,
		.is_read_only = 1,
	},
	{
		/* WINDOWSTART */
		.idx = 0x249,
		.regno = 0x249,
		.byte_size = 4,
		.gpkt_offset = 284,
		.is_read_only = 1,
	},
	{
		/* PS */
		.idx = 0x2E6,
		.regno = 0x2E6,
		.byte_size = 4,
		.gpkt_offset = 296,
		.stack_offset = ___xtensa_irq_bsa_t_ps_OFFSET,
	},
	{
		/* THREADPTR */
		.idx = 0x3E7,
		.regno = 0x3E7,
		.byte_size = 4,
		.gpkt_offset = 300,
		IF_ENABLED(CONFIG_THREAD_LOCAL_STORAGE,
		  (.stack_offset = ___xtensa_irq_bsa_t_threadptr_OFFSET,))
	},
	{
		/* SCOMPARE1 */
		.idx = 0x20C,
		.regno = 0x20C,
		.byte_size = 4,
		.gpkt_offset = 308,
		.stack_offset = ___xtensa_irq_bsa_t_scompare1_OFFSET,
	},
	{
		/* EXCCAUSE */
		.idx = 0x2E8,
		.regno = 0x2E8,
		.byte_size = 4,
		.gpkt_offset = 592,
		.stack_offset = ___xtensa_irq_bsa_t_exccause_OFFSET,
	},
	{
		/* DEBUGCAUSE */
		.idx = 0x2E9,
		.regno = 0x2E9,
		.byte_size = 4,
		.gpkt_offset = 596,
	},
	{
		/* EXCVADDR */
		.idx = 0x2EE,
		.regno = 0x2EE,
		.byte_size = 4,
		.gpkt_offset = 616,
	},
	{
		/* A0 */
		.idx = 0x0,
		.regno = 0x0,
		.byte_size = 4,
		.gpkt_offset = 632,
		.stack_offset = ___xtensa_irq_bsa_t_a0_OFFSET,
	},
	{
		/* A1 */
		.idx = 0x1,
		.regno = 0x1,
		.byte_size = 4,
		.gpkt_offset = 636,
	},
	{
		/* A2 */
		.idx = 0x2,
		.regno = 0x2,
		.byte_size = 4,
		.gpkt_offset = 640,
		.stack_offset = ___xtensa_irq_bsa_t_a2_OFFSET,
	},
	{
		/* A3 */
		.idx = 0x3,
		.regno = 0x3,
		.byte_size = 4,
		.gpkt_offset = 644,
		.stack_offset = ___xtensa_irq_bsa_t_a3_OFFSET,
	},
	{
		/* A4 */
		.idx = 0x4,
		.regno = 0x4,
		.byte_size = 4,
		.gpkt_offset = 648,
		.stack_offset = -16,
	},
	{
		/* A5 */
		.idx = 0x5,
		.regno = 0x5,
		.byte_size = 4,
		.gpkt_offset = 652,
		.stack_offset = -12,
	},
	{
		/* A6 */
		.idx = 0x6,
		.regno = 0x6,
		.byte_size = 4,
		.gpkt_offset = 656,
		.stack_offset = -8,
	},
	{
		/* A7 */
		.idx = 0x7,
		.regno = 0x7,
		.byte_size = 4,
		.gpkt_offset = 660,
		.stack_offset = -4,
	},
	{
		/* A8 */
		.idx = 0x8,
		.regno = 0x8,
		.byte_size = 4,
		.gpkt_offset = 664,
		.stack_offset = -32,
	},
	{
		/* A9 */
		.idx = 0x9,
		.regno = 0x9,
		.byte_size = 4,
		.gpkt_offset = 668,
		.stack_offset = -28,
	},
	{
		/* A10 */
		.idx = 0xA,
		.regno = 0xA,
		.byte_size = 4,
		.gpkt_offset = 672,
		.stack_offset = -24,
	},
	{
		/* A11 */
		.idx = 0xB,
		.regno = 0xB,
		.byte_size = 4,
		.gpkt_offset = 676,
		.stack_offset = -20,
	},
	{
		/* A12 */
		.idx = 0xC,
		.regno = 0xC,
		.byte_size = 4,
		.gpkt_offset = 680,
		.stack_offset = -48,
	},
	{
		/* A13 */
		.idx = 0xD,
		.regno = 0xD,
		.byte_size = 4,
		.gpkt_offset = 684,
		.stack_offset = -44,
	},
	{
		/* A14 */
		.idx = 0xE,
		.regno = 0xE,
		.byte_size = 4,
		.gpkt_offset = 688,
		.stack_offset = -40,
	},
	{
		/* A15 */
		.idx = 0xF,
		.regno = 0xF,
		.byte_size = 4,
		.gpkt_offset = 692,
		.stack_offset = -36,
	},
};

struct gdb_ctx xtensa_gdb_ctx = {
	.regs = gdb_reg_list,
	.num_regs = ARRAY_SIZE(gdb_reg_list),
};
