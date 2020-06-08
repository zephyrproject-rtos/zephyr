/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 */

#ifndef __PLATFORM_SHIM_H__
#define __PLATFORM_SHIM_H__

#include <platform/memory.h>

#ifndef ASSEMBLY
#include <stdint.h>
#endif

#if !defined(__ASSEMBLER__) && !defined(LINKER)
#include <sys/sys_io.h>
#include <arch/common/sys_io.h>
#endif

#ifndef BIT
#define BIT(b)			(1 << (b))
#endif

/* DSP IPC for Host Registers */
#define IPC_DIPCT		0x00
#define IPC_DIPCTE		0x04
#define IPC_DIPCI		0x08
#define IPC_DIPCIE		0x0c
#define IPC_DIPCCTL		0x10

/* DIPCT */
#define IPC_DIPCT_BUSY		(1 << 31)
#define IPC_DIPCT_MSG_MASK	0x7FFFFFFF

/* DIPCTE */
#define IPC_DIPCTE_MSG_MASK	0x3FFFFFFF

/* DIPCI */
#define IPC_DIPCI_BUSY		(1 << 31)
#define IPC_DIPCI_MSG_MASK	0x7FFFFFFF

/* DIPCIE */
#define IPC_DIPCIE_DONE		(1 << 30)
#define IPC_DIPCIE_MSG_MASK	0x3FFFFFFF

/* DIPCCTL */
#define IPC_DIPCCTL_IPCIDIE	(1 << 1)
#define IPC_DIPCCTL_IPCTBIE	(1 << 0)

#define IPC_DSP_OFFSET		0x10

#define SHIM_PWRCTL		0x90
#define SHIM_PWRSTS		0x92
#define SHIM_LPSCTL		0x94

/* HP & LP SRAM Power Gating */
#define SHIM_HSPGCTL		0x80
#define SHIM_LSPGCTL		0x84
#define SHIM_SPSREQ		0xa0
#define LSPGCTL			SHIM_LSPGCTL

#define SHIM_SPSREQ_RVNNP	(0x1 << 0)

/** \brief LDO Control */
#define SHIM_LDOCTL		0xA4
#define SHIM_LDOCTL_HPSRAM_MASK	(3 << 0)
#define SHIM_LDOCTL_LPSRAM_MASK	(3 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_ON	(3 << 0)
#define SHIM_LDOCTL_LPSRAM_LDO_ON	(3 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_BYPASS	BIT(0)
#define SHIM_LDOCTL_LPSRAM_LDO_BYPASS	BIT(2)
#define SHIM_LDOCTL_HPSRAM_LDO_OFF	(0 << 0)
#define SHIM_LDOCTL_LPSRAM_LDO_OFF	(0 << 2)

#define SHIM_HSPGISTS		0xb0
#define SHIM_LSPGISTS		0xb4
#define LSPGISTS		(SHIM_BASE + SHIM_LSPGISTS)


#define SHIM_LPSCTL_FDSPRUN	(0X1 << 9)
#define SHIM_LPSCTL_FDMARUN	(0X1 << 8)

#define SHIM_L2_MECS		(SHIM_BASE + 0xd0)

#define SHIM_L2_CACHE_CTRL	(SHIM_BASE + 0x500)
#define SHIM_L2_PREF_CFG	(SHIM_BASE + 0x508)
#define SHIM_L2_CACHE_PREF	(SHIM_BASE + 0x510)

/* host windows */
#define DMWBA(x)		(HOST_WIN_BASE(x) + 0x0)
#define DMWLO(x)		(HOST_WIN_BASE(x) + 0x4)

#define DMWBA_ENABLE		(1 << 0)
#define DMWBA_READONLY		(1 << 1)

#if !defined(__ASSEMBLER__) && !defined(LINKER)

static inline uint32_t shim_read(uint32_t reg)
{
	return sys_read32(SHIM_BASE + reg);
}

static inline void shim_write(uint32_t reg, uint32_t val)
{
	sys_write32(val, (SHIM_BASE + reg));
}

static inline uint32_t ipc_read(uint32_t reg)
{
	return sys_read32(IPC_HOST_BASE + reg);
}

static inline void ipc_write(uint32_t reg, uint32_t val)
{
	sys_write32(val, (IPC_HOST_BASE + reg));
}

#endif /* !defined(__ASSEMBLER__) && !defined(LINKER) */

#endif
