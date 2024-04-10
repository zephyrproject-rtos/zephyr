/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#include "soc.h"

#define N22_CORE_START (DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + DT_REG_SIZE(DT_CHOSEN(zephyr_flash)))

	.option push
	.option norelax

SECTION_FUNC(init, init)
	.org 0x0

	.global reset_vector

reset_vector:
	.global _start
	.type _start,@function

	.word (0x31707848)	               #/ magic
	.word (0x33e90d91)                 #/ header crc32
	.org 0x14
	.word (0xa0000080)                 #/ start address

	.org 0xa0
_start:                                #/ after reset starts on D25 core

	lui    t0, 0xf0100
	lui	   t1, %hi(N22_CORE_START)
	addi   t1, t1, %lo(N22_CORE_START)
	sw     t1, 0x204(t0)               #/ N22 core address: 0xf0100204 <- N22_CORE_START
	lui    t0, 0xf1700
	li     t1, 0x10
	sw     t1, 0x218(t0)               #/ N22 core reset: 0xf1700218 <- 0x10

	csrw   mie, zero                   #/ disable interrupts
	li     t0, (1 << 3)
	csrc   mstatus, t0                 #/ Clear Machine Status
	csrw   mepc, zero                  #/ Clear Machine Exception PC
	li     t0, (3 << 6)
	csrc   NDS_MXSTATUS, t0            #/ Clear Machine Extended Status

#if defined(CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE
	lui    t0, 0xf0900
	li     t1, 0x00
	sw     t1, 0x050(t0)               #/ SPI flash regular mode: 0xf0900050 <- 0x00
#else
	lui    t0, 0xf0900
	li     t1, 0x05
	sw     t1, 0x050(t0)               #/ SPI flash quad mode: 0xf0900050 <- 0x05
#endif

#if defined(CONFIG_ICACHE) && CONFIG_ICACHE
	li     t0, 0x1
	csrs   NDS_MCACHE_CTL, t0          #/ enable I-Cache
#endif
#if defined(CONFIG_DCACHE) && CONFIG_DCACHE
	li     t0, 0x2
	csrs   NDS_MCACHE_CTL, t0          #/ enable D-Cache
#endif
	li     t0, (1 << 6)
	csrs   NDS_MMISC_CTL, t0           #/ Enable Misaligned access

_RAMCODE_INIT:
	la     t1, _RAMCODE_LMA_START
	la     t2, _RAMCODE_VMA_START
	la     t3, _RAMCODE_VMA_END
_RAMCODE_INIT_BEGIN:
	bleu   t3, t2, _START
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAMCODE_INIT_BEGIN

_START:
	call   __start                     #/ jump to kernel entry
1:	j      1b

	.option pop
