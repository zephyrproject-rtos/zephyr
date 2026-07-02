/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

/* Bootload BL61x or BL808 into a different flash offset before anything else happens:
 * 1. Setup cache system for the operation
 * 2. Relocate switch function to any RAM space as XIP will break for a few instructions
 * 3. Set offset from 0x2000 (typically) to flash child range
 * 4. Chain normal startup (__start)
 */

/* SF_CTRL_BASE + SF_CTRL_SF_ID0_OFFSET_OFFSET */
#define CORE_0_OFFSET	0x2000b0a0
/* SF_CTRL_BASE + SF_CTRL_SF_ID1_OFFSET_OFFSET */
#define CORE_1_OFFSET	0x2000b0a4

#define THEAD_MHCR	0x7C1
#define THEAD_MXSTATUS	0x7C0

/* Pick a RAM to use */
#define RAM_ADDR	(DT_REG_ADDR(DT_COMPAT_GET_ANY_STATUS_OKAY(mmio_sram)))

/* Fetch what offset to use from ranges */
#if DT_RANGES_HAS_IDX(DT_CHOSEN(zephyr_flash), 0)
#define ROM_ADDR	(DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_CHOSEN(zephyr_flash), 0))
#else
#define ROM_ADDR	0x0
#endif

#define MXSTATUS_OR	((1U << 22) | (1U << 15))
#define MHCR_NOCACHE	(~31U)

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
	/* Fence to clear pipeline of bad instructions */
	fence
	fence.i
	.option pop
	/* Get *absolute* address of __start and continue boot */
	lui a2, %hi(__start)
	addi a2, a2, %lo(__start)
	jr 0(a2)
.__bflb_reset_ram_end:

SECTION_FUNC(vectors, __bflb_reset)
	.option push
	.option norelax
	.option norvc
	/* Enable cache extension (and fences) */
	csrr a2, THEAD_MXSTATUS
	li a5,MXSTATUS_OR
        or a5, a2, a5
	csrw THEAD_MXSTATUS, a2
	fence
	fence.i
	/* Disable cache */
	csrr a2, THEAD_MHCR
	andi a2, a2, MHCR_NOCACHE
	csrw THEAD_MHCR, a2
	/* Clear cache */
	th.dcache.iall
	th.icache.iall
	th.dcache.ciall
	fence
	fence.i
	.option pop
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
