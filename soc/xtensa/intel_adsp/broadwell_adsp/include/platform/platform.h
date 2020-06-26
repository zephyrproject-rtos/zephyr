/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 */

#ifdef __SOF_PLATFORM_H__

#ifndef __PLATFORM_PLATFORM_H__
#define __PLATFORM_PLATFORM_H__

#define PLATFORM_LPSRAM_EBB_COUNT 1

#define LPSRAM_BANK_SIZE (64 * 1024)

#define LPSRAM_SIZE (PLATFORM_LPSRAM_EBB_COUNT * LPSRAM_BANK_SIZE)

#if !defined(__ASSEMBLER__) && !defined(LINKER)

#include <arch/lib/wait.h>
#include <sof/drivers/interrupt.h>
#include <sof/lib/clk.h>
#include <sof/lib/mailbox.h>
#include <sof/lib/shim.h>
#include <stddef.h>
#include <stdint.h>

struct ll_schedule_domain;
struct timer;

/*! \def PLATFORM_DEFAULT_CLOCK
 *  \brief clock source for audio pipeline
 *
 *  There are two types of clock: cpu clock which is a internal clock in
 *  xtensa core, and ssp clock which is provided by external HW IP.
 *  The choice depends on HW features on different platform
 */
#define PLATFORM_DEFAULT_CLOCK CLK_CPU(0)

/* IPC Interrupt */
#define PLATFORM_IPC_INTERRUPT	IRQ_NUM_EXT_IA
#define PLATFORM_IPC_INTERRUPT_NAME	NULL

/* Host page size */
#define HOST_PAGE_SIZE			4096
#define PLATFORM_PAGE_TABLE_SIZE	256

/* pipeline IRQ */
#define PLATFORM_SCHEDULE_IRQ	IRQ_NUM_SOFTWARE2
#define PLATFORM_SCHEDULE_IRQ_NAME	NULL

/* Platform stream capabilities */
#define PLATFORM_MAX_CHANNELS	4
#define PLATFORM_MAX_STREAMS	5

/* local buffer size of DMA tracing */
#define DMA_TRACE_LOCAL_SIZE	HOST_PAGE_SIZE

/* trace bytes flushed during panic */
#define DMA_FLUSH_TRACE_SIZE    (MAILBOX_TRACE_SIZE >> 2)

/* the interval of DMA trace copying */
#define DMA_TRACE_PERIOD	500000

/*
 * the interval of reschedule DMA trace copying in special case like half
 * fullness of local DMA trace buffer
 */
#define DMA_TRACE_RESCHEDULE_TIME	100

/* DSP default delay in cycles */
#define PLATFORM_DEFAULT_DELAY	12

/* Platform defined panic code */
static inline void platform_panic(uint32_t p)
{
	shim_write(SHIM_IPCX, MAILBOX_EXCEPTION_OFFSET & 0x3fffffff);
	shim_write(SHIM_IPCD, (SHIM_IPCD_BUSY | p));
}

/**
 * \brief Platform specific CPU entering idle.
 * May be power-optimized using platform specific capabilities.
 * @param level Interrupt level.
 */
static inline void platform_wait_for_interrupt(int level)
{
	if (arch_interrupt_get_level())
		panic(SOF_IPC_PANIC_WFI);

	arch_wait_for_interrupt(level);
}

extern intptr_t _module_init_start;
extern intptr_t _module_init_end;

#endif
#endif /* __PLATFORM_PLATFORM_H__ */

#else

#error "This file shouldn't be included from outside of sof/platform.h"

#endif /* __SOF_PLATFORM_H__ */
