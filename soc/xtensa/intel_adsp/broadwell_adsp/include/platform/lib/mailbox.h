/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifdef __SOF_LIB_MAILBOX_H__

#ifndef __PLATFORM_LIB_MAILBOX_H__
#define __PLATFORM_LIB_MAILBOX_H__

#include <sof/lib/memory.h>
#include <config.h>
#include <stddef.h>
#include <stdint.h>

#if CONFIG_BROADWELL
#define MAILBOX_HOST_OFFSET	0x0009E000
#else
#define MAILBOX_HOST_OFFSET	0x0007E000
#endif

#define MAILBOX_DSPBOX_OFFSET	0x0
#define MAILBOX_DSPBOX_SIZE	0x400
#define MAILBOX_DSPBOX_BASE \
	(MAILBOX_BASE + MAILBOX_DSPBOX_OFFSET)

#define MAILBOX_HOSTBOX_OFFSET	MAILBOX_DSPBOX_SIZE
#define MAILBOX_HOSTBOX_SIZE	0x400
#define MAILBOX_HOSTBOX_BASE \
	(MAILBOX_BASE + MAILBOX_HOSTBOX_OFFSET)

#define MAILBOX_EXCEPTION_OFFSET \
	(MAILBOX_HOSTBOX_SIZE + MAILBOX_DSPBOX_SIZE)
#define MAILBOX_EXCEPTION_SIZE	0x100
#define MAILBOX_EXCEPTION_BASE \
	(MAILBOX_BASE + MAILBOX_EXCEPTION_OFFSET)

#define MAILBOX_DEBUG_OFFSET \
	(MAILBOX_EXCEPTION_SIZE + MAILBOX_EXCEPTION_OFFSET)
#define MAILBOX_DEBUG_SIZE	0x100
#define MAILBOX_DEBUG_BASE \
	(MAILBOX_BASE + MAILBOX_DEBUG_OFFSET)

#define MAILBOX_STREAM_OFFSET \
	(MAILBOX_DEBUG_SIZE + MAILBOX_DEBUG_OFFSET)
#define MAILBOX_STREAM_SIZE	0x200
#define MAILBOX_STREAM_BASE \
	(MAILBOX_BASE + MAILBOX_STREAM_OFFSET)

#define MAILBOX_TRACE_OFFSET \
	(MAILBOX_STREAM_SIZE + MAILBOX_STREAM_OFFSET)

#if CONFIG_TRACE
#define MAILBOX_TRACE_SIZE	0x380
#else
#define MAILBOX_TRACE_SIZE	0x0
#endif

#define MAILBOX_TRACE_BASE \
	(MAILBOX_BASE + MAILBOX_TRACE_OFFSET)

static inline void mailbox_sw_reg_write(size_t offset, uint32_t src)
{
	volatile uint32_t *ptr;

	ptr = (volatile uint32_t *)(MAILBOX_DEBUG_BASE + offset);
	*ptr = src;
}

#endif /* __PLATFORM_LIB_MAILBOX_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/mailbox.h"

#endif /* __SOF_LIB_MAILBOX_H__ */
