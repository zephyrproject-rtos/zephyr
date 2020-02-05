/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 *         Xiuli Pan <xiuli.pan@linux.intel.com>
 */

#ifndef __PLATFORM_PLATFORM_H__
#define __PLATFORM_PLATFORM_H__

#include <xtensa/config/core.h>

#include <platform/memory.h>

#define PLATFORM_RESET_MHE_AT_BOOT		1

#define PLATFORM_DISABLE_L2CACHE_AT_BOOT	1

#define PLATFORM_MASTER_CORE_ID			0

#define MAX_CORE_COUNT				2

#if PLATFORM_CORE_COUNT > MAX_CORE_COUNT
#error "Invalid core count - exceeding core limit"
#endif

#if !defined(__ASSEMBLER__) && !defined(LINKER)

#include <platform/mailbox.h>
#include <platform/io.h>
#include <platform/shim.h>

/* Host page size */
#define HOST_PAGE_SIZE		4096

#endif /* !defined(__ASSEMBLER__) && !defined(LINKER) */

#endif /* __PLATFORM_PLATFORM_H__ */
