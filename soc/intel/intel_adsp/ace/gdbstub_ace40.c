/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <xtensa_asm2_context.h>

#include <offsets.h>

static struct xtensa_register gdb_reg_list[] = {
	{
		/* PC */
		.idx = 0,
		.regno = 0x20,
		.byte_size = 4,
		.gpkt_offset = 0,
		.stack_offset = ___xtensa_irq_bsa_t_pc_OFFSET,
	},
	{
		/* AR0 */
		.idx = 1,
		.regno = 0x100,
		.byte_size = 4,
		.gpkt_offset = 4,
	},
	{
		/* AR1 */
		.idx = 2,
		.regno = 0x101,
		.byte_size = 4,
		.gpkt_offset = 8,
	},
	{
		/* AR2 */
		.idx = 3,
		.regno = 0x102,
		.byte_size = 4,
		.gpkt_offset = 12,
	},
	{
		/* AR3 */
		.idx = 4,
		.regno = 0x103,
		.byte_size = 4,
		.gpkt_offset = 16,
	},
	{
		/* AR4 */
		.idx = 5,
		.regno = 0x104,
		.byte_size = 4,
		.gpkt_offset = 20,
	},
	{
		/* AR5 */
		.idx = 6,
		.regno = 0x105,
		.byte_size = 4,
		.gpkt_offset = 24,
	},
	{
		/* AR6 */
		.idx = 7,
		.regno = 0x106,
		.byte_size = 4,
		.gpkt_offset = 28,
	},
	{
		/* AR7 */
		.idx = 8,
		.regno = 0x107,
		.byte_size = 4,
		.gpkt_offset = 32,
	},
	{
		/* AR8 */
		.idx = 9,
		.regno = 0x108,
		.byte_size = 4,
		.gpkt_offset = 36,
	},
	{
		/* AR9 */
		.idx = 10,
		.regno = 0x109,
		.byte_size = 4,
		.gpkt_offset = 40,
	},
	{
		/* AR10 */
		.idx = 11,
		.regno = 0x10a,
		.byte_size = 4,
		.gpkt_offset = 44,
	},
	{
		/* AR11 */
		.idx = 12,
		.regno = 0x10b,
		.byte_size = 4,
		.gpkt_offset = 48,
	},
	{
		/* AR12 */
		.idx = 13,
		.regno = 0x10c,
		.byte_size = 4,
		.gpkt_offset = 52,
	},
	{
		/* AR13 */
		.idx = 14,
		.regno = 0x10d,
		.byte_size = 4,
		.gpkt_offset = 56,
	},
	{
		/* AR14 */
		.idx = 15,
		.regno = 0x10e,
		.byte_size = 4,
		.gpkt_offset = 60,
	},
	{
		/* AR15 */
		.idx = 16,
		.regno = 0x10f,
		.byte_size = 4,
		.gpkt_offset = 64,
	},
	{
		/* AR16 */
		.idx = 17,
		.regno = 0x110,
		.byte_size = 4,
		.gpkt_offset = 68,
	},
	{
		/* AR17 */
		.idx = 18,
		.regno = 0x111,
		.byte_size = 4,
		.gpkt_offset = 72,
	},
	{
		/* AR18 */
		.idx = 19,
		.regno = 0x112,
		.byte_size = 4,
		.gpkt_offset = 76,
	},
	{
		/* AR19 */
		.idx = 20,
		.regno = 0x113,
		.byte_size = 4,
		.gpkt_offset = 80,
	},
	{
		/* AR20 */
		.idx = 21,
		.regno = 0x114,
		.byte_size = 4,
		.gpkt_offset = 84,
	},
	{
		/* AR21 */
		.idx = 22,
		.regno = 0x115,
		.byte_size = 4,
		.gpkt_offset = 88,
	},
	{
		/* AR22 */
		.idx = 23,
		.regno = 0x116,
		.byte_size = 4,
		.gpkt_offset = 92,
	},
	{
		/* AR23 */
		.idx = 24,
		.regno = 0x117,
		.byte_size = 4,
		.gpkt_offset = 96,
	},
	{
		/* AR24 */
		.idx = 25,
		.regno = 0x118,
		.byte_size = 4,
		.gpkt_offset = 100,
	},
	{
		/* AR25 */
		.idx = 26,
		.regno = 0x119,
		.byte_size = 4,
		.gpkt_offset = 104,
	},
	{
		/* AR26 */
		.idx = 27,
		.regno = 0x11a,
		.byte_size = 4,
		.gpkt_offset = 108,
	},
	{
		/* AR27 */
		.idx = 28,
		.regno = 0x11b,
		.byte_size = 4,
		.gpkt_offset = 112,
	},
	{
		/* AR28 */
		.idx = 29,
		.regno = 0x11c,
		.byte_size = 4,
		.gpkt_offset = 116,
	},
	{
		/* AR29 */
		.idx = 30,
		.regno = 0x11d,
		.byte_size = 4,
		.gpkt_offset = 120,
	},
	{
		/* AR30 */
		.idx = 31,
		.regno = 0x11e,
		.byte_size = 4,
		.gpkt_offset = 124,
	},
	{
		/* AR31 */
		.idx = 32,
		.regno = 0x11f,
		.byte_size = 4,
		.gpkt_offset = 128,
	},
	{
		/* AR32 */
		.idx = 33,
		.regno = 0x120,
		.byte_size = 4,
		.gpkt_offset = 132,
	},
	{
		/* AR33 */
		.idx = 34,
		.regno = 0x121,
		.byte_size = 4,
		.gpkt_offset = 136,
	},
	{
		/* AR34 */
		.idx = 35,
		.regno = 0x122,
		.byte_size = 4,
		.gpkt_offset = 140,
	},
	{
		/* AR35 */
		.idx = 36,
		.regno = 0x123,
		.byte_size = 4,
		.gpkt_offset = 144,
	},
	{
		/* AR36 */
		.idx = 37,
		.regno = 0x124,
		.byte_size = 4,
		.gpkt_offset = 148,
	},
	{
		/* AR37 */
		.idx = 38,
		.regno = 0x125,
		.byte_size = 4,
		.gpkt_offset = 152,
	},
	{
		/* AR38 */
		.idx = 39,
		.regno = 0x126,
		.byte_size = 4,
		.gpkt_offset = 156,
	},
	{
		/* AR39 */
		.idx = 40,
		.regno = 0x127,
		.byte_size = 4,
		.gpkt_offset = 160,
	},
	{
		/* AR40 */
		.idx = 41,
		.regno = 0x128,
		.byte_size = 4,
		.gpkt_offset = 164,
	},
	{
		/* AR41 */
		.idx = 42,
		.regno = 0x129,
		.byte_size = 4,
		.gpkt_offset = 168,
	},
	{
		/* AR42 */
		.idx = 43,
		.regno = 0x12a,
		.byte_size = 4,
		.gpkt_offset = 172,
	},
	{
		/* AR43 */
		.idx = 44,
		.regno = 0x12b,
		.byte_size = 4,
		.gpkt_offset = 176,
	},
	{
		/* AR44 */
		.idx = 45,
		.regno = 0x12c,
		.byte_size = 4,
		.gpkt_offset = 180,
	},
	{
		/* AR45 */
		.idx = 46,
		.regno = 0x12d,
		.byte_size = 4,
		.gpkt_offset = 184,
	},
	{
		/* AR46 */
		.idx = 47,
		.regno = 0x12e,
		.byte_size = 4,
		.gpkt_offset = 188,
	},
	{
		/* AR47 */
		.idx = 48,
		.regno = 0x12f,
		.byte_size = 4,
		.gpkt_offset = 192,
	},
	{
		/* AR48 */
		.idx = 49,
		.regno = 0x130,
		.byte_size = 4,
		.gpkt_offset = 196,
	},
	{
		/* AR49 */
		.idx = 50,
		.regno = 0x131,
		.byte_size = 4,
		.gpkt_offset = 200,
	},
	{
		/* AR50 */
		.idx = 51,
		.regno = 0x132,
		.byte_size = 4,
		.gpkt_offset = 204,
	},
	{
		/* AR51 */
		.idx = 52,
		.regno = 0x133,
		.byte_size = 4,
		.gpkt_offset = 208,
	},
	{
		/* AR52 */
		.idx = 53,
		.regno = 0x134,
		.byte_size = 4,
		.gpkt_offset = 212,
	},
	{
		/* AR53 */
		.idx = 54,
		.regno = 0x135,
		.byte_size = 4,
		.gpkt_offset = 216,
	},
	{
		/* AR54 */
		.idx = 55,
		.regno = 0x136,
		.byte_size = 4,
		.gpkt_offset = 220,
	},
	{
		/* AR55 */
		.idx = 56,
		.regno = 0x137,
		.byte_size = 4,
		.gpkt_offset = 224,
	},
	{
		/* AR56 */
		.idx = 57,
		.regno = 0x138,
		.byte_size = 4,
		.gpkt_offset = 228,
	},
	{
		/* AR57 */
		.idx = 58,
		.regno = 0x139,
		.byte_size = 4,
		.gpkt_offset = 232,
	},
	{
		/* AR58 */
		.idx = 59,
		.regno = 0x13a,
		.byte_size = 4,
		.gpkt_offset = 236,
	},
	{
		/* AR59 */
		.idx = 60,
		.regno = 0x13b,
		.byte_size = 4,
		.gpkt_offset = 240,
	},
	{
		/* AR60 */
		.idx = 61,
		.regno = 0x13c,
		.byte_size = 4,
		.gpkt_offset = 244,
	},
	{
		/* AR61 */
		.idx = 62,
		.regno = 0x13d,
		.byte_size = 4,
		.gpkt_offset = 248,
	},
	{
		/* AR62 */
		.idx = 63,
		.regno = 0x13e,
		.byte_size = 4,
		.gpkt_offset = 252,
	},
	{
		/* AR63 */
		.idx = 64,
		.regno = 0x13f,
		.byte_size = 4,
		.gpkt_offset = 256,
	},
	{
		/* LBEG */
		.idx = 65,
		.regno = 0x200,
		.byte_size = 4,
		.gpkt_offset = 260,
		.stack_offset = ___xtensa_irq_bsa_t_lbeg_OFFSET,
	},
	{
		/* LEND */
		.idx = 66,
		.regno = 0x201,
		.byte_size = 4,
		.gpkt_offset = 264,
		.stack_offset = ___xtensa_irq_bsa_t_lend_OFFSET,
	},
	{
		/* LCOUNT */
		.idx = 67,
		.regno = 0x202,
		.byte_size = 4,
		.gpkt_offset = 268,
		.stack_offset = ___xtensa_irq_bsa_t_lcount_OFFSET,
	},
	{
		/* SAR */
		.idx = 68,
		.regno = 0x203,
		.byte_size = 4,
		.gpkt_offset = 272,
		.stack_offset = ___xtensa_irq_bsa_t_sar_OFFSET,
	},
	{
		/* PREFCTL */
		.idx = 69,
		.regno = 0x228,
		.byte_size = 4,
		.gpkt_offset = 276,
		.is_read_only = 1,
	},
	{
		/* WINDOWBASE */
		.idx = 70,
		.regno = 0x248,
		.byte_size = 4,
		.gpkt_offset = 280,
		.is_read_only = 1,
	},
	{
		/* WINDOWSTART */
		.idx = 71,
		.regno = 0x249,
		.byte_size = 4,
		.gpkt_offset = 284,
		.is_read_only = 1,
	},
	{
		/* PS */
		.idx = 74,
		.regno = 0x2e6,
		.byte_size = 4,
		.gpkt_offset = 296,
		.stack_offset = ___xtensa_irq_bsa_t_ps_OFFSET,
	},
	{
		/* THREADPTR */
		.idx = 75,
		.regno = 0x3e7,
		.byte_size = 4,
		.gpkt_offset = 300,
		IF_ENABLED(CONFIG_THREAD_LOCAL_STORAGE,
		  (.stack_offset = ___xtensa_irq_bsa_t_threadptr_OFFSET,))
	},
	{
		/* BR */
		.idx = 76,
		.regno = 0x204,
		.byte_size = 4,
		.gpkt_offset = 304,
	},
	{
		/* SCOMPARE1 */
		.idx = 77,
		.regno = 0x20c,
		.byte_size = 4,
		.gpkt_offset = 308,
		.stack_offset = ___xtensa_irq_bsa_t_scompare1_OFFSET,
	},
	{
		/* PTEVADDR */
		.idx = 130,
		.regno = 0x253,
		.byte_size = 4,
		.gpkt_offset = 684,
	},
	{
		/* VADDRSTATUS */
		.idx = 131,
		.regno = 0x254,
		.byte_size = 4,
		.gpkt_offset = 688,
	},
	{
		/* VADDR0 */
		.idx = 132,
		.regno = 0x255,
		.byte_size = 4,
		.gpkt_offset = 692,
	},
	{
		/* VADDR1 */
		.idx = 133,
		.regno = 0x256,
		.byte_size = 4,
		.gpkt_offset = 696,
	},
	{
		/* RASID */
		.idx = 135,
		.regno = 0x25a,
		.byte_size = 4,
		.gpkt_offset = 704,
	},
	{
		/* MEMCTL */
		.idx = 140,
		.regno = 0x261,
		.byte_size = 4,
		.gpkt_offset = 724,
	},
	{
		/* ATOMCTL */
		.idx = 141,
		.regno = 0x263,
		.byte_size = 4,
		.gpkt_offset = 728,
	},
	{
		/* EPC1 */
		.idx = 149,
		.regno = 0x2b1,
		.byte_size = 4,
		.gpkt_offset = 760,
	},
	{
		/* EPC2 */
		.idx = 150,
		.regno = 0x2b2,
		.byte_size = 4,
		.gpkt_offset = 764,
	},
	{
		/* EPC3 */
		.idx = 151,
		.regno = 0x2b3,
		.byte_size = 4,
		.gpkt_offset = 768,
	},
	{
		/* EPC4 */
		.idx = 152,
		.regno = 0x2b4,
		.byte_size = 4,
		.gpkt_offset = 772,
	},
	{
		/* EPC5 */
		.idx = 153,
		.regno = 0x2b5,
		.byte_size = 4,
		.gpkt_offset = 776,
	},
	{
		/* DEPC */
		.idx = 154,
		.regno = 0x2c0,
		.byte_size = 4,
		.gpkt_offset = 780,
	},
	{
		/* EPS2 */
		.idx = 155,
		.regno = 0x2c2,
		.byte_size = 4,
		.gpkt_offset = 784,
	},
	{
		/* EPS3 */
		.idx = 156,
		.regno = 0x2c3,
		.byte_size = 4,
		.gpkt_offset = 788,
	},
	{
		/* EPS4 */
		.idx = 157,
		.regno = 0x2c4,
		.byte_size = 4,
		.gpkt_offset = 792,
	},
	{
		/* EPS5 */
		.idx = 158,
		.regno = 0x2c5,
		.byte_size = 4,
		.gpkt_offset = 796,
	},
	{
		/* EXCSAVE1 */
		.idx = 159,
		.regno = 0x2d1,
		.byte_size = 4,
		.gpkt_offset = 800,
	},
	{
		/* EXCSAVE2 */
		.idx = 160,
		.regno = 0x2d2,
		.byte_size = 4,
		.gpkt_offset = 804,
	},
	{
		/* EXCSAVE3 */
		.idx = 161,
		.regno = 0x2d3,
		.byte_size = 4,
		.gpkt_offset = 808,
	},
	{
		/* EXCSAVE4 */
		.idx = 162,
		.regno = 0x2d4,
		.byte_size = 4,
		.gpkt_offset = 812,
	},
	{
		/* EXCSAVE5 */
		.idx = 163,
		.regno = 0x2d5,
		.byte_size = 4,
		.gpkt_offset = 816,
	},
	{
		/* INTERRUPT */
		.idx = 165,
		.regno = 0x2e2,
		.byte_size = 4,
		.gpkt_offset = 824,
	},
	{
		/* INTSET */
		.idx = 166,
		.regno = 0x2e2,
		.byte_size = 4,
		.gpkt_offset = 828,
	},
	{
		/* INTCLEAR */
		.idx = 167,
		.regno = 0x2e3,
		.byte_size = 4,
		.gpkt_offset = 832,
	},
	{
		/* INTENABLE */
		.idx = 168,
		.regno = 0x2e4,
		.byte_size = 4,
		.gpkt_offset = 836,
	},
	{
		/* VECBASE */
		.idx = 169,
		.regno = 0x2e7,
		.byte_size = 4,
		.gpkt_offset = 840,
	},
	{
		/* EXCCAUSE */
		.idx = 170,
		.regno = 0x2e8,
		.byte_size = 4,
		.gpkt_offset = 844,
		.stack_offset = ___xtensa_irq_bsa_t_exccause_OFFSET,
	},
	{
		/* DEBUGCAUSE */
		.idx = 171,
		.regno = 0x2e9,
		.byte_size = 4,
		.gpkt_offset = 848,
	},
	{
		/* EXCVADDR */
		.idx = 176,
		.regno = 0x2ee,
		.byte_size = 4,
		.gpkt_offset = 868,
	},
	{
		/* MISC0 */
		.idx = 179,
		.regno = 0x2f4,
		.byte_size = 4,
		.gpkt_offset = 880,
	},
	{
		/* MISC1 */
		.idx = 180,
		.regno = 0x2f5,
		.byte_size = 4,
		.gpkt_offset = 884,
	},
	{
		/* A0 */
		.idx = 203,
		.regno = 0x00,
		.byte_size = 4,
		.gpkt_offset = 976,
		.stack_offset = ___xtensa_irq_bsa_t_a0_OFFSET,
	},
	{
		/* A1 */
		.idx = 204,
		.regno = 0x01,
		.byte_size = 4,
		.gpkt_offset = 980,
	},
	{
		/* A2 */
		.idx = 205,
		.regno = 0x02,
		.byte_size = 4,
		.gpkt_offset = 984,
		.stack_offset = ___xtensa_irq_bsa_t_a2_OFFSET,
	},
	{
		/* A3 */
		.idx = 206,
		.regno = 0x03,
		.byte_size = 4,
		.gpkt_offset = 988,
		.stack_offset = ___xtensa_irq_bsa_t_a3_OFFSET,
	},
	{
		/* A4 */
		.idx = 207,
		.regno = 0x04,
		.byte_size = 4,
		.gpkt_offset = 992,
		.stack_offset = -16,
	},
	{
		/* A5 */
		.idx = 208,
		.regno = 0x05,
		.byte_size = 4,
		.gpkt_offset = 996,
		.stack_offset = -12,
	},
	{
		/* A6 */
		.idx = 209,
		.regno = 0x06,
		.byte_size = 4,
		.gpkt_offset = 1000,
		.stack_offset = -8,
	},
	{
		/* A7 */
		.idx = 210,
		.regno = 0x07,
		.byte_size = 4,
		.gpkt_offset = 1004,
		.stack_offset = -4,
	},
	{
		/* A8 */
		.idx = 211,
		.regno = 0x08,
		.byte_size = 4,
		.gpkt_offset = 1008,
		.stack_offset = -32,
	},
	{
		/* A9 */
		.idx = 212,
		.regno = 0x09,
		.byte_size = 4,
		.gpkt_offset = 1012,
		.stack_offset = -28,
	},
	{
		/* A10 */
		.idx = 213,
		.regno = 0x0a,
		.byte_size = 4,
		.gpkt_offset = 1016,
		.stack_offset = -24,
	},
	{
		/* A11 */
		.idx = 214,
		.regno = 0x0b,
		.byte_size = 4,
		.gpkt_offset = 1020,
		.stack_offset = -20,
	},
	{
		/* A12 */
		.idx = 215,
		.regno = 0x0c,
		.byte_size = 4,
		.gpkt_offset = 1024,
		.stack_offset = -48,
	},
	{
		/* A13 */
		.idx = 216,
		.regno = 0x0d,
		.byte_size = 4,
		.gpkt_offset = 1028,
		.stack_offset = -44,
	},
	{
		/* A14 */
		.idx = 217,
		.regno = 0x0e,
		.byte_size = 4,
		.gpkt_offset = 1032,
		.stack_offset = -40,
	},
	{
		/* A15 */
		.idx = 218,
		.regno = 0x0f,
		.byte_size = 4,
		.gpkt_offset = 1036,
		.stack_offset = -36,
	},
};

struct gdb_ctx xtensa_gdb_ctx = {
	.regs = gdb_reg_list,
	.num_regs = ARRAY_SIZE(gdb_reg_list),
};
