/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <xtensa-asm2-context.h>

static struct xtensa_register gdb_reg_list[] = {
/* XT-GDB has different register definitions than Zephyr toolchain GDB */
#ifdef CONFIG_ADSP_XT_GDB
	{
		/* PC */
		.idx = 0x20,
		.regno = 0x20,
		.byte_size = 4,
		.gpkt_offset = 0,
		.stack_offset = BSA_PC_OFF,
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
		.stack_offset = BSA_LBEG_OFF,
	},
	{
		/* LEND */
		.idx = 0x201,
		.regno = 0x201,
		.byte_size = 4,
		.gpkt_offset = 264,
		.stack_offset = BSA_LEND_OFF,
	},
	{
		/* LCOUNT */
		.idx = 0x202,
		.regno = 0x202,
		.byte_size = 4,
		.gpkt_offset = 268,
		.stack_offset = BSA_LCOUNT_OFF,
	},
	{
		/* SAR */
		.idx = 0x203,
		.regno = 0x203,
		.byte_size = 4,
		.gpkt_offset = 272,
		.stack_offset = BSA_SAR_OFF,
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
		.stack_offset = BSA_PS_OFF,
	},
	{
		/* THREADPTR */
		.idx = 0x3E7,
		.regno = 0x3E7,
		.byte_size = 4,
		.gpkt_offset = 300,
		IF_ENABLED(CONFIG_THREAD_LOCAL_STORAGE,
		  (.stack_offset = BSA_THREADPTR_OFF,))
	},
	{
		/* SCOMPARE1 */
		.idx = 0x20C,
		.regno = 0x20C,
		.byte_size = 4,
		.gpkt_offset = 308,
		.stack_offset = BSA_SCOMPARE1_OFF,
	},
	{
		/* EXCCAUSE */
		.idx = 0x2E8,
		.regno = 0x2E8,
		.byte_size = 4,
		.gpkt_offset = 592,
		.stack_offset = BSA_EXCCAUSE_OFF,
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
		.stack_offset = BSA_A0_OFF,
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
		.stack_offset = BSA_A2_OFF,
	},
	{
		/* A3 */
		.idx = 0x3,
		.regno = 0x3,
		.byte_size = 4,
		.gpkt_offset = 644,
		.stack_offset = BSA_A3_OFF,
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
#else /* !defined(CONFIG_ADSP_XT_GDB) */
	{
		/* PC */
		.idx = 0x0,
		.regno = 0x0020,
		.byte_size = 4,
		.gpkt_offset = 0,
		.stack_offset = BSA_PC_OFF,
	},
	{
		/* AR0 */
		.idx = 0x1,
		.regno = 0x100,
		.byte_size = 4,
		.gpkt_offset = 4,
	},
	{
		/* AR1 */
		.idx = 0x2,
		.regno = 0x101,
		.byte_size = 4,
		.gpkt_offset = 8,
	},
	{
		/* AR2 */
		.idx = 0x3,
		.regno = 0x102,
		.byte_size = 4,
		.gpkt_offset = 12,
	},
	{
		/* AR3 */
		.idx = 0x4,
		.regno = 0x103,
		.byte_size = 4,
		.gpkt_offset = 16,
	},
	{
		/* AR4 */
		.idx = 0x5,
		.regno = 0x104,
		.byte_size = 4,
		.gpkt_offset = 20,
	},
	{
		/* AR5 */
		.idx = 0x6,
		.regno = 0x105,
		.byte_size = 4,
		.gpkt_offset = 24,
	},
	{
		/* AR6 */
		.idx = 0x7,
		.regno = 0x106,
		.byte_size = 4,
		.gpkt_offset = 28,
	},
	{
		/* AR7 */
		.idx = 0x8,
		.regno = 0x107,
		.byte_size = 4,
		.gpkt_offset = 32,
	},
	{
		/* AR8 */
		.idx = 0x9,
		.regno = 0x108,
		.byte_size = 4,
		.gpkt_offset = 36,
	},
	{
		/* AR9 */
		.idx = 0xa,
		.regno = 0x109,
		.byte_size = 4,
		.gpkt_offset = 40,
	},
	{
		/* AR10 */
		.idx = 0xb,
		.regno = 0x10a,
		.byte_size = 4,
		.gpkt_offset = 44,
	},
	{
		/* AR11 */
		.idx = 0xc,
		.regno = 0x10b,
		.byte_size = 4,
		.gpkt_offset = 48,
	},
	{
		/* AR12 */
		.idx = 0xd,
		.regno = 0x10c,
		.byte_size = 4,
		.gpkt_offset = 52,
	},
	{
		/* AR13 */
		.idx = 0xe,
		.regno = 0x10d,
		.byte_size = 4,
		.gpkt_offset = 56,
	},
	{
		/* AR14 */
		.idx = 0xf,
		.regno = 0x10e,
		.byte_size = 4,
		.gpkt_offset = 60,
	},
	{
		/* AR15 */
		.idx = 0x10,
		.regno = 0x10f,
		.byte_size = 4,
		.gpkt_offset = 64,
	},
	{
		/* AR16 */
		.idx = 0x11,
		.regno = 0x110,
		.byte_size = 4,
		.gpkt_offset = 68,
	},
	{
		/* AR17 */
		.idx = 0x12,
		.regno = 0x111,
		.byte_size = 4,
		.gpkt_offset = 72,
	},
	{
		/* AR18 */
		.idx = 0x13,
		.regno = 0x112,
		.byte_size = 4,
		.gpkt_offset = 76,
	},
	{
		/* AR19 */
		.idx = 0x14,
		.regno = 0x113,
		.byte_size = 4,
		.gpkt_offset = 80,
	},
	{
		/* AR20 */
		.idx = 0x15,
		.regno = 0x114,
		.byte_size = 4,
		.gpkt_offset = 84,
	},
	{
		/* AR21 */
		.idx = 0x16,
		.regno = 0x115,
		.byte_size = 4,
		.gpkt_offset = 88,
	},
	{
		/* AR22 */
		.idx = 0x17,
		.regno = 0x116,
		.byte_size = 4,
		.gpkt_offset = 92,
	},
	{
		/* AR23 */
		.idx = 0x18,
		.regno = 0x117,
		.byte_size = 4,
		.gpkt_offset = 96,
	},
	{
		/* AR24 */
		.idx = 0x19,
		.regno = 0x118,
		.byte_size = 4,
		.gpkt_offset = 100,
	},
	{
		/* AR25 */
		.idx = 0x1a,
		.regno = 0x119,
		.byte_size = 4,
		.gpkt_offset = 104,
	},
	{
		/* AR26 */
		.idx = 0x1b,
		.regno = 0x11a,
		.byte_size = 4,
		.gpkt_offset = 108,
	},
	{
		/* AR27 */
		.idx = 0x1c,
		.regno = 0x11b,
		.byte_size = 4,
		.gpkt_offset = 112,
	},
	{
		/* AR28 */
		.idx = 0x1d,
		.regno = 0x11c,
		.byte_size = 4,
		.gpkt_offset = 116,
	},
	{
		/* AR29 */
		.idx = 0x1e,
		.regno = 0x11d,
		.byte_size = 4,
		.gpkt_offset = 120,
	},
	{
		/* AR30 */
		.idx = 0x1f,
		.regno = 0x11e,
		.byte_size = 4,
		.gpkt_offset = 124,
	},
	{
		/* AR31 */
		.idx = 0x20,
		.regno = 0x11f,
		.byte_size = 4,
		.gpkt_offset = 128,
	},
	{
		/* AR32 */
		.idx = 0x21,
		.regno = 0x120,
		.byte_size = 4,
		.gpkt_offset = 132,
	},
	{
		/* AR33 */
		.idx = 0x22,
		.regno = 0x121,
		.byte_size = 4,
		.gpkt_offset = 136,
	},
	{
		/* AR34 */
		.idx = 0x23,
		.regno = 0x122,
		.byte_size = 4,
		.gpkt_offset = 140,
	},
	{
		/* AR35 */
		.idx = 0x24,
		.regno = 0x123,
		.byte_size = 4,
		.gpkt_offset = 144,
	},
	{
		/* AR36 */
		.idx = 0x25,
		.regno = 0x124,
		.byte_size = 4,
		.gpkt_offset = 148,
	},
	{
		/* AR37 */
		.idx = 0x26,
		.regno = 0x125,
		.byte_size = 4,
		.gpkt_offset = 152,
	},
	{
		/* AR38 */
		.idx = 0x27,
		.regno = 0x126,
		.byte_size = 4,
		.gpkt_offset = 156,
	},
	{
		/* AR39 */
		.idx = 0x28,
		.regno = 0x127,
		.byte_size = 4,
		.gpkt_offset = 160,
	},
	{
		/* AR40 */
		.idx = 0x29,
		.regno = 0x128,
		.byte_size = 4,
		.gpkt_offset = 164,
	},
	{
		/* AR41 */
		.idx = 0x2a,
		.regno = 0x129,
		.byte_size = 4,
		.gpkt_offset = 168,
	},
	{
		/* AR42 */
		.idx = 0x2b,
		.regno = 0x12a,
		.byte_size = 4,
		.gpkt_offset = 172,
	},
	{
		/* AR43 */
		.idx = 0x2c,
		.regno = 0x12b,
		.byte_size = 4,
		.gpkt_offset = 176,
	},
	{
		/* AR44 */
		.idx = 0x2d,
		.regno = 0x12c,
		.byte_size = 4,
		.gpkt_offset = 180,
	},
	{
		/* AR45 */
		.idx = 0x2e,
		.regno = 0x12d,
		.byte_size = 4,
		.gpkt_offset = 184,
	},
	{
		/* AR46 */
		.idx = 0x2f,
		.regno = 0x12e,
		.byte_size = 4,
		.gpkt_offset = 188,
	},
	{
		/* AR47 */
		.idx = 0x30,
		.regno = 0x12f,
		.byte_size = 4,
		.gpkt_offset = 192,
	},
	{
		/* AR48 */
		.idx = 0x31,
		.regno = 0x130,
		.byte_size = 4,
		.gpkt_offset = 196,
	},
	{
		/* AR49 */
		.idx = 0x32,
		.regno = 0x131,
		.byte_size = 4,
		.gpkt_offset = 200,
	},
	{
		/* AR50 */
		.idx = 0x33,
		.regno = 0x132,
		.byte_size = 4,
		.gpkt_offset = 204,
	},
	{
		/* AR51 */
		.idx = 0x34,
		.regno = 0x133,
		.byte_size = 4,
		.gpkt_offset = 208,
	},
	{
		/* AR52 */
		.idx = 0x35,
		.regno = 0x134,
		.byte_size = 4,
		.gpkt_offset = 212,
	},
	{
		/* AR53 */
		.idx = 0x36,
		.regno = 0x135,
		.byte_size = 4,
		.gpkt_offset = 216,
	},
	{
		/* AR54 */
		.idx = 0x37,
		.regno = 0x136,
		.byte_size = 4,
		.gpkt_offset = 220,
	},
	{
		/* AR55 */
		.idx = 0x38,
		.regno = 0x137,
		.byte_size = 4,
		.gpkt_offset = 224,
	},
	{
		/* AR56 */
		.idx = 0x39,
		.regno = 0x138,
		.byte_size = 4,
		.gpkt_offset = 228,
	},
	{
		/* AR57 */
		.idx = 0x3a,
		.regno = 0x139,
		.byte_size = 4,
		.gpkt_offset = 232,
	},
	{
		/* AR58 */
		.idx = 0x3b,
		.regno = 0x13a,
		.byte_size = 4,
		.gpkt_offset = 236,
	},
	{
		/* AR59 */
		.idx = 0x3c,
		.regno = 0x13b,
		.byte_size = 4,
		.gpkt_offset = 240,
	},
	{
		/* AR60 */
		.idx = 0x3d,
		.regno = 0x13c,
		.byte_size = 4,
		.gpkt_offset = 244,
	},
	{
		/* AR61 */
		.idx = 0x3e,
		.regno = 0x13d,
		.byte_size = 4,
		.gpkt_offset = 248,
	},
	{
		/* AR62 */
		.idx = 0x3f,
		.regno = 0x13e,
		.byte_size = 4,
		.gpkt_offset = 252,
	},
	{
		/* AR63 */
		.idx = 0x40,
		.regno = 0x13f,
		.byte_size = 4,
		.gpkt_offset = 256,
	},
	{
		/* LBEG */
		.idx = 0x41,
		.regno = 0x0200,
		.byte_size = 4,
		.gpkt_offset = 260,
		.stack_offset = BSA_LBEG_OFF,
	},
	{
		/* LEND */
		.idx = 0x42,
		.regno = 0x0201,
		.byte_size = 4,
		.gpkt_offset = 264,
		.stack_offset = BSA_LEND_OFF,
	},
	{
		/* LCOUNT */
		.idx = 0x43,
		.regno = 0x0202,
		.byte_size = 4,
		.gpkt_offset = 268,
		.stack_offset = BSA_LCOUNT_OFF,
	},
	{
		/* SAR */
		.idx = 0x44,
		.regno = 0x0203,
		.byte_size = 4,
		.gpkt_offset = 272,
		.stack_offset = BSA_SAR_OFF,
	},
	{
		/* WINDOWBASE */
		.idx = 0x46,
		.regno = 0x0248,
		.byte_size = 4,
		.gpkt_offset = 280,
		.is_read_only = 1,
	},
	{
		/* WINDOWSTART */
		.idx = 0x47,
		.regno = 0x0249,
		.byte_size = 4,
		.gpkt_offset = 284,
		.is_read_only = 1,
	},
	{
		/* PS */
		.idx = 0x4a,
		.regno = 0x02E6,
		.byte_size = 4,
		.gpkt_offset = 296,
		.stack_offset = BSA_PS_OFF,
	},
	{
		/* THREADPTR */
		.idx = 0x4b,
		.regno = 0x03E7,
		.byte_size = 4,
		.gpkt_offset = 300,
		IF_ENABLED(CONFIG_THREAD_LOCAL_STORAGE,
		  (.stack_offset = BSA_THREADPTR_OFF,))
	},
	{
		/* SCOMPARE1 */
		.idx = 0x4d,
		.regno = 0x020C,
		.byte_size = 4,
		.gpkt_offset = 308,
		.stack_offset = BSA_SCOMPARE1_OFF,
	},
	{
		/* EXCCAUSE */
		.idx = 0x94,
		.regno = 0x02E8,
		.byte_size = 4,
		.gpkt_offset = 592,
		.stack_offset = BSA_EXCCAUSE_OFF,
	},
	{
		/* DEBUGCAUSE */
		.idx = 0x95,
		.regno = 0x02E9,
		.byte_size = 4,
		.gpkt_offset = 596,
	},
	{
		/* EXCVADDR */
		.idx = 0x9a,
		.regno = 0x02EE,
		.byte_size = 4,
		.gpkt_offset = 616,
	},
	{
		/* A0 */
		.idx = 0x9e,
		.regno = 0x0000,
		.byte_size = 4,
		.gpkt_offset = 632,
		.stack_offset = BSA_A0_OFF,
	},
	{
		/* A1 */
		.idx = 0x9f,
		.regno = 0x0001,
		.byte_size = 4,
		.gpkt_offset = 636,
	},
	{
		/* A2 */
		.idx = 0xa0,
		.regno = 0x0002,
		.byte_size = 4,
		.gpkt_offset = 640,
		.stack_offset = BSA_A2_OFF,
	},
	{
		/* A3 */
		.idx = 0xa1,
		.regno = 0x0003,
		.byte_size = 4,
		.gpkt_offset = 644,
		.stack_offset = BSA_A3_OFF,
	},
	{
		/* A4 */
		.idx = 0xa2,
		.regno = 0x0004,
		.byte_size = 4,
		.gpkt_offset = 648,
		.stack_offset = -16,
	},
	{
		/* A5 */
		.idx = 0xa3,
		.regno = 0x0005,
		.byte_size = 4,
		.gpkt_offset = 652,
		.stack_offset = -12,
	},
	{
		/* A6 */
		.idx = 0xa4,
		.regno = 0x0006,
		.byte_size = 4,
		.gpkt_offset = 656,
		.stack_offset = -8,
	},
	{
		/* A7 */
		.idx = 0xa5,
		.regno = 0x0007,
		.byte_size = 4,
		.gpkt_offset = 660,
		.stack_offset = -4,
	},
	{
		/* A8 */
		.idx = 0xa6,
		.regno = 0x0008,
		.byte_size = 4,
		.gpkt_offset = 664,
		.stack_offset = -32,
	},
	{
		/* A9 */
		.idx = 0xa7,
		.regno = 0x0009,
		.byte_size = 4,
		.gpkt_offset = 668,
		.stack_offset = -28,
	},
	{
		/* A10 */
		.idx = 0xa8,
		.regno = 0x000A,
		.byte_size = 4,
		.gpkt_offset = 672,
		.stack_offset = -24,
	},
	{
		/* A11 */
		.idx = 0xa9,
		.regno = 0x000B,
		.byte_size = 4,
		.gpkt_offset = 676,
		.stack_offset = -20,
	},
	{
		/* A12 */
		.idx = 0xaa,
		.regno = 0x000C,
		.byte_size = 4,
		.gpkt_offset = 680,
		.stack_offset = -48,
	},
	{
		/* A13 */
		.idx = 0xab,
		.regno = 0x000D,
		.byte_size = 4,
		.gpkt_offset = 684,
		.stack_offset = -44,
	},
	{
		/* A14 */
		.idx = 0xac,
		.regno = 0x000E,
		.byte_size = 4,
		.gpkt_offset = 688,
		.stack_offset = -40,
	},
	{
		/* A15 */
		.idx = 0xad,
		.regno = 0x000F,
		.byte_size = 4,
		.gpkt_offset = 692,
		.stack_offset = -36,
	},
#endif
};

struct gdb_ctx xtensa_gdb_ctx = {
	.regs = gdb_reg_list,
	.num_regs = ARRAY_SIZE(gdb_reg_list),
};
