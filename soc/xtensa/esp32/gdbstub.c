/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <xtensa-asm2-context.h>
#include <zephyr/debug/gdbstub.h>

/*
 * Address Mappings From ESP32 Technical Reference Manual Version 4.5
 * https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
 */
const struct gdb_mem_region gdb_mem_region_array[] = {
	{
		/* External Memory (Data Bus) */
		.start      = 0x3F400000,
		.end        = 0x3FBFFFFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* Peripheral (Data Bus) */
		.start      = 0x3FF00000,
		.end        = 0x3FF7FFFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* RTC FAST Memory (Data Bus) */
		.start      = 0x3FF80000,
		.end        = 0x3FF81FFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* Internal ROM 1 (Data Bus) */
		.start      = 0x3FF90000,
		.end        = 0x3FF9FFFF,
		.attributes = GDB_MEM_REGION_RO,
		.alignment  = 4,
	},
	{
		/* Internal SRAM 1 and 2 (Data Bus) */
		.start      = 0x3FFAE000,
		.end        = 0x3FFFFFFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* Internal ROM 0 (Instruction Bus) */
		.start      = 0x40000000,
		.end        = 0x4005FFFF,
		.attributes = GDB_MEM_REGION_RO,
		.alignment  = 4,
	},
	{
		/* Internal SRAM 0 and 1 (Instruction Bus) */
		.start      = 0x40070000,
		.end        = 0x400BFFFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* RTC FAST Memory (Instruction Bus) */
		.start      = 0x400C0000,
		.end        = 0x400C1FFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* External Memory (Instruction Bus) */
		.start      = 0x400C2000,
		.end        = 0x400CFFFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/*
		 * Flash memory obtained via GDB memory map
		 * with ESP32's OpenOCD
		 */
		.start      = 0x400D0000,
		.end        = 0x400D5FFF,
		.attributes = GDB_MEM_REGION_RO,
		.alignment  = 4,
	},
	{
		/* External Memory (Instruction Bus) */
		.start      = 0x400D6000,
		.end        = 0x40BFFFFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
	{
		/* RTC SLOW Memory (Data/Instruction Bus) */
		.start      = 0x50000000,
		.end        = 0x50001FFF,
		.attributes = GDB_MEM_REGION_RW,
		.alignment  = 4,
	},
};

const size_t gdb_mem_num_regions = ARRAY_SIZE(gdb_mem_region_array);

static struct xtensa_register gdb_reg_list[] = {
	{
		/* PC */
		.idx = 0,
		.regno = 0x0020,
		.byte_size = 4,
		.gpkt_offset = 0,
		.stack_offset = BSA_PC_OFF,
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
		.regno = 0x0200,
		.byte_size = 4,
		.gpkt_offset = 260,
		.stack_offset = BSA_LBEG_OFF,
	},
	{
		/* LEND */
		.idx = 66,
		.regno = 0x0201,
		.byte_size = 4,
		.gpkt_offset = 264,
		.stack_offset = BSA_LBEG_OFF,
	},
	{
		/* LCOUNT */
		.idx = 67,
		.regno = 0x0202,
		.byte_size = 4,
		.gpkt_offset = 268,
		.stack_offset = BSA_LBEG_OFF,
	},
	{
		/* SAR */
		.idx = 68,
		.regno = 0x0203,
		.byte_size = 4,
		.gpkt_offset = 272,
		.stack_offset = BSA_SAR_OFF,
	},
	{
		/* WINDOWBASE */
		.idx = 69,
		.regno = 0x0248,
		.byte_size = 4,
		.gpkt_offset = 276,
		.is_read_only = 1,
	},
	{
		/* WINDOWSTART */
		.idx = 70,
		.regno = 0x0249,
		.byte_size = 4,
		.gpkt_offset = 280,
		.is_read_only = 1,
	},
	{
		/* PS */
		.idx = 73,
		.regno = 0x02E6,
		.byte_size = 4,
		.gpkt_offset = 292,
		.stack_offset = BSA_PS_OFF,
	},
	{
		/* THREADPTR */
		.idx = 74,
		.regno = 0x02E7,
		.byte_size = 4,
		.gpkt_offset = 296,
#ifdef CONFIG_THREAD_LOCAL_STORAGE
		/* Only saved in stack if TLS is enabled */
		.stack_offset = BSA_THREADPTR_OFF,
#endif
	},
	{
		/* SCOMPARE1 */
		.idx = 76,
		.regno = 0x020C,
		.byte_size = 4,
		.gpkt_offset = 304,
		.stack_offset = BSA_SCOMPARE1_OFF,
	},
	{
		/* EXCCAUSE */
		.idx = 143,
		.regno = 0x02E8,
		.byte_size = 4,
		.gpkt_offset = 572,
		.stack_offset = BSA_EXCCAUSE_OFF,
	},
	{
		/* DEBUGCAUSE */
		.idx = 144,
		.regno = 0x02E9,
		.byte_size = 4,
		.gpkt_offset = 576,
	},
	{
		/* EXCVADDR */
		.idx = 149,
		.regno = 0x02EE,
		.byte_size = 4,
		.gpkt_offset = 596,
	},
	{
		/* A0 */
		.idx = 157,
		.regno = 0x0000,
		.byte_size = 4,
		.gpkt_offset = 628,
		.stack_offset = BSA_A0_OFF,
	},
	{
		/* A1 */
		.idx = 158,
		.regno = 0x0001,
		.byte_size = 4,
		.gpkt_offset = 632,
	},
	{
		/* A2 */
		.idx = 159,
		.regno = 0x0002,
		.byte_size = 4,
		.gpkt_offset = 636,
		.stack_offset = BSA_A2_OFF,
	},
	{
		/* A3 */
		.idx = 160,
		.regno = 0x0003,
		.byte_size = 4,
		.gpkt_offset = 640,
		.stack_offset = BSA_A3_OFF,
	},
	{
		/* A4 */
		.idx = 161,
		.regno = 0x0004,
		.byte_size = 4,
		.gpkt_offset = 644,
		.stack_offset = -16,
	},
	{
		/* A5 */
		.idx = 162,
		.regno = 0x0005,
		.byte_size = 4,
		.gpkt_offset = 648,
		.stack_offset = -12,
	},
	{
		/* A6 */
		.idx = 163,
		.regno = 0x0006,
		.byte_size = 4,
		.gpkt_offset = 652,
		.stack_offset = -8,
	},
	{
		/* A7 */
		.idx = 164,
		.regno = 0x0007,
		.byte_size = 4,
		.gpkt_offset = 656,
		.stack_offset = -4,
	},
	{
		/* A8 */
		.idx = 165,
		.regno = 0x0008,
		.byte_size = 4,
		.gpkt_offset = 660,
		.stack_offset = -32,
	},
	{
		/* A9 */
		.idx = 166,
		.regno = 0x0009,
		.byte_size = 4,
		.gpkt_offset = 664,
		.stack_offset = -28,
	},
	{
		/* A10 */
		.idx = 167,
		.regno = 0x000A,
		.byte_size = 4,
		.gpkt_offset = 668,
		.stack_offset = -24,
	},
	{
		/* A11 */
		.idx = 168,
		.regno = 0x000B,
		.byte_size = 4,
		.gpkt_offset = 672,
		.stack_offset = -20,
	},
	{
		/* A12 */
		.idx = 169,
		.regno = 0x000C,
		.byte_size = 4,
		.gpkt_offset = 676,
		.stack_offset = -48,
	},
	{
		/* A13 */
		.idx = 170,
		.regno = 0x000D,
		.byte_size = 4,
		.gpkt_offset = 680,
		.stack_offset = -44,
	},
	{
		/* A14 */
		.idx = 171,
		.regno = 0x000E,
		.byte_size = 4,
		.gpkt_offset = 684,
		.stack_offset = -40,
	},
	{
		/* A15 */
		.idx = 172,
		.regno = 0x000F,
		.byte_size = 4,
		.gpkt_offset = 688,
		.stack_offset = -36,
	},
};

struct gdb_ctx xtensa_gdb_ctx = {
	.regs = gdb_reg_list,
	.num_regs = ARRAY_SIZE(gdb_reg_list),
};
