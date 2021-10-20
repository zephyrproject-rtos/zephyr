/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_MEMORY_H
#define __INC_MEMORY_H

#include <cavs-mem.h>

/* boot loader in IMR */
#define IMR_BOOT_LDR_TEXT_ENTRY_BASE	0xB000A000
#define IMR_BOOT_LDR_TEXT_ENTRY_SIZE	0x120

#define IMR_BOOT_LDR_LIT_BASE \
	(IMR_BOOT_LDR_TEXT_ENTRY_BASE + IMR_BOOT_LDR_TEXT_ENTRY_SIZE)
#define IMR_BOOT_LDR_LIT_SIZE		0x100

#define IMR_BOOT_LDR_TEXT_BASE \
	(IMR_BOOT_LDR_LIT_BASE + IMR_BOOT_LDR_LIT_SIZE)
#define IMR_BOOT_LDR_TEXT_SIZE	0x1c00

#define IMR_BOOT_LDR_DATA_BASE	0xb0002000
#define IMR_BOOT_LDR_DATA_SIZE	0x1000

#define IMR_BOOT_LDR_BSS_BASE	0xb0100000
#define IMR_BOOT_LDR_BSS_SIZE	0x10000

#define BOOT_LDR_STACK_BASE		(L2_SRAM_BASE + L2_SRAM_SIZE - \
					BOOT_LDR_STACK_SIZE)
#define BOOT_LDR_STACK_SIZE		(4 * 0x1000)

/* Manifest base address in IMR - used by boot loader copy procedure. */
#define IMR_BOOT_LDR_MANIFEST_BASE	0xB0004000

/* Manifest size (seems unused). */
#define IMR_BOOT_LDR_MANIFEST_SIZE	0x6000

#define SRAM_ALIAS_BASE		0x9E000000
#define SRAM_ALIAS_MASK		0xFF000000
#define SRAM_ALIAS_OFFSET	0x20000000

/* IRQ controller */
#define IRQ_BASE		0x00001600
#define IRQ_SIZE		0x00000200

/* IPC to the host */
#define IPC_HOST_BASE		0x00001180
#define IPC_HOST_SIZE		0x00000020

/* intra DSP  IPC */
#define IPC_DSP_SIZE		0x00000080
#define IPC_DSP_BASE(x)		(0x00001200 + x * IPC_DSP_SIZE)

#endif /* __INC_MEMORY_H */
