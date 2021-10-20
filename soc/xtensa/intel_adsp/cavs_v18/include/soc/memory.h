/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_MEMORY_H
#define __INC_MEMORY_H

#include <cavs/cpu.h>
#include <cavs-mem.h>

#define SRAM_ALIAS_BASE		0x9E000000
#define SRAM_ALIAS_MASK		0xFF000000
#define SRAM_ALIAS_OFFSET	0x20000000

/* IRQ controller */
#define IRQ_BASE		0x00078800
#define IRQ_SIZE		0x00000200

/* IPC to the host */
#define IPC_HOST_BASE		0x00071E00
#define IPC_HOST_SIZE		0x00000020

/* intra DSP  IPC */
#define IPC_DSP_SIZE		0x00000080
#define IPC_DSP_BASE(x)		(0x00001200 + x * IPC_DSP_SIZE)

#endif /* __INC_MEMORY_H */
