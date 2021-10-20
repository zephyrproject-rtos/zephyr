/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2017 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 *         Rander Wang <rander.wang@intel.com>
 */

#ifndef __PLATFORM_LIB_SHIM_H__
#define __PLATFORM_LIB_SHIM_H__

#include <soc/memory.h>
#include <sys/util_macro.h>

#define IRQ_CPU_OFFSET	0x40

/** \brief GPDMA shim registers Control */
#define SHIM_GPDMA_BASE_OFFSET 0x6500
#define SHIM_GPDMA_BASE(x)     (SHIM_GPDMA_BASE_OFFSET + (x) * 0x100)

/** \brief GPDMA Clock Control */
#define SHIM_GPDMA_CLKCTL(x)   (SHIM_GPDMA_BASE(x) + 0x4)
/* LP GPDMA Force Dynamic Clock Gating bits, 0--enable */
#define SHIM_CLKCTL_LPGPDMAFDCGB BIT(0)

#define DSP_INIT_LPGPDMA(x)	(0x71A60 + (2*x))
#define LPGPDMA_CTLOSEL_FLAG	BIT(15)
#define LPGPDMA_CHOSEL_FLAG	0xFF

#define DSP_INIT_GENO	0x71A6C
#define GENO_MDIVOSEL		BIT(1)
#define GENO_DIOPTOSEL		BIT(2)

#endif /* __PLATFORM_LIB_SHIM_H__ */
