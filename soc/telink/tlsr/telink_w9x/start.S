/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#include "soc.h"

#define IPC_REMOTE_TX_START (DT_REG_ADDR(DT_NODELABEL(sram_rx)))
#define IPC_REMOTE_TX_END (IPC_REMOTE_TX_START + DT_REG_SIZE(DT_NODELABEL(sram_rx)))

	.option push
	.option norelax

SECTION_FUNC(init, init)
	.org 0x0

	.global reset_vector

reset_vector:
	.global _check_start_core
	.type _check_start_core,@function

#if !defined(CONFIG_BOOTLOADER_MCUBOOT)
	.word (0x31707848)                      #/ magic
	.word (0x33e90d91)                      #/ header crc32
	.org 0x14
	.word (((D25_START_ADDR >> 24) & 0xff)   | \
	       ((D25_START_ADDR >> 8)  & 0xff00) | \
	       ((D25_START_ADDR & 0xff00) << 8)  | \
	       ((D25_START_ADDR & 0xff) << 24)) #/ start address

	.org D25_START_OFFSET
#endif

_check_start_core:

	csrr a0, mhartid
	li t0, CONFIG_RV_BOOT_HART
	beq a0, t0, _start                      #/ started on D25 core

	lui    t0, %hi(D25_RESET_VECTOR)        #/ started on N22 core, change into D25 core
	lui    t1, %hi(_start)
	addi   t1, t1, %lo(_start)
	sw     t1, %lo(D25_RESET_VECTOR)(t0)    #/ D25 core addr: D25_RESET_VECTOR <- _start
	lui    t0, %hi(HART_RESET_CTRL)
	li     t1, D25_MASK
	sw     t1, %lo(HART_RESET_CTRL)(t0)     #/ D25 core reset: HART_RESET_CTRL <- D25_MASK

_start:                                     #/ after reset starts on D25 core

	lui    t0, 0
	la     t2, IPC_REMOTE_TX_START
	la     t3, IPC_REMOTE_TX_END
_ZERO_IPC_REMOTE_TX_BEGIN:
	bleu   t3, t2, _start_n22
	sw     t0, 0(t2)
	addi   t2, t2, 4
	j      _ZERO_IPC_REMOTE_TX_BEGIN

_start_n22:

	lui    t0, %hi(N22_RESET_VECTOR)
	lui    t1, %hi(N22_START_ADDR)
	addi   t1, t1, %lo(N22_START_ADDR)
	sw     t1, %lo(N22_RESET_VECTOR)(t0)    #/ N22 core addr: N22_RESET_VECTOR <- N22_START_ADDR
	lui    t0, %hi(HART_RESET_CTRL)
	li     t1, N22_MASK
	sw     t1, %lo(HART_RESET_CTRL)(t0)     #/ N22 core reset: HART_RESET_CTRL <- N22_MASK

	csrw   mie, zero                        #/ disable interrupts
	li     t0, (1 << 3)
	csrc   mstatus, t0                      #/ Clear Machine Status
	csrw   mepc, zero                       #/ Clear Machine Exception PC
	li     t0, (3 << 6)
	csrc   NDS_MXSTATUS, t0                 #/ Clear Machine Extended Status

#if defined(CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE
	lui    t0, 0xf0900
	li     t1, 0x00
	sw     t1, 0x050(t0)                    #/ SPI flash regular mode: 0xf0900050 <- 0x00
#else
	lui    t0, 0xf0900
	li     t1, 0x05
	sw     t1, 0x050(t0)                    #/ SPI flash quad mode: 0xf0900050 <- 0x05
#endif

#if defined(CONFIG_ICACHE) && CONFIG_ICACHE
	li     t0, 0x1
	csrs   NDS_MCACHE_CTL, t0               #/ enable I-Cache
#endif
#if defined(CONFIG_DCACHE) && CONFIG_DCACHE
	li     t0, 0x2
	csrs   NDS_MCACHE_CTL, t0               #/ enable D-Cache
#endif
	li     t0, (1 << 6)
	csrs   NDS_MMISC_CTL, t0                #/ Enable Misaligned access

_RAM_DLM_INIT:
	la     t1, _RAM_DLM_LMA_START
	la     t2, _RAM_DLM_VMA_START
	la     t3, _RAM_DLM_VMA_END
_RAM_DLM_INIT_BEGIN:
	bleu   t3, t2, _RAM_CODE_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAM_DLM_INIT_BEGIN

_RAM_CODE_INIT:
	la     t1, _RAM_CODE_LMA_START
	la     t2, _RAM_CODE_VMA_START
	la     t3, _RAM_CODE_VMA_END
_RAM_CODE_INIT_BEGIN:
	bleu   t3, t2, _START
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAM_CODE_INIT_BEGIN

_START:
	tail   __start                          #/ jump to kernel entry
1:	j      1b

	.option pop
