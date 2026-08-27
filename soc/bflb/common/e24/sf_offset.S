/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

/* Bootload E24 SoCs into a different flash offset before anything else happens:
 * 1. Relocate switch function to any RAM space as XIP will break for a few instructions
 * 2. Set offset from 0x2000 (typically) to flash child range
 * 3. Chain normal startup (__start)
 */

#ifdef CONFIG_SOC_SERIES_BL70XL
/* SF_CTRL_BASE + SF_CTRL_SF_ID0_OFFSET_OFFSET */
#define CORE_0_OFFSET	0x4000b0a0
/* SF_CTRL_BASE + SF_CTRL_SF_ID1_OFFSET_OFFSET */
#define CORE_1_OFFSET	0x4000b0a4
#else
/* SF_CTRL_BASE + SF_CTRL_SF_ID0_OFFSET_OFFSET */
#define CORE_0_OFFSET	0x4000b434
/* SF_CTRL_BASE + SF_CTRL_SF_ID1_OFFSET_OFFSET */
#define CORE_1_OFFSET	0x4000b438
#endif

/* Pick a RAM to use */
#define RAM_ADDR	(DT_REG_ADDR(DT_COMPAT_GET_ANY_STATUS_OKAY(mmio_sram)))

/* Fetch what offset to use from ranges */
#if DT_RANGES_HAS_IDX(DT_CHOSEN(zephyr_flash), 0)
#define ROM_ADDR	(DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_CHOSEN(zephyr_flash), 0))
#else
#define ROM_ADDR	0x0
#endif

GTEXT(__start)

SECTION_FUNC(vectors, __bflb_reset_ram)
.__bflb_reset_ram_start:
	.option push
	.option norelax
	.option norvc
	li a4, ROM_ADDR
	/* Set Flash Offset on core 0 */
	li a5, CORE_0_OFFSET
	sw a4, 0(a5)
	/* Set Flash Offset on core 1 */
	li a5, CORE_1_OFFSET
	sw a4, 0(a5)
	.option pop
	/* Get *absolute* address of __start and continue boot */
	lui a2, %hi(__start)
	addi a2, a2, %lo(__start)
	jr 0(a2)
.__bflb_reset_ram_end:

SECTION_FUNC(vectors, __bflb_reset)
	li a2, RAM_ADDR
	la a3, .__bflb_reset_ram_start
	la a4, .__bflb_reset_ram_end
.copy_loop:
	lw a5, 0(a3)
	sw a5, 0(a2)
	addi a3, a3, 4
	addi a2, a2, 4
	bne a3, a4, .copy_loop
	li a2, RAM_ADDR
	jr 0(a2)
