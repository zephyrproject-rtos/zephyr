/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#include "soc.h"

#define N22_CORE_START (DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + DT_REG_SIZE(DT_CHOSEN(zephyr_flash)))
#define IPC_REMOTE_TX_START (DT_REG_ADDR(DT_N_NODELABEL_sram_rx))
#define IPC_REMOTE_TX_END (IPC_REMOTE_TX_START + DT_REG_SIZE(DT_N_NODELABEL_sram_rx))

	.option push
	.option norelax

SECTION_FUNC(init, init)
	.org 0x0

	.global reset_vector

reset_vector:
	.global _start
	.type _start,@function

#if !defined(CONFIG_BOOTLOADER_MCUBOOT)
	.word (0x31707848)                 #/ magic
	.word (0x33e90d91)                 #/ header crc32
	.org 0x14
	.word (0xa0000080)                 #/ start address

	.org 0xa0
#endif

_start:                                #/ after reset starts on D25 core

	lui    t0, 0
	la     t2, IPC_REMOTE_TX_START
	la     t3, IPC_REMOTE_TX_END
_ZERO_IPC_REMOTE_TX_BEGIN:
	bleu   t3, t2, _start_n22
	sw     t0, 0(t2)
	addi   t2, t2, 4
	j      _ZERO_IPC_REMOTE_TX_BEGIN

_start_n22:

	lui    t0, 0xf0100
	lui    t1, %hi(N22_CORE_START)
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
	call   __start                     #/ jump to kernel entry
1:	j      1b

	.option pop
